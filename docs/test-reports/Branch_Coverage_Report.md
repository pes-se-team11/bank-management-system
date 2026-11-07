# Branch Coverage Report

**Project:** Team 11 - Bank Management System (C/C++)
**Commit measured:** `297bf67` on `main` (after PRs #17-#22)
**Date:** 07-11-2026
**Tools:** g++ (GCC) 14.4.0, gcovr 7.2, cmake version 3.31.6 (Linux, `gcc:14` container)
**Test run:** 125/125 test cases passed, 1705/1705 assertions passed, 0 failed

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
| Branches | 1778 | 2236 | **79.5%** |

## By module

| Module | Path | Owner | Branches | Taken | Coverage |
|---|---|---|---|---|---|
| AccountModule | `src/account/` | P1 Dhanush | 95 | 86 | 90.5% |
| AuthModule | `src/auth/` | P1 Dhanush | 195 | 167 | 85.6% |
| ValidationModule | `src/validation/` | P1 Dhanush | 142 | 129 | 90.8% |
| MenuLayer (CLI) | `src/cli/` | Shared | 761 | 541 | 71.1% |
| TransactionModule | `src/transaction/` | P2 Vidit | 51 | 42 | 82.4% |
| Money type | `src/core/` | P2 Vidit | 18 | 16 | 88.9% |
| PersistenceModule | `src/persistence/` | P2 Vidit | 560 | 418 | 74.6% |
| Platform layer | `src/platform/` | P2 Vidit | 23 | 7 | 30.4% |
| LedgerModule | `src/ledger/` | P3 Adarsha | 341 | 329 | 96.5% |
| ReportModule | `src/report/` | P3 Adarsha | 50 | 43 | 86.0% |

## By file

| File | Branches | Taken | Coverage |
|---|---|---|---|
| `src/account/account_service.cpp` | 95 | 86 | 90.5% |
| `src/auth/auth_service.cpp` | 139 | 112 | 80.6% |
| `src/auth/authorization.cpp` | 9 | 8 | 88.9% |
| `src/auth/crypto.cpp` | 47 | 47 | 100.0% |
| `src/cli/in_memory_stores.h` | 29 | 23 | 79.3% |
| `src/cli/main.cpp` | 34 | 0 | 0.0% |
| `src/cli/menu.cpp` | 698 | 518 | 74.2% |
| `src/core/money.h` | 18 | 16 | 88.9% |
| `src/ledger/ledger.cpp` | 341 | 329 | 96.5% |
| `src/persistence/append_file.cpp` | 39 | 34 | 87.2% |
| `src/persistence/crc32.h` | 6 | 6 | 100.0% |
| `src/persistence/file_account_store.cpp` | 288 | 207 | 71.9% |
| `src/persistence/file_credential_store.cpp` | 167 | 129 | 77.2% |
| `src/persistence/record_types.h` | 60 | 42 | 70.0% |
| `src/platform/platform_posix.cpp` | 23 | 7 | 30.4% |
| `src/report/report_service.cpp` | 50 | 43 | 86.0% |
| `src/transaction/transaction_service.cpp` | 51 | 42 | 82.4% |
| `src/validation/validation.cpp` | 142 | 129 | 90.8% |

## Analysis - Account, Auth and Validation modules (P1 Dhanush)

Account 90%, Auth 82% and Validation 91% of branches are taken. Both outcomes
of every business decision are tested: role allowed / refused (TC-AUT-03,
TC-SEC-08), correct / wrong PIN and lockout on the third failure (TC-AUT-01,
TC-AUT-02), unlock of a locked / unlocked / closed account (TC-AUT-04), closure
at zero / non-zero balance (TC-ACC-04), changed / unchanged customer details
(TC-ACC-03) and valid / invalid input (TC-SEC-04, TC-SEC-05). The untaken
branches are the "store returned an error" side of each storage call - the
same paths explained in the Code Coverage Report - since the in-memory test
stores never fail.

## Analysis - other modules

_To be added by each module owner:_

- _Transaction, Money, Persistence, Platform - P2 Vidit_
- _Ledger, Report - P3 Adarsha_
- _MenuLayer (CLI) and `main.cpp` - shared_

The full list of lines with untaken branches per file is in
`branch_coverage.txt`.
