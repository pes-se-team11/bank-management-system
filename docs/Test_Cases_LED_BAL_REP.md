# Test case updates — TC-LED, TC-BAL, TC-REP

**Owner of this update:** Adarsha E (PES1UG24AM334) · **Team 11 — Bank Management System**  
**Baseline:** SRS v1.0 requirement IDs and acceptance criteria.  
**Result column:** automated run on Linux, g++ 14, `-Wall -Wextra -Werror -pedantic`, 33 test cases in this group, 0 failures.  
**Where the tests live:** `tests/test_ledger.cpp` (TC-LED, TC-SEC-03, TC-SEC-06), `tests/test_report.cpp` (TC-REP / TC-LED-03, TC-LED-04, TC-AUT-03).  
Run one case: `./build/tests/bms_tests -tc="TC-LED-01*"`  
Run the group: `./build/tests/bms_tests -tc="TC-LED*" -tc="TC-SEC-03*" -tc="TC-SEC-06*" -tc="TC-LED-03*" -tc="TC-LED-04*" -tc="TC-AUT-03*"`  

Test levels: **U** = unit (Ledger/ReportService + in-memory/fake clock), **I** = integration (real file store via Ledger).

---

## TC-LED-01 — Append-only journal & audit capture (BMS-F-060, BMS-F-063)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-LED-01a | BMS-F-060 | U | Fresh Ledger (journal.log, audit.log) | Append OPEN, DEPOSIT, WITHDRAW, TRANSFER_OUT, TRANSFER_IN; read back | Every record round-trips: seq, timestamp, type, account, amount, balance_after, operator_id all match | `TC-LED-01: every journal type round trips including OPEN and both transfer legs` | Pass |
| TC-LED-01b | BMS-F-060 | U | Fresh Ledger | Append 2 transactions, read journal, restart Ledger, append 3rd | Journal sequences continue (1,2,3); audit sequences continue independently; earlier bytes preserved exactly | `TC-LED-01: independent journal and audit sequences continue after restart` | Pass |
| TC-LED-01c | BMS-F-060 | U | Fresh Ledger | Append transaction, read file bytes, append 2nd, read again | 2nd append adds bytes; first N bytes identical to before (no rewrite) | `TC-LED-01: journal and audit appends preserve every earlier byte` | Pass |
| TC-LED-01d | BMS-F-060 | U | Fresh Ledger | Attempt invalid transactions (zero account, negative amount, empty operator, unknown type, balance_after mismatch) | Each returns `InvalidInput`; journal file does not exist or is unchanged; output sequence unchanged | `TC-LED-01: invalid transactions leave files and output sequence unchanged` | Pass |
| TC-LED-01e | BMS-F-063 | U | Fresh Ledger | Append all 8 audit actions (CustomerCreate, CustomerModify, AccountOpen, AccountClose, AccountLock, AccountUnlock, LoginFail, ReportRun) | Each round-trips with correct seq, timestamp, operator_id, action, target, details | `TC-LED-04: privileged actions capture operator action target details and timestamp` | Pass |

---

