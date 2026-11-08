# Part 3 — Coverage analysis: other modules

**Author:** Vidit Soni (PES1UG24AM318) · **Team 11 — Bank Management System**
**Scope:** Transaction, Persistence, Platform, Core and CLI. Account/Auth/Validation are in Dhanush's section; Ledger/Report are in Adarsha's.
Paste Part A into the *code (line) coverage* report and Part B into the *branch coverage* report, under "other modules".

## How this was measured

| | |
|---|---|
| Build | `g++` (C++17), `-O0 --coverage`, Debug, doctest 2.4.11, **162 test cases / 1957 assertions, all passing** |
| Tool | `gcov` via `gcovr` 8.8 — reproduce with `bash tools/coverage_other_modules.sh` |
| Branches | Counted **excluding** compiler-generated exception-path branches and unreachable branches (`--exclude-throw-branches --exclude-unreachable-branches`). Raw gcov counts those too and shows ~59 % overall, which says more about the C++ standard library than about our tests. |
| Not measured | `platform_win.cpp` (only compiled on the Windows CI job; this run was Linux). `core/version.cpp` has no executable lines worth counting. |

---

## Part A — Code (line) coverage, other modules

| Module | File | Lines covered | Line % |
|---|---|---|---|
| Transaction | `transaction/transaction_service.cpp` | 167 / 169 | **98.8 %** |
| Persistence | `persistence/append_file.cpp` | 30 / 33 | 90.9 % |
| | `persistence/crc32.h` | 9 / 9 | 100 % |
| | `persistence/file_account_store.cpp` | 263 / 336 | 78.3 % |
| | `persistence/file_credential_store.cpp` | 149 / 178 | 83.7 % |
| | `persistence/record_types.h` | 189 / 190 | 99.5 % |
| | **Persistence total** | **640 / 746** | **85.8 %** |
| Platform | `platform/platform_posix.cpp` | 68 / 82 | **82.9 %** |
| Core | `core/money.h` | 21 / 21 | **100 %** |
| CLI | `cli/menu.cpp` | 503 / 607 | 82.9 % |
| | `cli/in_memory_stores.h` | 51 / 55 | 92.7 % |
| | `cli/main.cpp` | 0 / 30 | 0 % |
| | **CLI total** | **554 / 692** | **80.1 %** |
| | **All modules in this section** | **1450 / 1710** | **84.8 %** |

### Analysis

**Transaction (98.8 %).** The two uncovered lines are `transaction_service.cpp` 281–282: the case where the *second journal append of a transfer fails after the first succeeded*, which triggers the reversing entry. The sibling paths (store write fails on leg 1 or leg 2) are tested in `TC-TRF-01`; this one needs a ledger that fails on its Nth append, which the current `Ledger` class cannot be told to do. Risk: low, the code is eight lines and mirrors the tested path. Action: add a failure-injecting ledger seam if time allows.

**Persistence (85.8 %).** Nearly every uncovered line is a `return Error::Io;` after a failed open/write/read in `file_account_store.cpp`, `file_credential_store.cpp` or `append_file.cpp`. They only run when the OS refuses a write (disk full, read-only folder, permission denied). They are not covered because the tests would need fault injection that works on both Linux and Windows CI. The Test Plan already lists "Corrupt record or full disk" as a risk (§10), so this is the known gap, not a surprise. Corrupt-data paths are covered much better: bad CRC, truncated file, duplicate IDs, zero IDs, bad enum values. `record_types.h` rose to 99.5 % after we added a test that round-trips `Locked` and `Closed` accounts and the daily counters through the file. Before that, a Locked account had never been written to disk by any test.

**Platform (82.9 %).** Uncovered: the failure returns of `secureRandom` (`/dev/urandom` unreadable, EINTR, short read), the *terminal* branch of `readLineNoEcho` (needs a real TTY; CI has none, so only the piped-input branch is tested), a lock file that cannot be opened, and `fsync`/`rename` failure inside `replaceFileDurably`. All are OS-failure paths. The new owner-only-permission check, instance lock and atomic replace are covered on their success and refusal paths.

**Core (100 %).** `money.h` is fully executed. Observation: `TransactionService` does its own overflow-checked arithmetic on `Paise` (`int64_t`) and does not use the `Money` class, so `Money` is tested but not on the real money path. BMS-NF-003/SR-003 are met by the checked arithmetic in the service, not by `Money`.

