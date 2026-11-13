# System Validation Report — Sprint 3 (Stories 6.1, 6.2, 6.3)

**Project:** Team 11 — Bank Management System  
**Sprint:** Sprint 3 (Part 3)  
**Stories validated:** 6.1 (Append-only journal & audit), 6.2 (Reconciliation), 6.3 (End-of-day report)  
**Owner:** Adarsha E (PES1UG24AM334) — P3  
**Date:** 12-11-2026  
**Baseline:** SRS v1.0, SAD v1.0, Test Plan v1.0  
**Commit validated:** `448ecb9` on `main` (after PRs #17–#24)  

---

## 1. Validation Scope

| Story | SRS Requirements | Module | Source files | Test file |
|---|---|---|---|---|
| 6.1 | BMS-F-060, BMS-F-063, BMS-SR-005 | LedgerModule | `src/ledger/ledger.cpp/h` | `tests/test_ledger.cpp` |
| 6.2 | BMS-F-061, BMS-SR-003 | LedgerModule | `src/ledger/ledger.cpp/h` | `tests/test_ledger.cpp` |
| 6.3 | BMS-F-062, BMS-F-012, BMS-F-063 | ReportModule | `src/report/report_service.cpp/h` | `tests/test_report.cpp` |

**Cross-cutting:** BMS-NF-006 (single writer per file pair), BMS-NF-002 (storage fault tolerance — partial).

---

## 2. Requirements Coverage Matrix

| SRS ID | Description | Test Cases | Status | Notes |
|---|---|---|---|---|
| **BMS-F-060** | Append-only journal & audit capture every transaction & privileged action | TC-LED-01, TC-LED-04, TC-SEC-06 | ✅ Validated | All transaction types (OPEN, DEPOSIT, WITHDRAW, TRANSFER_OUT, TRANSFER_IN) and all 8 audit actions round-trip; sequences persist across restart. |
| **BMS-F-063** | Every privileged action writes an audit record (operator, action, target, details, timestamp) | TC-LED-04, TC-LED-03 (report run) | ✅ Validated | 8 audit actions tested; REPORT_RUN written by ReportService; refused operations write no audit. |
| **BMS-SR-005** | Append-only — no update/delete; corrupt/unterminated files detected | TC-SEC-06 | ✅ Validated | Strict parsing rejects malformed lines; corrupt files refuse reads & further appends; missing files treated as empty. |
| **BMS-F-061** | Reconciliation verifies every account balance = opening + journal entries; names mismatches | TC-LED-02 | ✅ Validated | Clean data passes; tampered stored balances named; edited journal amounts detected; missing OPEN / duplicate OPEN caught; integer limits handled without wrap. |
| **BMS-SR-003** | Money arithmetic covers integer limits without wraparound | TC-SEC-03 | ✅ Validated | `Paise::max()` and `Paise::min()` round-trip; one paise beyond max detected as failure. |
| **BMS-F-041** | Mini-statement: up to 10 newest journal records for one account | TC-BAL-02 | ✅ Validated | Returns up to 10, filtered by account, descending sequence; empty journal clears output; account 0 rejected; corruption propagated. |
| **BMS-F-062** | End-of-day report: totals by type + closing balance of every account in journal; IST day boundary | TC-LED-03 | ✅ Validated | Deposits, withdrawals, transfer-out, transfer-in totalled separately; every account with history closes; empty day totals zero; day label uses IST midnight. |
| **BMS-F-012** | Role-based access: only Manager may run end-of-day report | TC-AUT-03, TC-SEC-08 | ✅ Validated | Teller → NotAuthorised; Customer → NotAuthorised; empty operator → NotAuthorised; Manager → Ok. |
| **BMS-NF-006** | Single writer owns each (journal, audit) file pair | Architecture | ✅ Validated | Ledger constructor recovers independent sequences; no concurrent writers in design. |

---

## 3. Test Execution Results

| Test Suite | Test Cases | Assertions | Pass | Fail | Duration |
|---|---|---|---|---|---|
| Full suite (`bms_tests`) | 163 | 1966 | 163 | 0 | ~1.2 s |
| LedgerModule (`TC-LED*`, `TC-SEC-03*`, `TC-SEC-06*`) | 25 | ~700 | 25 | 0 | ~0.4 s |
| ReportModule (`TC-LED-03*`, `TC-LED-04*`, `TC-AUT-03*`) | 8 | 84 | 8 | 0 | ~0.1 s |

**CI status:** All GitHub Actions workflows pass (`Build and test` job).

---

## 4. Code Coverage Validation

| Module | Line Coverage | Branch Coverage | Function Coverage | Target | Status |
|---|---|---|---|---|---|
| LedgerModule | **97.7 %** | **96.5 %** | **100 %** | ≥ 90 % | ✅ Exceeds |
| ReportModule | **92.9 %** | **84.0 %** | **100 %** | ≥ 90 % (lines) | ✅ Lines OK / Branches accepted gap |

**Gap justification (ReportModule branches 84 %):** The 8 untaken branches are all IO-failure paths (`readJournal` error, `appendAudit` InvalidInput/Io, `journalDay` negative timestamp) that require fault-injection or unrealistic timestamps. Same gap class as PersistenceModule and AuthModule — accepted for v1.0.

---

## 5. P1 / P2 Module Coverage Summary (for reference)

| Module | Owner | Line Coverage | Branch Coverage | Function Coverage |
|---|---|---|---|---|
| AccountModule | P1 Dhanush | 93.0 % | 90.9 % | 100 % |
| AuthModule | P1 Dhanush | 93.5 % | 85.6 % | 100 % |
| ValidationModule | P1 Dhanush | 98.7 % | 90.8 % | 100 % |
| TransactionModule | P2 Vidit | 98.8 % | 93.7 % | 100 % |
| Money type | P2 Vidit | 100.0 % | 88.9 % | 100 % |
| PersistenceModule | P2 Vidit | 86.0 % | 77.7 % | 100 % |
| Platform layer | P2 Vidit | 82.9 % | 71.1 % | 100 % |
| MenuLayer (CLI) | Shared | 79.8 % | 72.2 % | 100 % |

*Detailed analysis for P1/P2 modules is in the coverage reports; their owners should add validation sections here.*

---

## 6. Static Analysis & Code Quality

| Check | Tool | Result |
|---|---|---|
| Compilation warnings | `g++ -Wall -Wextra -Werror -pedantic` | **Clean** (0 warnings, 0 errors) |
| Banned functions | `tools/check_banned.sh` | **Clean** (no `gets`, `strcpy`, `strcat`, `sprintf`, unbounded `scanf`) |
| C++ standard | C++17 | Conforms |

---

## 7. Deviations & Open Points

| # | Requirement | Deviation | Severity | Resolution Plan |
|---|---|---|---|---|
| 1 | BMS-F-061 (full) | No start-up journal replay. `reconcile()` reports mismatches but nothing replays journal into stored balances at process start. `TC-REL-01` (kill mid-write, restart, verify) **not met**. | Medium | Post-v1.0: implement start-up replay using `last_seq` field. |
| 2 | BMS-F-063 (audit timing) | Audit record written *after* report computation. Crash between report and audit → report ran without trace. | Low | Mirrors AuthService unlock flow; accepted for v1.0. |
| 3 | BMS-F-041 (performance) | Mini-statement reads entire journal sequentially. No index for large journals. | Low | Not in v1.0 scope; index can be added if needed. |
| 4 | BMS-SR-005 (repair) | Corrupt records detected but not repaired. No automatic recovery. | Medium | Operational runbook: manual intervention + reconciliation. |
| 5 | BMS-NF-002 (storage) | Fault-injection for IO failures not implemented; IO-failure branches uncovered in Ledger, Report, Persistence, Auth. | Low | Project-wide improvement: add fault-injection seam. |

---

## 8. Traceability Summary

| Artifact | Location | Status |
|---|---|---|
| SRS v1.0 | `docs/SRS.md` | Baseline |
| SAD v1.0 | `docs/SAD_Traceability.md` | **Unchanged this PR** |
| Test Plan | `docs/Test_Plan.md` | **Unchanged this PR** |
| Test Cases (TC-LED, TC-BAL, TC-REP) | `docs/Test_Cases_LED_BAL_REP.md` | **Created this sprint** |
| Unit Test Report | `docs/test-reports/Unit_Test_Report.md` | **Created this sprint** |
| Code Coverage Report | `docs/test-reports/Code_Coverage_Report.md` | **Updated this sprint** |
| Branch Coverage Report | `docs/test-reports/Branch_Coverage_Report.md` | **Updated this sprint** |

---

## 9. Sign-off

| Role | Name | Date | Decision |
|---|---|---|---|
| **Module Owner (P3)** | Adarsha E | 12-11-2026 | ✅ Accepted |
| **Tech Lead (P1)** | Dhanush S | — | Pending review |
| **Tech Lead (P2)** | Vidit S | — | Pending review |
| **QA / Scrum Master** | — | — | Pending |

---

*This report validates that Sprint 3 Stories 6.1, 6.2, 6.3 meet their acceptance criteria per SRS v1.0. All automated tests pass. Coverage exceeds targets for lines/functions; branch gap is documented and accepted.*