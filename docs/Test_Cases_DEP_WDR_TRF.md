# Test case updates — TC-DEP, TC-WDR, TC-TRF

**Owner of this update:** Vidit Soni (PES1UG24AM318) · **Team 11 — Bank Management System**
**Baseline:** SRS v1.0 requirement IDs and acceptance criteria. **Result column:** automated run on Linux, g++ 13, `-Wall -Wextra -Werror -pedantic`, 162 test cases, 0 failures.
**Where the tests live:** `tests/test_transaction.cpp` (Sprint 2 tests), `tests/test_transaction_rules.cpp` (this update), `tests/test_cli.cpp`, `tests/test_platform.cpp`.
Run one case: `./build/tests/bms_tests -tc="TC-TRF-03*"`

Test levels: **U** = unit (service + in-memory/fake store), **I** = integration (service + real file store), **S** = scripted menu run.

## Deposit

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-DEP-01 | BMS-F-020 | U | ACTIVE Savings account, balance 5,000 paise | Deposit 1,500 | Balance becomes exactly 6,500; call returns Ok | `Deposit increases an active account balance` | Pass |
| TC-DEP-02 | BMS-F-021 | U | ACTIVE account A (5,000); CLOSED account B (5,000); ceiling 1,000,000 | Deposit 0; −5; 1,000,001 into A; 100 into B | Each returns `InvalidInput`; both balances still 5,000; journal empty | `TC-DEP-02: each of the four deposit rejections…` | Pass |
| TC-DEP-03 | BMS-F-022 | U | ACTIVE account (5,000), operator `teller1` | Deposit 700; deposit 300; deposit 0 | Exactly 2 journal records, type DEPOSIT, with account, amount, balance_after (5,700 then 6,000) and operator ID; the refused deposit writes nothing | `TC-DEP-03: exactly one journal record per successful deposit` | Pass |
| TC-DEP-04 *(new, edge)* | BMS-SR-003 | U | Balance = Int64 max − 50 | Deposit 100 | Refused, balance unchanged (no overflow) | `Deposit that would overflow the balance is refused` | Pass |

## Withdrawal

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-WDR-01a | BMS-F-030 | U | Savings, balance 50,000; daily limit 10,000 | Withdraw 6,000; then 5,000; then 4,000; then 1 | 6,000 Ok; 5,000 → `DailyLimitExceeded` (balance would allow it); 4,000 Ok (exactly at limit); 1 → `DailyLimitExceeded`. Balance 40,000 | `TC-WDR-01: a withdrawal that passes today's limit is refused` | Pass |
| TC-WDR-01b | BMS-F-030 | U | As above, limit used up | Move the bank day forward one day; withdraw 10,000 | Allowed: daily total restarts | `TC-WDR-01: the daily withdrawal total restarts…` | Pass |
| TC-WDR-01c | BMS-F-030 | U | Balance 1,000 | Withdraw 1,001 | `InvalidInput`; balance 1,000; journal empty | `TC-WDR-01: a withdrawal over the balance…` | Pass |
| TC-WDR-02a | BMS-F-032 | U | Savings, balance 80,000; minimum 50,000 | Withdraw 30,001; then 30,000; then 1; then 90,000 | 30,001 → `BelowMinimumBalance`, balance unchanged; 30,000 Ok (leaves exactly the minimum); 1 → `BelowMinimumBalance`; 90,000 → `InvalidInput` (more than the whole balance) | `TC-WDR-02: Savings minimum balance…` | Pass |
| TC-WDR-02b | BMS-F-032 | U | **Current** account, balance 10,000; same minimum configured | Withdraw 10,000 | Allowed: minimum applies to Savings only | `TC-WDR-02: the minimum balance applies to Savings only` | Pass |
| TC-WDR-02c | BMS-F-032 | U | Savings already below the minimum (30,000 vs 50,000) | Withdraw 1 | `BelowMinimumBalance` | `TC-WDR-02: a Savings account already under the minimum…` | Pass |
| TC-WDR-03 | BMS-F-033 | U | Current, balance 1,000, overdraft limit 5,000 | Withdraw 6,000, then 1 | 6,000 Ok → balance −5,000; the extra 1 is refused (`InvalidInput`) | `Current account withdrawal respects overdraft limit`, `TC-WDR-03: Current overdraft still works with the new limits…` | Pass |
| TC-WDR-04 *(new, edge)* | BMS-SR-004 | U | Current account stored with a *negative* overdraft limit | Withdraw; transfer | `InvalidInput`, balance unchanged | `A stored negative overdraft limit is refused…` | Pass |
| TC-WDR-05 *(new, edge)* | BMS-F-031 | U | Journal folder missing, so every journal write fails | Deposit; withdraw; transfer | None returns Ok; balances unchanged (journal-first rule) | `TC-REL-01 (partial): a journal that cannot be written…` | Pass |