## TC-LED-02 — Reconciliation of balances against journal (BMS-F-061, BMS-SR-003)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-LED-02a | BMS-F-061 | U | Clean journal: OPEN(42,10000), DEPOSIT(42,2500), WITHDRAW(42,1000), OPEN(43,200), TRANSFER_OUT(42,500), TRANSFER_IN(43,500) | Run reconcile with stored balances {42:11000, 43:700} | Returns Ok, empty rows (everything reconciles) | `TC-LED-02: reconciliation passes on clean data` | Pass |
| TC-LED-02b | BMS-F-061 | U | Empty journal, no accounts | Run reconcile with empty stored balances | Returns Ok, empty rows | `TC-LED-02: reconciliation passes with no journal and no accounts` | Pass |
| TC-LED-02c | BMS-F-061 | U | Clean journal as above | Run reconcile with stored {42:11001} (43 missing) | Returns Ok, 2 rows: account 42 expected 11000 stored 11001; account 43 expected 700 stored 0 | `TC-LED-02: reconciliation names every tampered stored balance` | Pass |
| TC-LED-02d | BMS-F-061 | U | Clean journal as above | Edit journal[1] (DEPOSIT) amount +100; run reconcile with clean stored | Returns Ok, 1 row: account 42 expected 10000 stored 0 (replay stops at divergence) | `TC-LED-02: reconciliation detects an edited journal amount` | Pass |
| TC-LED-02e | BMS-F-061 | U | Clean journal as above | Edit last record (TRANSFER_IN) amount +100 & balance_after +100; run reconcile | Returns Ok, 1 row: account 43 expected 800 stored 700 (chain holds, stored balance catches it) | `TC-LED-02: reconciliation detects a consistently edited final record` | Pass |
| TC-LED-02f | BMS-F-061 | U | Journal starts with DEPOSIT (no OPEN) | Run reconcile | Returns Ok, 1 row: account 42 expected 0 stored 0 (no OPEN start) | `TC-LED-02: a journal history without an OPEN start cannot reconcile` | Pass |
| TC-LED-02g | BMS-F-061 | U | Journal has two OPEN for same account | Run reconcile | Returns Ok, 1 row: account 42 expected 100 stored 0 (repeated OPEN) | `TC-LED-02: a repeated OPEN record cannot reconcile` | Pass |
| TC-LED-02h | BMS-SR-003 | U | Journal with OPEN(0) then WITHDRAW(max, min+1) | Run reconcile with stored = min+1 | Returns Ok, empty rows (integer limits round-trip) | `TC-SEC-03: reconciliation covers integer limits without wraparound` | Pass |
| TC-LED-02i | BMS-SR-003 | U | Journal with OPEN(max) then DEPOSIT(1, max) | Run reconcile with empty stored | Returns Ok, 1 row: account 42 expected max stored 0 (overflow detected, not wrapped) | `TC-SEC-03: reconciliation covers integer limits without wraparound` | Pass |

---