**CLI (80.1 %).** `main.cpp` is 0 % by nature: it is the program entry point and unit tests do not start `main`. It is exercised only by the CI step "Run application", plus the manual smoke test noted in the PR (second instance refused; loosened folder refused). `menu.cpp` misses mostly rarely-seen error messages and manager-menu paths that Adarsha's 7.6 tests can extend.

---

## Part B — Branch coverage, other modules

| Module | File | Branches taken | Branch % |
|---|---|---|---|
| Transaction | `transaction/transaction_service.cpp` | 133 / 142 | **93.7 %** |
| Persistence | `persistence/append_file.cpp` | 34 / 39 | 87.2 % |
| | `persistence/crc32.h` | 6 / 6 | 100 % |
| | `persistence/file_account_store.cpp` | 217 / 296 | 73.3 % |
| | `persistence/file_credential_store.cpp` | 129 / 167 | 77.2 % |
| | `persistence/record_types.h` | 55 / 60 | 91.7 % |
| | **Persistence total** | **441 / 568** | **77.6 %** |
| Platform | `platform/platform_posix.cpp` | 54 / 76 | **71.1 %** |
| Core | `core/money.h` | 16 / 18 | **88.9 %** |
| CLI | `cli/menu.cpp` | 564 / 749 | 75.3 % |
| | `cli/in_memory_stores.h` | 23 / 29 | 79.3 % |
| | `cli/main.cpp` | 0 / 36 | 0 % |
| | **CLI total** | **587 / 814** | **72.1 %** |
| | **All modules in this section** | **1231 / 1618** | **76.1 %** |

### Analysis

**Transaction (93.7 %, 9 branches not taken).** All business rules have both outcomes tested: minimum balance (below / exactly at / above), daily limit (inside / exactly at / over / next day), transfer ceilings, same account, closed, locked, unknown account, overflow, insufficient funds. What is left are *halves of compound conditions*: (1) the `used <= limit` half of the limit check, which is only false if an operator lowers the limit below what an account already used today; (2) the line-281 reversal described in Part A; (3) a guard against a configured deposit ceiling of zero or less; (4) sub-conditions in the date calculation. In the date code, which side runs depends on the day the tests run (months January–February, dates before 1970). So this number can move by a branch or two between runs without any code change.

**Persistence (77.6 %).** Same cause as Part A: each `if (!file) return Error::Io;` is a branch with an untaken side. In `file_account_store.cpp` (73.3 %) and `file_credential_store.cpp` (77.2 %) those I/O-failure sides are the bulk of the 127 missing branches. Branches that decide *data validity* (CRC mismatch, wrong record size, duplicate key, zero or maximum ID) are mostly taken both ways. The scan guards for an ID equal to the type's maximum value need a hand-built data file with ID 4,294,967,295 to reach; they are low priority.

**Platform (71.1 %).** Lowest in the section, for the reason in Part A: this layer is a thin wrapper over system calls, and most of its branches are "the system call failed". The functional branches (folder owner-only, group/world-writable folder refused, second lock refused, lock freed after release, atomic replace, no-echo skipped when input is piped) are all tested.

**Core (88.9 %).** The two missing branches are the *underflow* side of `Money::operator-` / `operator+` (`paise_ < min - other`). Overflow (the upward side) is tested. Cheap to close: add `Money(min) - Money(1)` to `test_money.cpp`.

**CLI (72.1 %).** `main.cpp` is 0 % (Part A). In `menu.cpp` the `describe()` switch and the manager and customer menu prompts hold most of the untaken branches. The new transfer flow is covered through both the teller menu and the customer menu, and the new limit messages are asserted by text.

### What would raise the numbers

| Priority | Action | Effect |
|---|---|---|
| 1 | Fault-injection seam for file open/write (e.g. inject a failing stream factory) | Closes most of Persistence's remaining ~130 lines/branches |
| 2 | Failing-Nth-append ledger for tests | Closes Transaction's last 2 lines |
| 3 | `Money` underflow test | +2 branches in Core |
| 4 | Pseudo-terminal test for `readLineNoEcho` (Linux only) | Platform terminal branch |
