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

_Ledger, Report - P3 Adarsha, and MenuLayer (CLI) / `main.cpp` - shared: to be added by their owners._

### TransactionModule, Money type, PersistenceModule, Platform layer (P2 Vidit)

The tables above are for `main` at `448ecb9`, which includes PR #24: Sprint 3
stories 3.3, 5.1, 5.2 and 7.5 and the `counters.dat` fix. They already count
that code. The last part of this section compares them with `297bf67`, the
commit before PR #24.

**TransactionModule - 93.7 % (133 / 142).** Every decision in the money rules
has both outcomes tested: Savings minimum (below / exactly at / above), daily
limit (inside / exactly at / over / next bank day), per-transfer and daily
transfer ceilings, same account, closed, locked and unknown account, overflow,
insufficient funds, and failure between the two transfer legs (both legs
tried). The 9 untaken branches are halves of compound conditions:

- the `used <= limit` half of the limit check (false only if the limit is
  lowered below what an account already used today);
- the journal-append failure at transfer line 281 (the credit leg failing
  after the debit leg succeeded);
- the `deposit_ceiling_ <= 0` guard (a misconfigured ceiling);
- sub-conditions of the bank-date calculation. Which side of those runs
  depends on the day the tests run (January-February, years before 1970), so
  the figure can move by a branch or two without any code change.

**Money type - 88.9 % (16 / 18).** The two untaken branches are the underflow
operands of `operator+` (line 27) and `operator-` (line 38), the
`paise_ < min - other` direction. The overflow direction is tested.

**PersistenceModule - 77.7 % (443 / 570).** `file_account_store.cpp` 73.4 %
(218 / 297), `file_credential_store.cpp` 77.4 % (130 / 168), `append_file.cpp`
87.2 % (34 / 39), `record_types.h` 91.7 % (55 / 60), `crc32.h` 100 %. The
untaken side of most branches is "the stream failed"
(`if (!file) return Error::Io;`), which needs fault injection that does not
exist yet. Branches that decide whether *data is valid* (CRC mismatch, wrong
record size, duplicate key, zero ID, damaged counters file) are taken both
ways. The five untaken branches in `record_types.h` are operands of the
account type and state check in `serializeAccount`, which can only fail for a
value that is not a valid enum member.

**Platform layer - 71.1 % (54 / 76).** The untaken sides are "the system call
failed": `/dev/urandom` cannot be opened or read, `stat` after `mkdir`, the
lock file cannot be opened, `fsync` or `rename` fails; plus the terminal
branch of `readLineNoEcho`, which needs a real terminal. The functional
branches are all tested: folder created owner-only, group- or world-writable
folder refused, second instance refused, lock released, atomic replace, and
echo handling when input is piped.

#### What PR #24 changed (same toolchain, `297bf67` before, `448ecb9` after)

| Module | Branches before | Branches after |
|---|---|---|
| TransactionModule | 42 / 51 (82.4 %) | 133 / 142 (**93.7 %**) |
| Money type | 16 / 18 (88.9 %) | 16 / 18 (88.9 %) |
| PersistenceModule | 418 / 560 (74.6 %) | 443 / 570 (**77.7 %**) |
| Platform layer | 7 / 23 (30.4 %) | 54 / 76 (**71.1 %**) |

Platform was the lowest figure in the project because `readLineNoEcho` never
ran and the folder, lock and replace code did not exist yet.

Remaining gaps and what would close them:

| Priority | Action | Effect |
|---|---|---|
| 1 | Fault-injection seam for file open / write | Most of Persistence's ~127 untaken branches |
| 2 | Ledger that fails on its Nth append (test double) | Last transfer branch |
| 3 | `Money` underflow test (`Money(min) - Money(1)`) | +2 branches in Money |
| 4 | Pseudo-terminal test for `readLineNoEcho` (Linux only) | Platform terminal branch |

The full list of lines with untaken branches per file is in
`branch_coverage.txt`.