## Transfer

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-TRF-01a | BMS-F-050 | U | A = 10,000; B = 2,000 | Transfer 2,500 A→B | A 7,500; B 4,500; journal has TRANSFER_OUT (A) then TRANSFER_IN (B); reconciliation reports no mismatch | `TC-TRF-01: a transfer moves the money and journals both legs` | Pass |
| TC-TRF-01b | BMS-F-050 | U | As above; store made to fail on the 1st write (debit), then on the 2nd write (credit) | Transfer 2,500 A→B | Returns `Io`; **both balances equal their original values**; reconciliation still clean (reversing entries); a retry afterwards succeeds | `TC-TRF-01: failure between the two legs…` | Pass |
| TC-TRF-01c | BMS-F-050 | S | Teller and customer logged in through the menu | Teller option 9 (A→B 200); customer option 5 (B→A 50) | Confirmation messages show both amounts | `TC-TRF-01/02: transfer through the teller and customer menus` | Pass |
| TC-TRF-02a | BMS-F-051 | U | A active, C closed, L locked | A→A; A→C; C→A; A→L; A→unknown; unknown→A; amount 0; empty operator | `SameAccount`; `AccountClosed`; `AccountClosed`; `AccountLocked`; `AccountNotFound` ×2; `InvalidInput` ×2. Distinct codes; **no balance or journal change** | `TC-TRF-02: self-transfer and transfer to a closed account…` | Pass |
| TC-TRF-02b | BMS-F-051 | U | Savings min 500; A = 1,000; B near Int64 max | A→B 501; 2,000; 500 | `BelowMinimumBalance`; `InvalidInput` (insufficient); `InvalidInput` (credit would overflow); A unchanged | `TC-TRF-02: a transfer cannot overdraw the source…` | Pass |
| TC-TRF-03a | BMS-F-052 | U | Per-transfer ceiling 5,000 | Transfer 5,001; then 5,000 | `TransferCeilingExceeded`; then Ok | `TC-TRF-03: per-transaction ceiling` | Pass |
| TC-TRF-03b | BMS-F-052 | U | Ceiling 5,000 each; daily ceiling 12,000 | 5,000; 5,000; 5,000; 2,000; next day 5,000 | Third is refused with `DailyLimitExceeded` **although it is within the per-transfer ceiling**; 2,000 Ok (exactly 12,000); next bank day allowed | `TC-TRF-03: daily ceiling is refused even when…` | Pass |
| TC-TRF-03c | BMS-F-052 | U | Daily withdrawal limit and daily transfer ceiling both 3,000 | Withdraw 3,000; transfer 3,000; withdraw 1; transfer 1 | First two Ok; last two `DailyLimitExceeded`. Counters are separate | `TC-TRF-03: transfers and withdrawals keep separate daily totals` | Pass |

## Related cases touched by this update

| ID | Req. | Test | Result |
|---|---|---|---|
| TC-SEC-07 (lock) | BMS-SR-006 | `TC-SEC-07: a second instance cannot take the lock…` | Pass |
| TC-SEC-07 (perms) | BMS-SR-006 | `…created owner-only…` (folder 0700, files 0600); `…group- or world-writable data folder is refused…` | Pass (Linux only) |
| TC-REL-01 (storage) | BMS-NF-002 | `Storage fix: a torn counters.dat is rebuilt…`, `…bad checksum…`, `…swaps in a complete file and leaves no temp file` | Pass |
| TC-MENU-LIMITS | BMS-NF-005 | `Menu describes the new limit refusals in plain words` | Pass |

## Deviations and open points (for the deviations report)

1. **BMS-F-031 / BMS-NF-002 — crash between journal write and balance write is detected, not repaired.** Deposit, withdrawal and transfer write the journal first, then the balance. If the process dies in between, the journal is ahead of the stored balance. `reconcile()` (BMS-F-061) reports it, but nothing replays the journal into the balance at start-up. `TC-REL-01` in its full form (kill the process mid-write, restart, verify balances) is therefore **not met**. Only the in-process failure case (TC-TRF-01b) is. A start-up replay using the `last_seq` field is the remaining work.
2. **BMS-F-052 says the transfer ceiling "shares the daily counter with BMS-F-030".** The accounts.dat layout (SAD 9.4) has two separate fields, `daily_withdrawn` and `daily_transferred`, and the code follows the layout. An account can therefore withdraw the full daily withdrawal limit *and* transfer the full daily transfer ceiling in the same day. SRS wording needs Dhanush's approval either way (CONTRIBUTING: requirement changes go through Person 1).
3. **BMS-F-030 note says the daily counter is "reset by BMS-F-062" (end-of-day report).** Implemented as an automatic roll-over when the stored `counter_date` differs from today, so no manual step is needed. The bank day changes at midnight India Standard Time.
4. **BMS-SR-007 — transfer ownership is checked in the menu, not the service.** `TransactionService::transfer` does not check that the logged-in customer owns `from`; the customer menu only ever passes their own account. The service has no session parameter today (deposit and withdraw do not either). Closing this means passing a `Session` into the transaction calls and adding operations to `authorize()`. **Not done in this update.**
5. **Limit values are my proposal and need team sign-off** (`src/cli/main.cpp`): Savings minimum Rs 500; withdrawals Rs 50,000 per account per day; one transfer Rs 1,00,000; transfers Rs 3,00,000 per account per day. The SRS does not state numbers.
6. **BMS-SR-006 on Windows.** The folder check and owner-only creation rely on POSIX modes; `platform_win.cpp` only creates the folder and relies on the user profile's ACL. The Windows code was written but **not compiled or run** in this update; the Windows CI job will be its first build.
7. **Stories not done:** 3.4 and 1.5 are unchanged by this update (3.4's overdraft logic already existed and is still tested).
