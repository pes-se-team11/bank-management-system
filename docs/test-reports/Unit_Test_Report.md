# Unit Test Report — LedgerModule & ReportModule

**Project:** Team 11 — Bank Management System (C/C++)  
**Owner:** Adarsha E (PES1UG24AM334) — P3  
**Commit measured:** `448ecb9` on `main` (after PRs #17–#24)  
**Date:** 12-11-2026  
**Tools:** g++ (GCC) 14.4.0, doctest 2.4.11, gcovr 7.2, CMake 3.31.6 (Linux, `gcc:14` container)  
**Test run:** 163/163 test cases passed, 1966/1966 assertions passed, 0 failed  
**Run command:** `./build/tests/bms_tests` (or `ctest` from `build/`)

---

## 1. Test Suite Summary

| Module | Source files | Test file | Test cases | Assertions | Status |
|---|---|---|---|---|---|
| LedgerModule | `src/ledger/ledger.cpp`, `src/ledger/ledger.h` | `tests/test_ledger.cpp` | 25 | ~700 | **All pass** |
| ReportModule | `src/report/report_service.cpp`, `src/report/report_service.h` | `tests/test_report.cpp` | 8 | 84 | **All pass** |

**Total for P3 (Adarsha):** 33 test cases, ~784 assertions — all passing.

---

## 2. LedgerModule — Coverage & Test Mapping

| Metric | Covered | Total | Coverage |
|---|---|---|---|
| Lines (statements) | 257 | 263 | **97.7 %** |
| Branches | 329 | 341 | **96.5 %** |
| Functions | 30 | 30 | **100.0 %** |

### Test Cases → Requirements Traceability (Ledger)

| Test case ID | SRS Requirement | Level | What it verifies | Automated test name |
|---|---|---|---|---|
| TC-LED-01 | BMS-F-060 | U | Journal & audit append preserves every earlier byte; sequences continue after restart; every transaction type round-trips including OPEN and both transfer legs; invalid transactions leave files/sequence unchanged | `TC-LED-01: journal and audit appends preserve every earlier byte`, `independent journal and audit sequences continue after restart`, `every journal type round trips…`, `invalid transactions leave files and output sequence unchanged` |
| TC-LED-02 | BMS-F-061 | U | Reconciliation passes on clean data; passes with no journal/accounts; names every tampered stored balance; detects edited journal amount; detects consistently edited final record; journal without OPEN start fails; repeated OPEN fails; integer limits without wraparound | `TC-LED-02: reconciliation passes on clean data`, `passes with no journal and no accounts`, `names every tampered stored balance`, `detects an edited journal amount`, `detects a consistently edited final record`, `a journal history without an OPEN start cannot reconcile`, `a repeated OPEN record cannot reconcile`, `reconciliation covers integer limits without wraparound` |
| TC-LED-03 | BMS-F-062 | U | Totals equal the day's journal entries and close every account; empty day totals zero but still closes every account; empty journal produces zero report with correct day label | `TC-LED-03: totals equal the day's journal entries and close every account`, `an empty day totals zero but still closes every account`, `an empty journal produces a zero report for the day` |
| TC-LED-04 | BMS-F-063 | U | Each manager report writes exactly one audit record (REPORT_RUN); a refused report writes no audit record | `TC-LED-04: each manager report writes exactly one audit record`, `a refused report writes no audit record` |
| TC-SEC-03 | BMS-SR-003 | U | Integer limits round-trip and exhausted sequences never wrap | `TC-SEC-03: integer limits round trip and exhausted sequences never wrap` |
| TC-SEC-06 | BMS-SR-005 | U | Reserved free-text characters escape and round-trip exactly; strict journal/audit parsing refuses malformed fields without changing output; corrupt/unterminated files refuse reads and further appends; missing files are empty; unwritable path reports Io | `TC-SEC-06: reserved free text characters escape and round trip exactly`, `strict journal parsing refuses malformed fields…`, `strict audit parsing refuses malformed fields…`, `corrupt or unterminated files refuse reads and further appends`, `missing files are empty and an unwritable path reports Io` |
| TC-SEC-06 (clock) | BMS-F-060 | U | Default clock records current Unix time | `TC-LED-04: the default clock records current Unix time` |

### Uncovered Lines / Branches (Ledger)

| File | Uncovered lines | Reason |
|---|---|---|
| `src/ledger/ledger.cpp` | 6 lines (approx.) | Error paths that require the underlying append file to fail (disk full, unwritable directory). The in-memory test fixtures never trigger these. |
| `src/ledger/ledger.cpp` | 12 branches (approx.) | Same IO-failure branches; one additional branch for the `readLines` failure path in the `readJournal`/`readAudit` fast-path. |

**Gap assessment:** Accepted — the Ledger is an append-only writer; its only failure mode is the underlying filesystem. Fault-injection for `appendLine` would be needed to cover these, which is tracked as a project-wide improvement (see PersistenceModule gap).

---

## 3. ReportModule — Coverage & Test Mapping

| Metric | Covered | Total | Coverage |
|---|---|---|---|
| Lines (statements) | 52 | 56 | **92.9 %** |
| Branches | 42 | 50 | **84.0 %** |
| Functions | 6 | 6 | **100.0 %** |

### Test Cases → Requirements Traceability (Report)

| Test case ID | SRS Requirement | Level | What it verifies | Automated test name |
|---|---|---|---|---|
| TC-LED-03 | BMS-F-062 | U | Totals equal the day's journal entries and close every account; empty day totals zero but still closes every account; empty journal produces zero report with correct day label | `TC-LED-03: totals equal the day's journal entries and close every account`, `an empty day totals zero but still closes every account`, `an empty journal produces a zero report for the day` |
| TC-AUT-03 / TC-SEC-08 | BMS-F-012 | U | Tellers and customers are refused; managers succeed; empty-operator session refused | `TC-AUT-03 TC-SEC-08: tellers and customers are refused, managers are not` |
| TC-LED-04 | BMS-F-063 | U | Each manager report writes exactly one audit record (REPORT_RUN); a refused report writes no audit record | `TC-LED-04: each manager report writes exactly one audit record`, `a refused report writes no audit record` |
| TC-LED-03 (corrupt) | BMS-F-061 | U | A corrupt journal is reported and the report is refused (no audit written) | `TC-LED-03: a corrupt journal is reported and the report is refused` |
| (Helper) | — | U | `journalDay` splits timestamps on the IST midnight boundary correctly | `journalDay splits timestamps on the IST midnight` |

### Uncovered Lines / Branches (Report)

| File | Uncovered lines | Uncovered branches | Reason |
|---|---|---|---|
| `src/report/report_service.cpp` | 4 lines (62, 67, 92, 95) | 8 branches | `readJournal` error return (line 55–57); `journalDay` negative-day branch (line 41–42); `appendAudit` InvalidInput/Io branches (lines 92–95) — the audit store in tests never fails. |

**Gap assessment:** Accepted — the same IO-failure paths as Ledger. The `journalDay` negative branch is unreachable with real timestamps (tests use positive fixed time).

---

## 4. Test Execution Evidence

```bash
$ cd /home/adarsha/projects/bank-management-system
$ ./build/tests/bms_tests
```

**Output (excerpt):**
```
[doctest] test cases: 163 | 163 passed | 0 failed | 0 skipped
[doctest] assertions: 1966 | 1966 passed | 0 failed
```

**Ledger & Report specific:**
```
$ ./build/tests/bms_tests -tc="TC-LED*"
[doctest] test cases: 25 | 25 passed | 0 failed

$ ./build/tests/bms_tests -tc="TC-LED-03*"
[doctest] test cases: 5 | 5 passed | 0 failed

$ ./build/tests/bms_tests -tc="TC-LED-04*"
[doctest] test cases: 4 | 4 passed | 0 failed

$ ./build/tests/bms_tests -tc="TC-SEC-06*"
[doctest] test cases: 5 | 5 passed | 0 failed

$ ./build/tests/bms_tests -tc="TC-SEC-03*"
[doctest] test cases: 2 | 2 passed | 0 failed
```

---

## 5. P1 / P2 Module Summaries (to be completed by owners)

| Module | Owner | Test file | Test cases | Line Coverage | Branch Coverage | Status |
|---|---|---|---|---|---|---|
| AccountModule | P1 Dhanush | `tests/test_account.cpp` | — | 93.0 % | 90.9 % | 📝 To fill |
| AuthModule | P1 Dhanush | `tests/test_auth.cpp`, `tests/test_authorization.cpp` | — | 93.5 % | 85.6 % | 📝 To fill |
| ValidationModule | P1 Dhanush | `tests/test_validation.cpp` | — | 98.7 % | 90.8 % | 📝 To fill |
| TransactionModule | P2 Vidit | `tests/test_transaction.cpp`, `tests/test_transaction_rules.cpp` | — | 98.8 % | 93.7 % | 📝 To fill |
| Money type | P2 Vidit | `tests/test_money.cpp` | — | 100.0 % | 88.9 % | 📝 To fill |
| PersistenceModule | P2 Vidit | `tests/test_persistence.cpp` | — | 86.0 % | 77.7 % | 📝 To fill |
| Platform layer | P2 Vidit | `tests/test_platform.cpp` | — | 82.9 % | 71.1 % | 📝 To fill |
| MenuLayer (CLI) | Shared | `tests/test_cli.cpp`, `tests/test_main.cpp` | — | 79.8 % | 72.2 % | 📝 To fill |

---

## 6. Deviations & Open Points

| # | Requirement | Deviation | Impact |
|---|---|---|---|
| 1 | BMS-F-061 (full) | No start-up journal replay. `reconcile()` reports mismatches but nothing replays the journal into stored balances at process start. `TC-REL-01` (kill mid-write, restart, verify) is therefore **not met**. | Data integrity relies on external reconciliation runs; no automatic recovery. |
| 2 | BMS-F-063 | Audit record written *after* report computation. If process crashes between report and audit write, the report ran but left no trace. Mirrors the AuthService unlock flow; accepted for v1.0. | Low — manager can re-run; audit gap is visible in next reconciliation. |
| 3 | BMS-F-041 | Mini-statement reads entire journal sequentially. For very large journals this could be slow; no index. | Performance — not a functional gap for v1.0 scope. |
| 4 | BMS-SR-005 | Append-only by design; no update/delete API. Corrupt records are detected but not repaired. | Operational — requires manual intervention if corruption occurs. |

---

## 7. Sign-off

| Role | Name | Date | Status |
|---|---|---|---|
| **Owner (P3)** | Adarsha E | 12-11-2026 | ✅ Complete |
| **Reviewer (P1)** | Dhanush S | — | Pending |
| **Reviewer (P2)** | Vidit S | — | Pending |

---

*Generated from `docs/test-reports/Unit_Test_Report.md`. Re-run `tools/coverage.sh` to refresh coverage figures.*