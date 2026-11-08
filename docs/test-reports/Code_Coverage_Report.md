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

_Ledger, Report - P3 Adarsha, and MenuLayer (CLI) / `main.cpp` - shared: to be added by their owners._

### TransactionModule, Money type, PersistenceModule, Platform layer (P2 Vidit)

The figures in the tables above are for `main` at `297bf67`, before Sprint 3
stories 3.3, 5.1, 5.2 and 7.5. Those stories, and a storage fix, are in PR #24
(`feat/part3-transaction-rules`), so each module is analysed twice: what was
uncovered on `main`, and what PR #24 changes.

**TransactionModule - 88.9 % (56 / 63).** The 7 uncovered lines (30, 39, 55,
76, 87, 93, 120 in `transaction_service.cpp`) are all `return` statements on a
rare path:

- *Store or journal failure* (30, 55, 76, 120): `return result;` when reading
  the account or appending to the journal fails. The in-memory test store
  never fails, so these never ran.
- *Overflow guard* (39): a deposit that would pass the largest representable
  balance.
- *Bad stored data* (87, 93): an account with a negative overdraft limit, or
  one already below its allowed floor.

On `main` the module had no daily withdrawal limit, no Savings minimum balance
and no transfer at all, so TC-WDR-01, TC-WDR-02 and TC-TRF-01..03 had no code
to exercise. The tests that did run were TC-DEP-01/02, TC-WDR-03 (overdraft)
and TC-BAL-01..



**Money type - 100 %.** Every line of `money.h` runs. The two untaken branches
are the *underflow* side of `operator+` and `operator-` (lines 27 and 38). The
service does its own overflow-checked arithmetic on `Paise` and does not use
`Money`, so `Money` is tested but is not on the real money path.

**PersistenceModule - 84.4 % (632 / 749).**

- `file_account_store.cpp` 76.9 %, `file_credential_store.cpp` 83.7 %,
  `append_file.cpp` 90.9 %. The uncovered lines are almost all
  `return Error::Io;` after a failed open, read or write (disk full, read-only
  folder). Faking those failures portably on Linux and Windows CI needs a
  fault-injection seam that does not exist yet, so they are an accepted gap.
- Damaged-data paths are covered well: bad CRC, truncated file, duplicate
  or zero IDs, invalid enum values (TC-SEC-05 area).
- `record_types.h` 96.0 %: no test ever stored a **Locked** account on disk, so
  the Locked branch of the account (de)serialiser never ran.
- **Defect found while measuring:** `FileAccountStore::saveCounters()` rewrote
  `counters.dat` in place. A crash mid-write left a short file, after which the
  store refused to start (`CorruptRecord`) although `customers.dat` and
  `accounts.dat` were intact.

**Platform layer - 50.0 % (16 / 32).** Two groups are uncovered. First, the
failure returns of `secureRandom` (`/dev/urandom` cannot be opened or read,
lines 15-26). Second, the whole of `readLineNoEcho` (lines 34-48): the menu
tests feed an `istringstream`, so the real no-echo function never ran, and CI
has no terminal. The layer is a thin wrapper over system calls, so most of its
lines are either the success path (covered through the credential tests) or
"the call failed".
#### What PR #24 changes (measured on `6792701`, g++ 13, gcovr 8.8)

| Module | Lines before | Lines after | Change |
|---|---|---|---|
| TransactionModule | 56 / 63 (88.9 %) | 167 / 169 (**98.8 %**) | 106 lines of new rules, 111 more covered |
| Money type | 21 / 21 (100 %) | 21 / 21 (100 %) | none |
| PersistenceModule | 624 / 741 (84.2 %) | 643 / 749 (**85.8 %**) | +8 lines, +19 covered |
| Platform layer | 16 / 32 (50.0 %) | 68 / 82 (**82.9 %**) | owner-only folder, instance lock, atomic replace added and tested |

"Before" is re-measured with the same toolchain as "after" so the comparison is
fair. The Persistence total in the table above (632 / 749) comes from gcc 14,
which counts 8 more lines in `record_types.h`; the ratio is the same within
0.2 points. The suite grows from 125 to 162 test cases.

What is still uncovered after the PR: two lines in `transaction_service.cpp`
(281-282, the second journal append of a transfer failing after the first
succeeded; the same reversal after a *store* failure is tested); the Io-failure
returns in Persistence described above; and in Platform the failure returns of
`secureRandom`, the terminal branch of `readLineNoEcho` (needs a pseudo-
terminal) and the `fsync` / `rename` failure returns. Re-run
`tools/coverage.sh` after PR #24 merges to refresh every table above.

Requirements covered by the added tests: TC-DEP-01..03, TC-WDR-01..03,
TC-TRF-01..03, TC-SEC-07, and the counters-file recovery (BMS-NF-002). See
`docs/Test_Cases_DEP_WDR_TRF.md`.

The full list of uncovered line numbers per file is in `code_coverage.txt`.