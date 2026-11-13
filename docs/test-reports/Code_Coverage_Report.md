# Code Coverage Report

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

## Summary

| Metric | Covered | Total | Coverage |
|---|---|---|---|
| Lines (statements) | 2192 | 2491 | **88.0%** |
| Functions | 201 | 202 | **99.5%** |

## By module

| Module | Path | Owner | Lines | Executed | Coverage |
|---|---|---|---|---|---|
| AccountModule | `src/account/` | P1 Dhanush | 100 | 93 | 93.0% |
| AuthModule | `src/auth/` | P1 Dhanush | 261 | 244 | 93.5% |
| ValidationModule | `src/validation/` | P1 Dhanush | 79 | 78 | 98.7% |
| MenuLayer (CLI) | `src/cli/` | Shared | 702 | 560 | 79.8% |
| TransactionModule | `src/transaction/` | P2 Vidit | 169 | 167 | 98.8% |
| Money type | `src/core/` | P2 Vidit | 21 | 21 | 100.0% |
| PersistenceModule | `src/persistence/` | P2 Vidit | 758 | 652 | 86.0% |
| Platform layer | `src/platform/` | P2 Vidit | 82 | 68 | 82.9% |
| LedgerModule | `src/ledger/` | P3 Adarsha | 263 | 257 | 97.7% |
| ReportModule | `src/report/` | P3 Adarsha | 56 | 52 | 92.9% |

## By file

| File | Lines | Executed | Coverage |
|---|---|---|---|
| `src/account/account_service.cpp` | 97 | 90 | 92.8% |
| `src/account/account_service.h` | 2 | 2 | 100.0% |
| `src/account/account_store.h` | 1 | 1 | 100.0% |
| `src/auth/auth.h` | 3 | 3 | 100.0% |
| `src/auth/auth_service.cpp` | 135 | 119 | 88.1% |
| `src/auth/authorization.cpp` | 11 | 10 | 90.9% |
| `src/auth/crypto.cpp` | 112 | 112 | 100.0% |
| `src/cli/in_memory_stores.h` | 55 | 51 | 92.7% |
| `src/cli/main.cpp` | 30 | 0 | 0.0% |
| `src/cli/menu.cpp` | 617 | 509 | 82.5% |
| `src/core/money.h` | 21 | 21 | 100.0% |
| `src/ledger/ledger.cpp` | 263 | 257 | 97.7% |
| `src/persistence/append_file.cpp` | 33 | 30 | 90.9% |
| `src/persistence/crc32.h` | 9 | 9 | 100.0% |
| `src/persistence/file_account_store.cpp` | 337 | 264 | 78.3% |
| `src/persistence/file_account_store.h` | 1 | 1 | 100.0% |
| `src/persistence/file_credential_store.cpp` | 179 | 150 | 83.8% |
| `src/persistence/file_credential_store.h` | 1 | 1 | 100.0% |
| `src/persistence/record_types.h` | 198 | 197 | 99.5% |
| `src/platform/platform_posix.cpp` | 82 | 68 | 82.9% |
| `src/report/report_service.cpp` | 56 | 52 | 92.9% |
| `src/transaction/transaction_service.cpp` | 169 | 167 | 98.8% |
| `src/validation/validation.cpp` | 79 | 78 | 98.7% |

## Analysis - Account, Auth and Validation modules (P1 Dhanush)

All three modules are above 92% line coverage. Every uncovered line falls into
one of three groups:

- **Storage-failure returns** (`account_service.cpp` lines 24, 27, 56, 71, 76,
  118, 146 and most of the listed `auth_service.cpp` lines): the
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

### LedgerModule (P3 Adarsha)

**97.7 % line coverage (257 / 263), 96.5 % branch coverage (329 / 341).**

The 6 uncovered lines and ~12 uncovered branches are all **IO-failure paths** in `ledger.cpp`:

- `appendLine` failure returns (disk full, unwritable directory) — the in-memory test fixtures and temporary directories never trigger these.
- `readLines` failure in the fast-path of `readJournal`/`readAudit` — same reason.
- The `journal_status_` / `audit_status_` latch branches after a failed append — never reached because appends never fail in tests.

Every business decision is covered: every transaction type (OPEN, DEPOSIT, WITHDRAW, TRANSFER_OUT, TRANSFER_IN) and every audit action round-trips; mini-statement filtering, limiting, and error propagation; reconciliation logic for clean data, tampered balances, edited journals, missing/duplicate OPEN, integer limits; escape handling for reserved characters; corrupt-file detection; sequence recovery across restart.

Requirements covered: TC-LED-01 (BMS-F-060), TC-LED-02 (BMS-F-061), TC-LED-03 (BMS-F-041), TC-LED-04 (BMS-F-063), TC-SEC-03 (BMS-SR-003), TC-SEC-06 (BMS-SR-005), TC-BAL-02 (BMS-F-041).

Gap accepted — fault-injection for `appendLine`/`readLines` is a project-wide improvement tracked in PersistenceModule.

### ReportModule (P3 Adarsha)

**92.9 % line coverage (52 / 56), 84.0 % branch coverage (42 / 50).**

The 4 uncovered lines and 8 uncovered branches in `report_service.cpp` are:

- `readJournal` error return (lines 55–57) — the test Ledger never fails a read.
- `journalDay` negative-timestamp branch (lines 41–42) — tests use a positive fixed Unix time.
- `appendAudit` `InvalidInput` and `Io` returns (lines 92–95) — the test Ledger's audit append never fails.

