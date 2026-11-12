# Test case updates — TC-ACC, TC-AUT, TC-SEC

**Owner of this update:** Dhanush S (PES1UG24AM360) · **Team 11 — Bank Management System**
**Baseline:** SRS v1.0 requirement IDs and acceptance criteria. **Result column:** automated run of `main` at `448ecb9` (after PR #24), 160 test cases in the suite, 0 failures; the 44 cases below all pass.
**Where the tests live:** `tests/test_account.cpp`, `tests/test_auth.cpp`, `tests/test_authorization.cpp`, `tests/test_validation.cpp`, `tests/test_crypto.cpp`, `tests/test_cli.cpp`.
Run one group: `./build/tests/bms_tests -tc="TC-AUT-02*"`

Test levels: **U** = unit (service + in-memory/fake store), **S** = scripted menu run (the real `Menu` driven by typed input), **M** = manual.

Amounts are in paise in unit tests (Rs 500 = 50,000 paise) and in rupees in menu runs.

## Customer and account management (TC-ACC)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-ACC-01a | BMS-F-001 | U | Empty store, Teller session | Create two customers with valid details | Each gets a new ID; the second is larger than the first; details stored as entered | `TC-ACC-01: a new customer gets an ID never used before` | Pass |
| TC-ACC-01b | BMS-F-001 | U | Empty store | Create a customer with contact "123", then a valid one | First refused `InvalidInput`, nothing stored; the valid one gets ID 1 (no ID used up) | `TC-ACC-01: invalid details are refused and use up no ID` | Pass |
| TC-ACC-02a | BMS-F-002 | U | Existing customer, Teller | Open Savings with Rs 5,000 and Current with Rs 0 | Different account numbers; state ACTIVE; type, opening balance and balance as given | `TC-ACC-02: an account opens ACTIVE with the deposit as its balance` | Pass |
| TC-ACC-02b *(new)* | BMS-F-002, BMS-F-032 | U | Existing customer | Open Savings with Rs 499.99, then exactly Rs 500 | First refused, no account created; Rs 500 accepted | `TC-ACC-02: Savings accounts require a minimum opening deposit of Rs 500` | Pass |
| TC-ACC-02c *(new)* | BMS-F-002, BMS-F-060 | U | Existing customer | Open an account with Rs 500 | One OPEN journal record: account, amount = balance_after = 50,000, operator `teller1`; account `last_seq` matches | `TC-ACC-02: opening writes an OPEN journal record with the opening balance` | Pass |
| TC-ACC-02d | BMS-F-002 | U | One customer | Open for unknown customer 42; with deposit −1; with an empty operator | `CustomerNotFound`; `InvalidInput`; `NotAuthorised`. No account and no journal record | `TC-ACC-02: opening is refused for an unknown customer or a negative deposit` | Pass |
| TC-ACC-03a *(new)* | BMS-F-003, BMS-F-063 | U | Customer "Asha Rao", Teller | Change address and contact | Both stored; name and ID reference unchanged; one CUSTOMER_MODIFY audit entry, operator `teller1`, details "address: old -> new; contact: old -> new" | `TC-ACC-03: a teller changes address and contact, and the audit keeps old and new` | Pass |
| TC-ACC-03b *(new)* | BMS-F-003 | U | As above | Change only the contact; then send the same values again | Audit holds only the contact change; the repeat writes nothing | `TC-ACC-03: only changed fields are recorded, and no change writes nothing` | Pass |
| TC-ACC-03c *(new)* | BMS-F-003, BMS-F-012 | U | As above | Contact "123"; empty address; address with `\|`; unknown ID 999; Manager session | `InvalidInput` ×3; `CustomerNotFound`; `NotAuthorised`. Record and audit log unchanged | `TC-ACC-03: modification is refused for bad input, unknown IDs and non-tellers` | Pass |
| TC-ACC-03d *(new)* | BMS-F-003 | S | Teller logged in, customer 1 exists | Option 8: keep address, change contact; keep both; bad contact; unknown ID | "Customer 1 updated."; "No changes made."; validation message; "No customer has that ID." | `TC-ACC-03: a teller modifies customer details from the menu` | Pass |
| TC-ACC-04a *(new)* | BMS-F-004 | U | Savings account at Rs 0, Manager | Close it; then deposit Rs 50 and withdraw Rs 1 | Close Ok; state CLOSED, record kept; deposit and withdrawal refused; balance 0; one ACCOUNT_CLOSE audit entry naming `manager1` | `TC-ACC-04: a zero-balance account closes and refuses every later transaction` | Pass |
| TC-ACC-04b *(new)* | BMS-F-004, BMS-F-012 | U | Funded account (Rs 500) and empty account | Close funded; close empty as Teller; close unknown; close empty twice | `BalanceNotZero`; `NotAuthorised`; `AccountNotFound`; Ok then `AccountClosed`. Only one audit entry | `TC-ACC-04: closure is refused with money left, twice, unknown, or by a teller` | Pass |
| TC-ACC-04c *(new)* | BMS-F-004 | U | CLOSED account with PIN 4821 | Log in with the right PIN, then a wrong PIN | Both `AccountClosed` | `TC-ACC-04: the right PIN for a closed account is refused` | Pass |
| TC-ACC-04d *(new, PR #19 review)* | BMS-F-004, BMS-F-011, BMS-F-013 | U | CLOSED account | Four wrong PINs; Manager unlock | Every login `AccountClosed`; failed-attempt count stays 0; unlock `AccountClosed`; state still CLOSED; audit log empty | `TC-ACC-04: wrong PINs and an unlock cannot reopen a closed account` | Pass |
| TC-ACC-04e *(new)* | BMS-F-004 | S | Account 100001 with Rs 500 | Manager closes (refused, money left); teller withdraws Rs 500; manager answers No, then Yes; teller deposits; customer logs in | Refusal message; "Not closed."; "Account 100001 closed."; list shows Closed; deposit and login refused | `TC-ACC-04: a manager closes an emptied account from the menu` | Pass |
| TC-ACC-04f *(new, PR #19 review)* | BMS-F-004 | S | As above, account closed | Three wrong PINs; manager unlock; teller deposit; customer login | No lock, no unlock, no deposit, no login; list still shows Closed | `TC-ACC-04: a closed account cannot be reopened through lockout and unlock` | Pass |
| TC-ACC-05 | BMS-F-005 | U | Customer A with 2 accounts, B with 1 | List A's accounts; look up 999999; list customer 77 | Exactly A's 2 accounts; `AccountNotFound`; `CustomerNotFound` (no crash) | `TC-ACC-05: look up by account number and list by customer` | Pass |
| TC-ACC-06 | BMS-F-006 | — | — | — | **Not run: story 1.5 was cut from Sprint 3** (see deviations). Closing a funded account is refused with `BalanceNotZero` instead (TC-ACC-04b). | — | Not run |

## Authentication and access control (TC-AUT)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-AUT-01a | BMS-F-010 | U | Customer 100001 (PIN 4821), Teller T007 | Log in with the right PIN and the right password | Sessions with operator ID and stored role (Customer, Teller) | `TC-AUT-01: correct credentials open a session with the stored role` | Pass |
| TC-AUT-01b | BMS-F-010 | U | As above | Wrong PIN; unknown account; staff ID used as a customer login | All `BadCredentials` (same refusal, nothing reveals which part was wrong) | `TC-AUT-01: a wrong PIN and an unknown ID give the same refusal` | Pass |
| TC-AUT-02a *(new)* | BMS-F-011 | U | Account 100001 with PIN 4821 | Three wrong PINs, then the right PIN | 1st and 2nd `BadCredentials`; 3rd `AccountLocked`; right PIN `AccountLocked`; state LOCKED; one ACCOUNT_LOCK audit entry naming the account, with a timestamp | `TC-AUT-02: the third wrong PIN locks the account and records it` | Pass |
| TC-AUT-02b *(new)* | BMS-F-011 | U | As above | Wrong, wrong, right, wrong, wrong, right | Never locks: a right PIN resets the count to 0; no audit entry | `TC-AUT-02: a right PIN resets the count, so only failures in a row lock` | Pass |
| TC-AUT-02c *(new)* | BMS-F-011 | U | Accounts 100001 and 100002 | Lock 100001; log in to 100002; four wrong PINs on unknown 999999 | 100002 unaffected; the unknown ID only ever gets `BadCredentials` | `TC-AUT-02: lockout touches only the account that was guessed at` | Pass |
| TC-AUT-02d *(new)* | BMS-F-011 | U | Manager M001 | Five wrong passwords, then the right one | Staff logins are never locked; the right password still works | `TC-AUT-02: staff passwords are never locked` | Pass |
| TC-AUT-02e *(new)* | BMS-F-011 | S | Account 100001 | Three wrong PINs and the right PIN at the menu; teller lists accounts | "This account is locked after three wrong PINs."; no login; list shows Locked | `TC-AUT-02: three wrong PINs lock the account in the menu` | Pass |
| TC-AUT-03a *(new)* | BMS-F-012 | U | Teller, Manager, Customer sessions | `authorize(EndOfDayReport)` for each | Teller and Customer `NotAuthorised`; Manager Ok (the SRS acceptance criterion) | `TC-AUT-03: a Teller is refused the end-of-day report; a Manager succeeds` | Pass |
| TC-AUT-03b *(new)* | BMS-F-012 | U | As above | Every operation for every role; sessions with no operator ID | Each operation allowed for exactly one role; empty operator always refused | `TC-AUT-03: each operation belongs to exactly one role` | Pass |
| TC-AUT-03c *(new)* | BMS-F-012 | S | Manager and teller logins | Manager tries option 8; teller tries option 9 | Each sees only its own menu; out-of-range options re-prompt | `TC-AUT-03: each role gets its own menu and input ending exits cleanly` | Pass |
| TC-AUT-04a *(new)* | BMS-F-013 | U | Locked account 100001 | Manager unlocks; log in with the right PIN | Ok; count 0; state ACTIVE; login works; audit = ACCOUNT_LOCK then ACCOUNT_UNLOCK naming `manager1` (the SRS acceptance criterion) | `TC-AUT-04: after a manager unlocks, the right PIN works and the count is zero` | Pass |
| TC-AUT-04b *(new)* | BMS-F-011, BMS-F-013 | U | Unlocked account | Three more wrong PINs | Locks again on the third | `TC-AUT-04: an unlocked account locks again after three more wrong PINs` | Pass |
| TC-AUT-04c *(new)* | BMS-F-012, BMS-F-013 | U | Locked account | Unlock as Teller and as Customer; unknown account; unlock twice | `NotAuthorised` ×2 (still locked); `AccountNotFound`; Ok then `AccountNotLocked`; refusals write no audit entry | `TC-AUT-04: only a manager can unlock, and only a locked account` | Pass |
| TC-AUT-04d *(new)* | BMS-F-013 | S | Locked account 100001 | Manager option 5: answers "not verified", then "verified", then again | "Not unlocked."; "Account 100001 unlocked."; "not locked"; customer logs in | `TC-AUT-04: a manager unlocks a locked account from the menu` | Pass |

## Security (TC-SEC, Person 1 cases)

| ID | Req. | Level | Preconditions | Steps | Expected result | Automated test | Result |
|---|---|---|---|---|---|---|---|
| TC-SEC-01a | BMS-SR-001 | U | — | Enrol the same PIN on two accounts; inspect the stored credentials | Different random salts and different hashes; PIN digits appear nowhere in what is stored | `TC-SEC-01: only a salted hash is stored, never the PIN` | Pass |
| TC-SEC-01b | BMS-SR-001 | U | — | Run SHA-256, HMAC-SHA256 and PBKDF2 against published test vectors; compare equal and unequal buffers | Every vector matches; constant-time compare is correct | `crypto: SHA-256 …`, `crypto: HMAC-SHA256 …`, `crypto: PBKDF2-HMAC-SHA256 …`, `crypto: constant-time comparison` | Pass |
| TC-SEC-02a | BMS-F-014 | U | — | Enrol; log in right; log in wrong | Caller's PIN string is empty after each call, success or failure | `TC-SEC-02: the caller's copy of the secret is wiped after use` | Pass |
| TC-SEC-02b | BMS-F-014 | M | Real terminal | Type a PIN at "PIN:" | Nothing is shown while typing | Manual (terminal echo cannot be checked from a test) | Pass |
| TC-SEC-04a *(new)* | BMS-SR-002 | S | Teller logged in | Enter a 10,000-character name, then a normal customer | Refused with the length message; no crash; the next customer gets ID 1 | `TC-SEC-04: an over-long name is refused safely and the menu carries on` | Pass |
| TC-SEC-04b | BMS-SR-002 | — | Source tree | `tools/check_banned.sh` (runs in CI on every PR) | No `gets`, `strcpy`, `strcat`, `sprintf` or unbounded `scanf` | CI job "Build and test" | Pass |
| TC-SEC-05a *(new)* | BMS-SR-004 | S | Teller logged in | Bad customer ID, bad account type; bad account number; three bad amounts ("five", "-5", "1e3"); menu choice 42 | Each re-prompts with a reason; after 3 bad amounts "Too many invalid entries. Nothing was changed"; no deposit, balance still Rs 500; menu choice re-prompts | `TC-SEC-05: invalid numbers and choices re-prompt and change nothing` | Pass |
| TC-SEC-05b | BMS-SR-004 | U | — | Feed malformed text, contacts, IDs, PINs, passwords, account numbers and amounts to the validators | Each malformed value refused; valid ones accepted; "1250.50" becomes 125,050 paise | `validation: …` (5 cases) | Pass |
| TC-SEC-08a *(new)* | BMS-SR-007 | U | Services only, no menu | `createCustomer` / `openAccount` as Manager and Customer | `NotAuthorised`; nothing stored or journalled; the same call as Teller works | `TC-SEC-08: account services refuse the wrong role with no menu involved` | Pass |
| TC-SEC-08b *(new)* | BMS-SR-007 | U | Services only | Teller creates staff; Customer creates staff; Manager sets a PIN | All `NotAuthorised`, secret wiped, nothing stored; Manager→staff and Teller→PIN work | `TC-SEC-08: only a Manager creates staff and only a Teller sets a PIN` | Pass |
| TC-SEC-08c *(new)* | BMS-SR-007 | U | Fresh install | Create the first manager; try again once staff exists | First Ok and can log in; second `NotAuthorised` | `TC-SEC-08: the first manager can be created only on a fresh install` | Pass |

TC-SEC-03 (money arithmetic), TC-SEC-06 (append-only journal) and TC-SEC-07 (file permissions) belong to the Transaction, Ledger and Persistence owners.

## Deviations and open points (for the deviations report)

1. **BMS-F-006 / TC-ACC-06 — residual transfer on closure not built.** Story 1.5 was cut from Sprint 3. Closure works only at a zero balance (BMS-F-004); a funded account is refused with "withdraw or move the remaining balance first".
2. **BMS-F-012 — roles are strictly separate.** Following the SRS wording, a Manager does not inherit Teller operations: creating customers, modifying them and opening accounts are Teller-only. A fresh install therefore starts with the manager adding a teller.
3. **BMS-F-011 — only customer PINs lock.** Staff passwords are never locked, because v1.0 has no way to unlock a staff login and locking the only manager would lock out the bank.
4. **BMS-F-004 — a closed account is refused before the PIN is checked** (PR #19 review), so even a wrong PIN on a closed account reports "closed".
5. **BMS-SR-002 — over-long input is refused, not truncated.** The SRS criterion says "truncated safely"; the system rejects the whole entry with the length limit instead, which is equally safe and avoids storing a silently shortened name.
6. **BMS-F-002 — Savings accounts need Rs 500 to open** (added in PR #24 after review), so a new Savings account always starts at or above the BMS-F-032 minimum. This is not in the SRS wording of BMS-F-002.
7. **BMS-F-014 — no-echo PIN entry is checked manually** (TC-SEC-02b); the wipe-after-use half is automated.
