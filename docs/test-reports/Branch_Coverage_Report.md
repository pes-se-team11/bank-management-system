# Branch Coverage Report

**Project:** Team 11 - Bank Management System (C/C++)
**Commit measured:** `448ecb9` on `main` (after PRs #17-#24)
**Date:** 11-11-2026
**Tools:** g++ (GCC) 14.4.0, gcovr 7.2, cmake version 3.31.6 (Linux, `gcc:14` container)
**Test run:** 163/163 test cases passed, 1966/1966 assertions passed, 0 failed

How it was measured: the whole project was built in Debug with `--coverage -O0`,
the full doctest suite (`bms_tests`) was run once, and `gcovr` collected the
results for the application sources under `src/` only (test code is not
counted). Regenerate with `tools/coverage.sh`. The line-by-line HTML report is
in [`coverage-html/index.html`](coverage-html/index.html).

Branches added by the compiler for C++ exception paths (`throw`) and branches
the compiler proves unreachable are excluded (`--exclude-throw-branches
--exclude-unreachable-branches`), so the figures count real decisions in the
source: each `if`, loop condition, `switch` case and `&&`/`||` operand.

## Summary

| Metric | Taken | Total | Coverage |
|---|---|---|---|
| Branches | 1991 | 2447 | **81.4%** |

## By module

| Module | Path | Owner | Branches | Taken | Coverage |
|---|---|---|---|---|---|
| AccountModule | `src/account/` | P1 Dhanush | 99 | 90 | 90.9% |
| AuthModule | `src/auth/` | P1 Dhanush | 195 | 167 | 85.6% |
| ValidationModule | `src/validation/` | P1 Dhanush | 142 | 129 | 90.8% |
| MenuLayer (CLI) | `src/cli/` | Shared | 814 | 588 | 72.2% |
| TransactionModule | `src/transaction/` | P2 Vidit | 142 | 133 | 93.7% |
| Money type | `src/core/` | P2 Vidit | 18 | 16 | 88.9% |
| PersistenceModule | `src/persistence/` | P2 Vidit | 570 | 443 | 77.7% |
| Platform layer | `src/platform/` | P2 Vidit | 76 | 54 | 71.1% |
| LedgerModule | `src/ledger/` | P3 Adarsha | 341 | 329 | 96.5% |
| ReportModule | `src/report/` | P3 Adarsha | 50 | 42 | 84.0% |

## By file

| File | Branches | Taken | Coverage |
|---|---|---|---|
| `src/account/account_service.cpp` | 99 | 90 | 90.9% |
| `src/auth/auth_service.cpp` | 139 | 112 | 80.6% |
| `src/auth/authorization.cpp` | 9 | 8 | 88.9% |
| `src/auth/crypto.cpp` | 47 | 47 | 100.0% |
| `src/cli/in_memory_stores.h` | 29 | 23 | 79.3% |
| `src/cli/main.cpp` | 36 | 0 | 0.0% |
| `src/cli/menu.cpp` | 749 | 565 | 75.4% |
| `src/core/money.h` | 18 | 16 | 88.9% |
| `src/ledger/ledger.cpp` | 341 | 329 | 96.5% |
| `src/persistence/append_file.cpp` | 39 | 34 | 87.2% |
| `src/persistence/crc32.h` | 6 | 6 | 100.0% |
| `src/persistence/file_account_store.cpp` | 297 | 218 | 73.4% |
| `src/persistence/file_credential_store.cpp` | 168 | 130 | 77.4% |
| `src/persistence/record_types.h` | 60 | 55 | 91.7% |
| `src/platform/platform_posix.cpp` | 76 | 54 | 71.1% |
| `src/report/report_service.cpp` | 50 | 42 | 84.0% |
| `src/transaction/transaction_service.cpp` | 142 | 133 | 93.7% |
| `src/validation/validation.cpp` | 142 | 129 | 90.8% |

## Analysis - Account, Auth and Validation modules (P1 Dhanush)

Account 91%, Auth 86% and Validation 91% of branches are taken. Both outcomes
of every business decision are tested: role allowed / refused (TC-AUT-03,
TC-SEC-08), correct / wrong PIN and lockout on the third failure (TC-AUT-01,
TC-AUT-02), unlock of a locked / unlocked / closed account (TC-AUT-04), closure
at zero / non-zero balance (TC-ACC-04), changed / unchanged customer details
(TC-ACC-03) and valid / invalid input (TC-SEC-04, TC-SEC-05). The untaken
branches are the "store returned an error" side of each storage call - the
same paths explained in the Code Coverage Report - since the in-memory test
stores never fail.


## Analysis - other modules

### LedgerModule (P3 Adarsha)

**96.5 % branch coverage (329 / 341).**

The 12 untaken branches are all **IO-failure paths** in `ledger.cpp`:

- `appendLine` failure (disk full, unwritable) — both the journal and audit sides.
- `readLines` failure in `readJournal`/`readAudit` fast-path.
- The `journal_status_` / `audit_status_` latch after a failed append (subsequent appends return the latched error).
- One `readLines` failure when reading a directory as a file (tested in mini-statement but the branch is in the shared helper).

Both outcomes of every **business decision** are tested:
- Every transaction type (OPEN, DEPOSIT, WITHDRAW, TRANSFER_OUT, TRANSFER_IN) and audit action round-trips.
- Mini-statement: account match / no match, limit 10 / fewer, empty journal, account zero, corruption before 10 matches.
- Reconciliation: clean / tampered balance / edited journal / edited final record / no OPEN / duplicate OPEN / integer overflow / integer limit.
- Escape handling: reserved chars escaped / not escaped; strict parse accept / reject malformed; corrupt file detect / not detect.
- Sequence recovery: independent journal/audit sequences continue after restart.

Requirements covered: TC-LED-01 (BMS-F-060), TC-LED-02 (BMS-F-061), TC-LED-03 (BMS-F-041), TC-LED-04 (BMS-F-063), TC-SEC-03 (BMS-SR-003), TC-SEC-06 (BMS-SR-005), TC-BAL-02 (BMS-F-041).

Gap accepted — fault-injection for `appendLine`/`readLines` is a project-wide improvement tracked in PersistenceModule.

### ReportModule (P3 Adarsha)

**84.0 % branch coverage (42 / 50).**

The 8 untaken branches in `report_service.cpp` are:

- `readJournal` error return (the test Ledger never fails a read).
- `journalDay` negative-timestamp branch (tests use a positive fixed Unix time).
- `appendAudit` `InvalidInput` and `Io` returns (the test Ledger's audit append never fails).

Both outcomes of every **business decision** are tested:
- IST day boundary: same day / next day (`journalDay`).
- Totals: each type (deposit, withdrawal, transfer-out, transfer-in) included / excluded by day.
- Balances: every account with history closes / empty journal produces empty list.
- Authorization: Manager Ok / Teller refused / Customer refused / empty operator refused.
- Audit: successful run writes record / refused run writes no record / corrupt journal refuses report and writes no audit.

Requirements covered: TC-LED-03 (BMS-F-062), TC-LED-04 (BMS-F-063), TC-AUT-03 / TC-SEC-08 (BMS-F-012).

Gap accepted — same IO-failure class as LedgerModule and PersistenceModule; `journalDay` negative branch unreachable with real timestamps.

### MenuLayer (CLI) / `main.cpp` - shared: to be added by their owners.

### TransactionModule, Money type, PersistenceModule, Platform layer (P2 Vidit)

The tables above are for `main` at `297bf67`. PR #24 (`feat/part3-transaction-rules`)
adds Sprint 3 stories 3.3, 5.1, 5.2, 7.5 and a storage fix, so each module is
analysed for `main` and then for the PR.

**TransactionModule - 82.4 % (42 / 51).** Both outcomes of the business
decisions that existed were tested: amount valid / invalid, account active /
not, balance enough / not, Current overdraft inside / beyond (TC-WDR-03). The
9 untaken branches are each the rare side of a guard:

- *Store or journal returns an error* (lines 29, 54, 75, 119): the in-memory
  test store never fails.
- *Deposit ceiling misconfigured* (22): the `deposit_ceiling_ <= 0` operand.
- *Overflow guards* (38, 100): balance near the largest representable value.
- *Bad stored data* (86, 92): a negative overdraft limit, or a balance already
  below its floor.

There was no branch for a daily limit, a Savings minimum or a transfer
because the features did not exist on `main`.

**Money type - 88.9 % (16 / 18).** The two untaken branches are the underflow
operands of `operator+` (line 27) and `operator-` (line 38), the
`paise_ < min - other` direction. The overflow direction is tested.

**PersistenceModule - 74.6 % (418 / 560).** `file_account_store.cpp` 71.9 %,
`file_credential_store.cpp` 77.2 %, `append_file.cpp` 87.2 %, `record_types.h`
70.0 %, `crc32.h` 100 %. The untaken side of most branches is "the stream
failed" (`if (!file) return Error::Io;`), which needs fault injection that does
not exist yet. Branches that decide whether *data is valid* (CRC mismatch,
wrong record size, duplicate key, zero ID) are taken both ways. In
`record_types.h` the compound check on account type and state (lines 112-117)
can only fail for an invalid enum value, and the Locked state was never written
to disk by any test, so those operands were untaken. The counters-file defect
described in the Code Coverage Report was in this module.

**Platform layer - 30.4 % (7 / 23).** The lowest figure in the project, for a
simple reason: `readLineNoEcho` never ran (the menu tests use a string
stream and CI has no terminal), and the error sides of `secureRandom`'s
`open` / `read` calls need `/dev/urandom` to fail.


#### What PR #24 changes (measured on `6792701`, g++ 13, gcovr 8.8)

| Module | Branches before | Branches after |
|---|---|---|
| TransactionModule | 42 / 51 (82.4 %) | 133 / 142 (**93.7 %**) |
| Money type | 16 / 18 (88.9 %) | 16 / 18 (88.9 %) |
| PersistenceModule | 418 / 560 (74.6 %) | 442 / 569 (**77.7 %**) |
| Platform layer | 7 / 23 (30.4 %) | 54 / 76 (**71.1 %**) |

"Before" is re-measured on the same toolchain as "after". In the new
TransactionModule code every decision has both outcomes tested: Savings
minimum (below / exactly at / above), daily limit (inside / exactly at / over /
next bank day), per-transfer and daily transfer ceilings, same account,
closed, locked and unknown account, overflow, insufficient funds, and failure
between the two transfer legs (both legs tried). The 9 untaken branches left
are halves of compound conditions: the `used <= limit` half of the limit check
(false only if the limit is lowered below what an account already used today),
the journal-append failure at transfer line 281, the `deposit_ceiling_ <= 0`
guard, and sub-conditions of the date calculation. Which side of the date
branches runs depends on the day the tests run (January-February, years before
1970), so the figure can move by a branch or two without any code change.

Remaining gaps and what would close them:

| Priority | Action | Effect |
|---|---|---|
| 1 | Fault-injection seam for file open / write | Most of Persistence's ~127 untaken branches |
| 2 | Ledger that fails on its Nth append (test double) | Last transfer branch |
| 3 | `Money` underflow test (`Money(min) - Money(1)`) | +2 branches in Money |
| 4 | Pseudo-terminal test for `readLineNoEcho` (Linux only) | Platform terminal branch |

The full list of lines with untaken branches per file is in
`branch_coverage.txt`.
