# Ledger record format v1

Story 6.1 / BMS-26 implements BMS-F-060, BMS-F-063 and BMS-SR-005.
The journal and audit log are separate append-only text files. Each record
ends with a newline; an unterminated final line is corrupt even if its fields
otherwise parse. Platform text streams use LF on Unix and CRLF on Windows.
Readers and writers use the same platform text convention.

## Journal

`J|1|<seq>|<unix_ts>|<type>|<account>|<amount_paise>|<balance_after_paise>|<operator>`

- `J` identifies a journal line; `1` is the format version.
- `seq` is a nonzero unsigned 64-bit sequence number, independent of audit
  sequences. It starts at 1 and advances once per successful append.
- `unix_ts` is signed 64-bit Unix time in seconds from `std::time(nullptr)`;
  tests may inject a clock.
- `type` is `OPEN`, `DEPOSIT`, `WITHDRAW`, `TRANSFER_OUT` or `TRANSFER_IN`.
- `account` is a nonzero `bms::AccountNo` (unsigned 32-bit).
- `amount_paise` and `balance_after_paise` are signed 64-bit `bms::Paise`
  integers (BMS-NF-003). An amount is positive except for `OPEN`, which also
  permits zero. Balances must be nonnegative except for `WITHDRAW` and
  `TRANSFER_OUT`, which support Current account overdrafts (BMS-F-033).
- `operator` is the escaped, nonempty operator identity.

`OPEN` carries the opening balance in both amount and resulting balance:

```text
J|1|1|1793030400|OPEN|42|10000|10000|teller1
```

For balance reconstruction, initialize the account balance from `OPEN` once,
then apply later credits/debits. Do not count its amount a second time. This
record supplies balance history; account metadata still belongs to the account
store and Story 7.2's recovery protocol.

A transfer consists of an outgoing leg followed immediately by an incoming
leg, with equal positive amounts and consecutive journal sequence numbers:

```text
J|1|2|1793030401|TRANSFER_OUT|42|2500|7500|teller1
J|1|3|1793030401|TRANSFER_IN|43|2500|12500|teller1
```

The caller appends these two legs in order. Story 6.1 does not provide an atomic
batch, account updates, or transfer validation; TransactionModule and Story
7.2's write-ahead protocol supply those guarantees (BMS-NF-002, BMS-F-050).

## Audit

`A|1|<seq>|<unix_ts>|<operator>|<action>|<target>|<details>`

`A` identifies an audit line. Version, sequence and timestamp have the same
numeric rules as the journal. `operator` is nonempty; `target` and `details`
may be empty. All three text fields are escaped. Actions are `CUSTOMER_CREATE`,
`CUSTOMER_MODIFY`, `ACCOUNT_OPEN`, `ACCOUNT_CLOSE`, `ACCOUNT_LOCK`,
`ACCOUNT_UNLOCK`, `LOGIN_FAIL` and `REPORT_RUN`.

```text
A|1|1|1793030400|manager1|ACCOUNT_UNLOCK|42|approved
```

Account/auth/report services call `appendAudit` with the session operator and
appropriate action, target and details. They remain responsible for service
authorization and capturing old/new values for customer modifications.

## Escaping and validation

In every free-text field, encode `%` as `%25`, `|` as `%7C`, LF as `%0A`, and
CR as `%0D`. Decode exactly once: literal `%7C` becomes `%257C` on disk and
returns as literal `%7C`. Only these four uppercase escapes are accepted;
incomplete, lowercase or unknown escapes and raw CR/LF are corrupt. Service
input validation remains the first defence; escaping prevents record injection.

Numbers are decimal integers without whitespace or a leading plus. Signed
fields permit a leading minus. Readers check range and consume every byte.
Wrong field counts, prefixes, versions, labels, invalid field values, and
non-increasing or zero sequence numbers return `Error::CorruptRecord`.
Serialization and append input validation return `Error::InvalidInput`.

## Recovery and integration boundary

One `Ledger` writer owns each pair of distinct paths. Do not copy a writer or
construct concurrent writers over the same files. Construction reads both
files and recovers each next sequence independently from its last valid
record. Missing files are empty. A corrupt or unreadable stream blocks its
appends with the corresponding error; it is never silently repaired or
truncated. Read errors leave output vectors empty. Failed appends preserve
the caller's output sequence and latch the error because a write may have
been partial; investigate storage and reconstruct the writer before retrying.
Sequence exhaustion returns `Error::Io` instead of wrapping.

The persistence boundary is `bms::persistence::appendLine(path, line)` and
`readLines(path, out)`. Writes open with `std::ios::app` only and flush before
success. Story 7.2 adds fsync, the write-ahead/COMMIT protocol and recovery
(BMS-NF-002) without changing these signatures. Flushing alone does not
provide crash durability or transaction atomicity.

Any format change **bumps the version field**. Never edit old lines to migrate
them, and never reinterpret v1 fields under a changed definition. There is no
update or delete API for either file.
