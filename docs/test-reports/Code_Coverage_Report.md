# Code Coverage Report

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

## Summary

| Metric | Covered | Total | Coverage |
|---|---|---|---|
| Lines (statements) | 1968 | 2277 | **86.4%** |
| Functions | 186 | 189 | **98.4%** |

## By module

| Module | Path | Owner | Lines | Executed | Coverage |
|---|---|---|---|---|---|
| AccountModule | `src/account/` | P1 Dhanush | 98 | 91 | 92.9% |
| AuthModule | `src/auth/` | P1 Dhanush | 261 | 244 | 93.5% |
| ValidationModule | `src/validation/` | P1 Dhanush | 79 | 78 | 98.7% |
| MenuLayer (CLI) | `src/cli/` | Shared | 655 | 521 | 79.5% |
| TransactionModule | `src/transaction/` | P2 Vidit | 63 | 56 | 88.9% |
| Money type | `src/core/` | P2 Vidit | 21 | 21 | 100.0% |
| PersistenceModule | `src/persistence/` | P2 Vidit | 749 | 632 | 84.4% |
| Platform layer | `src/platform/` | P2 Vidit | 32 | 16 | 50.0% |
| LedgerModule | `src/ledger/` | P3 Adarsha | 263 | 257 | 97.7% |
| ReportModule | `src/report/` | P3 Adarsha | 56 | 52 | 92.9% |

## By file

| File | Lines | Executed | Coverage |
|---|---|---|---|
| `src/account/account_service.cpp` | 95 | 88 | 92.6% |
| `src/account/account_service.h` | 2 | 2 | 100.0% |
| `src/account/account_store.h` | 1 | 1 | 100.0% |
| `src/auth/auth.h` | 3 | 3 | 100.0% |
| `src/auth/auth_service.cpp` | 135 | 119 | 88.1% |
| `src/auth/authorization.cpp` | 11 | 10 | 90.9% |
| `src/auth/crypto.cpp` | 112 | 112 | 100.0% |
| `src/cli/in_memory_stores.h` | 55 | 51 | 92.7% |
| `src/cli/main.cpp` | 27 | 0 | 0.0% |
| `src/cli/menu.cpp` | 573 | 470 | 82.0% |
| `src/core/money.h` | 21 | 21 | 100.0% |
| `src/ledger/ledger.cpp` | 263 | 257 | 97.7% |
| `src/persistence/append_file.cpp` | 33 | 30 | 90.9% |
| `src/persistence/crc32.h` | 9 | 9 | 100.0% |
| `src/persistence/file_account_store.cpp` | 329 | 253 | 76.9% |
| `src/persistence/file_account_store.h` | 1 | 0 | 0.0% |
| `src/persistence/file_credential_store.cpp` | 178 | 149 | 83.7% |
| `src/persistence/file_credential_store.h` | 1 | 1 | 100.0% |
| `src/persistence/record_types.h` | 198 | 190 | 96.0% |
| `src/platform/platform_posix.cpp` | 32 | 16 | 50.0% |
| `src/report/report_service.cpp` | 56 | 52 | 92.9% |
| `src/transaction/transaction_service.cpp` | 63 | 56 | 88.9% |
| `src/validation/validation.cpp` | 79 | 78 | 98.7% |

## Analysis - Account, Auth and Validation modules (P1 Dhanush)

All three modules are above 92% line coverage. Every uncovered line falls into
one of three groups:

- **Storage-failure returns** (`account_service.cpp` lines 23, 26, 48, 63, 68,
  110, 138 and most of the listed `auth_service.cpp` lines): the
  `return e;` taken when the account, credential or audit store itself fails
  (disk full, unreadable file). The unit tests use in-memory stores that never
  fail, so these paths are not reached. Each one only passes the store's error
  back unchanged.
- **Secret wiping on an early failure** (`auth_service.cpp` 43, 67, 81): the
  password or PIN is wiped before returning when a store lookup fails; the
  normal wipe paths are covered by TC-SEC-02.
- **Defensive code** (`authorization.cpp` 22, `validation.cpp` 57): a
  `return` after a `switch` that already handles every role (unreachable), and
  a government-ID reference shorter than 4 or longer than 20 characters, which
  the CLI tests do not enter.

Requirements covered for these modules: TC-ACC-01..05, TC-AUT-01..04,
TC-SEC-01, TC-SEC-02, TC-SEC-04, TC-SEC-05, TC-SEC-08.

## Analysis - other modules

_To be added by each module owner:_

- _Transaction, Money, Persistence, Platform - P2 Vidit_
- _Ledger, Report - P3 Adarsha_
- _MenuLayer (CLI) and `main.cpp` - shared_

The full list of uncovered line numbers per file is in `code_coverage.txt`.