## TC-LED-03 — End-of-day report totals & closing balances (BMS-F-062)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-LED-03a | BMS-F-062 | U | Journal: day-1 OPEN(100001,50000) DEPOSIT(100001,25000); day DEPOSIT(100001,20000) WITHDRAW(100001,5000) TRANSFER_OUT(100001,3000) TRANSFER_IN(100002,3000) DEPOSIT(100003,500) | Run `endOfDay` as Manager | Totals: deposits=20500, withdrawals=5000, transfer_out=3000, transfer_in=3000; 3 balances: 100001=87000, 100002=13000, 100003=500; day_label="26-10-2026" | `TC-LED-03: totals equal the day's journal entries and close every account` | Pass |
| TC-LED-03b | BMS-F-062 | U | Journal: OPEN(100001,50000) DEPOSIT(100001,10000) on day-1; fresh day, no entries | Run `endOfDay` as Manager | Totals all zero; 1 balance: 100001=60000 (history closes even without today's entries) | `TC-LED-03: an empty day totals zero but still closes every account` | Pass |
| TC-LED-03c | BMS-F-062 | U | Fresh journal (no entries) | Run `endOfDay` as Manager | Totals all zero; balances empty; day_label correct for today | `TC-LED-03: an empty journal produces a zero report for the day` | Pass |
| TC-LED-03d | BMS-F-062 | U | Helper: timestamps at IST boundary | `journalDay(1793030400)` (2026-10-26 21:30 IST) | Returns 20752; `journalDay(1793030400 - 19800)` same day; `journalDay(boundary)` = next day index | `journalDay splits timestamps on the IST midnight` | Pass |

---

## TC-LED-04 — Audit trail for privileged actions & report runs (BMS-F-063)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-LED-04a | BMS-F-063 | U | Fresh Ledger | Run `endOfDay` as Manager | Returns Ok; audit log has 1 record: operator_id="manager1", action=REPORT_RUN, target="26-10-2026", details="N accounts closed off" | `TC-LED-04: each manager report writes exactly one audit record` | Pass |
| TC-LED-04b | BMS-F-063 | U | As above | Run `endOfDay` again same day | Returns Ok; audit log has 2 records with different seq numbers | `TC-LED-04: each manager report writes exactly one audit record` | Pass |
| TC-LED-04c | BMS-F-063 | U | Fresh Ledger with 1 account | Run `endOfDay` as Teller | Returns `NotAuthorised`; audit log empty | `TC-LED-03: a refused report writes no audit record` | Pass |
| TC-LED-04d | BMS-F-063 | U | Fresh Ledger | Attempt `appendAudit` with empty operator_id, unknown action | Each returns `InvalidInput`; audit file not created | `TC-LED-04: audit refuses empty operators and unknown actions` | Pass |
| TC-LED-04e | BMS-F-063 | U | Journal file corrupted (truncated/bad line) | Run `endOfDay` as Manager | Returns `CorruptRecord`; report balances empty; audit log empty | `TC-LED-03: a corrupt journal is reported and the report is refused` | Pass |

---

## TC-BAL-02 — Mini-statement (BMS-F-041)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-BAL-02a | BMS-F-041 | U | Journal: 3/10/12 entries for account 42 interleaved with account 43 | `miniStatement(42, out)` | Returns up to 10 newest for account 42 only, descending seq; 3→3, 10→10, 12→10 | `TC-BAL-02: mini-statement returns up to ten newest records for only that account` | Pass |
| TC-BAL-02b | BMS-F-041 | U | Empty journal | `miniStatement(42, out)` | Returns Ok, out empty; journal file does not exist | `TC-BAL-02: mini-statement on an empty journal clears previous output` | Pass |
| TC-BAL-02c | BMS-F-041 | U | Fresh Ledger | `miniStatement(0, out)` | Returns `InvalidInput`, out empty | `TC-BAL-02: mini-statement rejects account zero and clears previous output` | Pass |
| TC-BAL-02d | BMS-F-041 | U | Journal with corrupt record in another account before 10 valid matches | `miniStatement(42, out)` | Returns `CorruptRecord`, out empty, journal file unchanged | `TC-BAL-02: mini-statement propagates journal errors without partial results` | Pass |
| TC-BAL-02e | BMS-F-041 | U | Ledger constructed with directory path as journal file | `miniStatement(42, out)` | Returns `Io`, out empty | `TC-BAL-02: mini-statement propagates journal errors without partial results` | Pass |

---

## TC-REP-01 — ReportService authorization & audit (BMS-F-012, BMS-F-063)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-REP-01a | BMS-F-012 | U | Fresh ReportService | `endOfDay(Teller, report)` | Returns `NotAuthorised`; report empty | `TC-AUT-03 TC-SEC-08: tellers and customers are refused, managers are not` | Pass |
| TC-REP-01b | BMS-F-012 | U | Fresh ReportService | `endOfDay(Customer, report)` | Returns `NotAuthorised`; report empty | `TC-AUT-03 TC-SEC-08: tellers and customers are refused, managers are not` | Pass |
| TC-REP-01c | BMS-F-012 | U | Fresh ReportService | `endOfDay(empty operator, report)` | Returns `NotAuthorised`; report empty | `TC-AUT-03 TC-SEC-08: tellers and customers are refused, managers are not` | Pass |
| TC-REP-01d | BMS-F-012 | U | Fresh ReportService | `endOfDay(Manager, report)` | Returns `Ok`; report populated; audit written | `TC-AUT-03 TC-SEC-08: tellers and customers are refused, managers are not` | Pass |

---

## TC-SEC-03 — Integer limits without wraparound (BMS-SR-003)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-SEC-03a | BMS-SR-003 | U | — | Serialize/parse JournalRecord with max seq, min/max timestamps, max account, max/min amounts | Round-trips exactly; no overflow/wrap | `TC-SEC-03: integer limits round trip and exhausted sequences never wrap` | Pass |
| TC-SEC-03b | BMS-SR-003 | U | Ledger with seq = UINT64_MAX | Attempt append | Returns `Io` (exhausted sequence never wraps); files unchanged | `TC-SEC-03: integer limits round trip and exhausted sequences never wrap` | Pass |

---

## TC-SEC-06 — Append-only integrity & escape handling (BMS-SR-005)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-SEC-06a | BMS-SR-005 | U | — | Serialize journal/audit with reserved chars `| % \n \r` in free text | Escapes as `%7C %25 %0A %0D`; parses back to original | `TC-SEC-06: reserved free text characters escape and round trip exactly` | Pass |
| TC-SEC-06b | BMS-SR-005 | U | — | Parse 40+ malformed journal lines (wrong prefix, bad seq, bad timestamp, unknown type, bad account, bad amount, bad balance, missing operator, bad escape) | Each returns `CorruptRecord`; output struct unchanged | `TC-SEC-06: strict journal parsing refuses malformed fields without changing output` | Pass |
| TC-SEC-06c | BMS-SR-005 | U | — | Parse 20+ malformed audit lines | Each returns `CorruptRecord`; output struct unchanged | `TC-SEC-06: strict audit parsing refuses malformed fields without changing output` | Pass |
| TC-SEC-06d | BMS-SR-005 | U | Journal file with: bad line, duplicate seq, decreasing seq, unterminated | Construct Ledger, call `readJournal`, then `appendTransaction` | Each returns `CorruptRecord`; reads return empty; appends refused; file unchanged | `TC-SEC-06: corrupt or unterminated files refuse reads and further appends` | Pass |
| TC-SEC-06e | BMS-SR-005 | U | Missing journal/audit files | Construct Ledger, call `readJournal`/`readAudit` | Returns Ok, empty vectors | `TC-LED-01: missing files are empty and an unwritable path reports Io` | Pass |
| TC-SEC-06f | BMS-SR-005 | U | Unwritable parent directory | Construct Ledger with missing parent, call `appendTransaction`/`appendAudit` | Returns `Io`; failure latched (retry also Io); file unchanged | `TC-LED-01: missing files are empty and an unwritable path reports Io` | Pass |

---

## Related cases touched by this update

| ID | Req. | Test | Result |
|---|---|---|---|
| TC-AUT-03 | BMS-F-012 | `TC-AUT-03: a Teller is refused the end-of-day report; a Manager succeeds` | Pass |
| TC-SEC-08 | BMS-SR-007 | `TC-SEC-08: account services refuse the wrong role with no menu involved` | Pass |
| TC-LED-01 | BMS-F-060 | `TC-SEC-06: journal and audit appends preserve every earlier byte` | Pass |

---

## Deviations and open points (for the deviations report)

1. **BMS-F-061 — no start-up replay.** `reconcile()` detects mismatches but nothing replays the journal into stored balances at process start. `TC-REL-01` (kill mid-write, restart, verify balances) is therefore **not met**. A start-up replay using `last_seq` is the remaining work.
2. **BMS-F-063 — audit written after report.** If the process crashes between report computation and `appendAudit`, the report ran but left no audit trace. Mirrors the AuthService unlock flow; accepted for v1.0.
3. **BMS-F-041 — mini-statement reads entire journal.** Sequential scan; no index. Acceptable for v1.0 data volumes.
4. **BMS-SR-005 — corrupt records detected, not repaired.** No automatic recovery; operational runbook needed.
5. **IO-failure branches uncovered.** `readJournal` error, `appendAudit` InvalidInput/Io, `journalDay` negative timestamp — require fault-injection seam not yet built (project-wide gap, same as Persistence/Auth modules).

---

*Generated from `docs/Test_Cases_LED_BAL_REP.md`. Re-run `tools/coverage.sh` to refresh coverage figures.*