Every business decision is covered: IST day boundary (`journalDay`); totals by type (deposit, withdrawal, transfer-out, transfer-in) for the report day only; closing balance of every account with journal history; empty day / empty journal handling; Manager-only authorization (Teller, Customer, empty operator refused); exactly one REPORT_RUN audit record per successful run; zero audit on refused run; corrupt journal reported and refused.

Requirements covered: TC-LED-03 (BMS-F-062), TC-LED-04 (BMS-F-063), TC-AUT-03 / TC-SEC-08 (BMS-F-012).

Gap accepted — same IO-failure class as LedgerModule and PersistenceModule; `journalDay` negative branch unreachable with real timestamps.

### MenuLayer (CLI) / `main.cpp` - shared: to be added by their owners.

### TransactionModule, Money type, PersistenceModule, Platform layer (P2 Vidit)

The tables above are for `main` at `448ecb9`, which includes PR #24: Sprint 3
stories 3.3, 5.1, 5.2 and 7.5 (Savings minimum, daily limit, transfer,
ceilings, owner-only data folder, instance lock) and the `counters.dat` fix.
They already count all of that code. The last part of this section compares
them with `297bf67`, the commit before PR #24.

**TransactionModule - 98.8 % (167 / 169).** The only uncovered lines are 281
and 282 of `transaction_service.cpp`: in `transfer()`, the second journal
append (the credit leg) failing after the first (the debit leg) succeeded,
which triggers `reverse_out()`. The same reversal after a *store* write
failure is tested for both legs (TC-TRF-01). Reaching these two lines needs a
ledger that fails on its Nth append, and `Ledger` cannot be told to do that.
The risk is low, because the code mirrors the tested path. Covered by TC-DEP-01..04,
TC-WDR-01..05 and TC-TRF-01..03.

**Money type - 100 %.** Every line of `money.h` runs. The two untaken branches
are the *underflow* side of `operator+` and `operator-` (lines 27 and 38). The
service does its own overflow-checked arithmetic on `Paise` and does not use
`Money`, so `Money` is tested but is not on the real money path.

**PersistenceModule - 86.0 % (652 / 758).** `file_account_store.cpp` 78.3 %
(264 / 337), `file_credential_store.cpp` 83.8 % (150 / 179), `append_file.cpp`
90.9 % (30 / 33), `record_types.h` 99.5 % (197 / 198); `crc32.h` and both
headers are 100 %.

- The uncovered lines are almost all `return Error::Io;` after a failed open,
  read or write (disk full, read-only folder), for example `append_file.cpp`
  lines 18, 29 and 36. Faking those failures portably on Linux and Windows CI
  needs a fault-injection seam that does not exist yet, so they are an
  accepted gap.
- Damaged-data paths are covered well: bad CRC, truncated file, duplicate or
  zero IDs, invalid enum values (TC-SEC-05 area), and a damaged
  `counters.dat`, which is now rebuilt from the data files.
- The one uncovered line in `record_types.h` (117) is the `InvalidInput`
  return for an account whose type or state is not a valid enum value, which
  can only happen through a cast.
- **Defect fixed in PR #24:** `FileAccountStore::saveCounters()` rewrote
  `counters.dat` in place. A crash mid-write left a short file, after which
  the store refused to start (`CorruptRecord`) although `customers.dat` and
  `accounts.dat` were intact. It now writes a temporary file and swaps it in,
  and rebuilds the counters if the file is damaged. `record_types.h` also went
  from 96.0 % to 99.5 % once a test stored Locked and Closed accounts on disk.

**Platform layer - 82.9 % (68 / 82).** Uncovered lines of `platform_posix.cpp`:
20, 26, 29-30 (`secureRandom`: `/dev/urandom` cannot be opened, is
interrupted, or returns too little); 43-45 and 49-50 (the terminal branch of
`readLineNoEcho`, which switches echo off and on and needs a real terminal,
while CI has none); 74-75 (`stat` fails right after the data folder was
created); 102 (the lock file cannot be opened); 120 (`fsync` or `rename`
fails in `replaceFileDurably`); and 125 (the target has no folder part). All
are "the system call failed" or "needs a terminal" paths. The new owner-only
folder check, the instance lock and the atomic replace are covered on their
success and refusal paths (TC-SEC-07).

#### What PR #24 changed (same toolchain, `297bf67` before, `448ecb9` after)

| Module | Lines before | Lines after | Change |
|---|---|---|---|
| TransactionModule | 56 / 63 (88.9 %) | 167 / 169 (**98.8 %**) | daily limit, Savings minimum, transfer and ceilings added and tested |
| Money type | 21 / 21 (100 %) | 21 / 21 (100 %) | none |
| PersistenceModule | 632 / 749 (84.4 %) | 652 / 758 (**86.0 %**) | counters recovery added; Locked and Closed states now stored by a test |
| Platform layer | 16 / 32 (50.0 %) | 68 / 82 (**82.9 %**) | owner-only folder, instance lock and atomic replace added and tested |

Before PR #24 every uncovered line in TransactionModule was an error return,
and TC-WDR-01, TC-WDR-02 and TC-TRF-01..03 had no code to run. In the Platform
layer `readLineNoEcho` had never run. The suite grew from 125 to 163 test
cases.

Requirements covered by the tests added in PR #24: TC-DEP-01..04,
TC-WDR-01..05, TC-TRF-01..03, TC-SEC-07 and the `counters.dat` recovery
(BMS-NF-002). See `docs/Test_Cases_DEP_WDR_TRF.md`.

The full list of uncovered line numbers per file is in `code_coverage.txt`.