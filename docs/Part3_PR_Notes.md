# PR notes — Part 3 (Vidit): Transaction rules, storage fix, file security

Branch suggestion: `feat/part3-transaction-rules`. Serves stories 3.2 (daily limit), 3.3, 5.1, 5.2, 7.5 and defect "storage bug". Jira: BMS-19, BMS-24, BMS-25, BMS-34.

## What changed

| Area | Change | Req. |
|---|---|---|
| `transaction_service` | `TransactionLimits` (Savings minimum, daily withdrawal limit, per-transfer ceiling, daily transfer ceiling); `withdraw()` enforces minimum balance and daily limit; new `transfer()` with reversing journal entries if a leg fails; daily counters restart on a new bank day (IST) | BMS-F-030/032/050/051/052 |
| `core/error.h` | New codes appended (not renumbered): `BelowMinimumBalance`, `DailyLimitExceeded`, `TransferCeilingExceeded`, `SameAccount`, `InstanceLocked`, `InsecureDataDirectory` | BMS-F-051, SR-006 |
| `platform` | `secureDataDirectory()` (owner-only, refuses group/world-writable folder), `InstanceLock`, `replaceFileDurably()` — POSIX and Windows versions | BMS-SR-006, NF-002 |
| `cli` | Teller option 9 and customer option 5 "Transfer"; plain-language messages for the new refusals; `main.cpp` wires limits, folder check and lock | BMS-NF-005 |
| `file_account_store` | **Storage bug fix** (below) | BMS-NF-002 |
| tests | 37 new test cases (125 → 162), one existing CLI test edited (see below) | — |
| docs / tools | `Coverage_Other_Modules.md`, `Test_Cases_DEP_WDR_TRF.md`, `tools/coverage_other_modules.sh` | — |

Defaults are "no limit", so existing callers and tests behave as before; only `main.cpp` passes real limits.

## The storage bug

`FileAccountStore::saveCounters()` opened `counters.dat` with `trunc` and wrote it in place. A crash or power cut between the truncate and the write left a short or empty file, and `loadCounters()` then returned `CorruptRecord`, so **the store refused to start and the bank could not be opened at all**, even though customers.dat and accounts.dat were fine. Fix: (1) write `counters.dat.tmp`, `fsync`, then atomically rename it over the old file; (2) if `counters.dat` is nevertheless the wrong size or fails its CRC, rebuild it from the account/customer files, which `scanExistingFiles()` has already read. A real tear in customers.dat or accounts.dat is still refused. An issued ID is never reused (tested). I could not tell from the chat which defect Dhanush meant; if it is a different one, this is still a genuine bug of the same family.

## Edited existing test

`tests/test_cli.cpp` "TC-AUT-03": it used teller option `9` as its "invalid option" example. Option 9 now exists, so it uses `10`.

## Needs a decision from the team

See "Deviations and open points" in `Test_Cases_DEP_WDR_TRF.md`. In short: no start-up replay after a crash between journal and balance write (NF-002 not fully met); SRS says one shared daily counter but the file layout has two; transfer ownership is checked in the menu, not the service (SR-007); limit values are proposals; Windows code not yet compiled.

## How to check

```bash
cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build --output-on-failure
bash tools/check_banned.sh
bash tools/coverage_other_modules.sh        # needs: pip install gcovr
```
Manual: start two copies of `./build/bms` in the same folder (second one must refuse); `chmod 777 data` (must refuse with a chmod hint).
