# Jira Backlog — Bank Management System

**Team 11** · **complete backlog due 15 September 2026**

Jira project: `Bank Management System` — Software Development → **SCRUM** → Company-managed.
Owned by the Jira & Traceability Lead. Keep this file and the Jira board in agreement; where they
disagree, the SRS wins and both get corrected.

Every story names the SRS requirements it delivers. Every requirement in the SRS appears in exactly
one story — that correspondence is what the RTM in [`SRS.md`](SRS.md) §8 is checked against.

---

## Epic 1 — Customer & Account Management

Creation and lifecycle of customers and their accounts.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 1.1 Create Customer Record | BMS-F-001 | High | 3 | 1 |
| 1.2 Open and Look Up Accounts | BMS-F-002, BMS-F-005 | High | 5 | 1 |
| 1.3 Modify Customer Details | BMS-F-003 | Medium | 2 | 4 |
| 1.4 Close a Zero-Balance Account | BMS-F-004 | High | 3 | 4 |
| 1.5 Transfer Residual Balance on Closure | BMS-F-006 | Medium | 5 | 4 |

**1.1** — As a Bank Teller, I want to register a customer with their contact and ID details and get a
unique customer ID, so that accounts can be opened against a person rather than a loose name.

**1.2** — As a Bank Teller, I want to open a Savings or Current account for an existing customer and
find any account by number or by customer, so that I can serve someone at the counter without
paperwork.

**1.3** — As a Bank Teller, I want to update a customer's address and phone number with the previous
value retained, so that records stay current and a mistaken edit can still be traced.

**1.4** — As a Bank Manager, I want to close an account that has been emptied and have the system
refuse every later transaction on it, so that a closed account cannot quietly come back to life.

**1.5** — As a Bank Manager, I want a closure request on an account still holding money to move that
residual to a nominated account first, so that closing an account can never destroy a balance.

---

## Epic 2 — Authentication & Access Control

Who the operator is, and what their role permits.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 2.1 Authenticate Customer and Staff | BMS-F-010, BMS-F-014 | High | 5 | 2 |
| 2.2 Lock Account After Failed Attempts | BMS-F-011 | High | 3 | 2 |
| 2.3 Enforce Role-Based Authorisation | BMS-F-012 | High | 5 | 2 |
| 2.4 Manager Unlocks a Locked Account | BMS-F-013 | Medium | 2 | 4 |

**2.1** — As a Customer, I want to sign in with my account number and PIN without the PIN appearing on
screen, so that nobody standing behind me at the counter can read it.

**2.2** — As a Bank Manager, I want an account locked after three wrong PINs with the event recorded,
so that a stolen passbook cannot be used to guess a PIN.

**2.3** — As a Bank Manager, I want each role restricted to its own operations, so that a Teller
cannot close accounts or read the audit log.

**2.4** — As a Bank Manager, I want to unlock an account after verifying the holder, so that a
customer who simply forgot their PIN is not permanently locked out.

---

## Epic 3 — Core Transactions

Deposit and withdrawal — the operations the system exists for.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 3.1 Deposit into an Account | BMS-F-020, BMS-F-021, BMS-F-022 | High | 5 | 2 |
| 3.2 Withdraw Within Limits | BMS-F-030, BMS-F-031 | High | 8 | 2 |
| 3.3 Enforce Savings Minimum Balance | BMS-F-032 | High | 3 | 3 |
| 3.4 Allow Current Account Overdraft | BMS-F-033 | Medium | 3 | 3 |

**3.1** — As a Customer, I want my deposit credited to my account and recorded in the ledger, so that
the money is provably mine from the moment it is handed over.

**3.2** — As a Customer, I want to withdraw cash only when my balance and daily limit allow it, with
the debit and the ledger entry applied together, so that a failure part-way through cannot leave my
balance and the record disagreeing.

**3.3** — As a Bank Manager, I want Savings withdrawals refused below the minimum balance, so that
the account stays within the product's terms.

**3.4** — As a Customer with a Current account, I want to draw against my sanctioned overdraft, so
that a short-term shortfall does not stop a legitimate payment.

---

## Epic 4 — Balance Inquiry & Statements

Read-only views of position and history.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 4.1 Check Current Balance | BMS-F-040 | High | 2 | 3 |
| 4.2 Print a Mini-Statement | BMS-F-041 | Medium | 3 | 3 |
| 4.3 Statement for a Date Range | BMS-F-042 | Medium | 3 | 3 |

**4.1** — As a Customer, I want to see my current balance, so that I know what I can spend.

**4.2** — As a Customer, I want the last ten transactions listed newest first, so that I can check a
recent entry without asking for a full statement.

**4.3** — As a Customer, I want a statement between two dates with a closing balance, so that I can
reconcile a period against my own records.

---

## Epic 5 — Funds Transfer

Moving money between two accounts at the bank.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 5.1 Transfer Between Accounts Atomically | BMS-F-050, BMS-F-051 | High | 8 | 3 |
| 5.2 Enforce Transfer Ceilings | BMS-F-052 | Medium | 3 | 3 |

**5.1** — As a Customer, I want a transfer to either complete on both accounts or on neither, so that
money can never be destroyed or duplicated by a failure between the two halves.

**5.2** — As a Bank Manager, I want per-transaction and daily transfer ceilings enforced, so that a
compromised account cannot be drained in one sitting.

---

## Epic 6 — Ledger, Audit & Reporting

The record that makes every balance explicable.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 6.1 Append-Only Journal and Audit Capture | BMS-F-060, BMS-F-063 | High | 5 | 2 |
| 6.2 Reconcile Balances Against the Journal | BMS-F-061 | High | 5 | 4 |
| 6.3 Generate End-of-Day Report | BMS-F-062 | Medium | 5 | 4 |
| 6.4 View the Audit Log | BMS-F-064 | Low | 2 | 4 |

**6.1** — As a Bank Manager, I want every balance change and every privileged action appended to a
record that the application cannot rewrite, so that the history of an account cannot be quietly
edited.

**6.2** — As a Bank Manager, I want a check that every balance equals its opening balance plus its
journal entries, so that tampering with the data files is detected instead of absorbed.

**6.3** — As a Bank Manager, I want the day's deposits, withdrawals, transfers and closing balances
totalled, so that I can close the branch knowing it reconciles.

**6.4** — As a Bank Manager, I want to read the audit log filtered by operator or date, so that I can
investigate a disputed action.

---

## Epic 7 — Platform Quality & Security

Cross-cutting properties every feature depends on. These are not "extras" — three of them
(7.1, 7.2, 7.3) must land in Sprint 1, because retrofitting a money type, a crash-safe write path or
credential hashing after the features are written means rewriting the features.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 7.1 Integer-Paise Money Type with Overflow Checks | BMS-NF-003, BMS-SR-003 | High | 5 | 1 |
| 7.2 Crash-Safe Write-Ahead Journal | BMS-NF-002, BMS-SR-005 | High | 8 | 1 |
| 7.3 Salted Credential Hashing and No-Echo Entry | BMS-SR-001 | High | 5 | 1 |
| 7.4 Input Validation and Bounds-Checked Buffers | BMS-SR-002, BMS-SR-004 | High | 5 | 3 |
| 7.5 File Permissions, Instance Lock, Service-Layer Role Check | BMS-SR-006, BMS-SR-007 | High | 5 | 3 |
| 7.6 Portable Warning-Free Build and Layered Modules | BMS-NF-004, BMS-NF-005, BMS-NF-006 | Medium | 5 | 4 |
| 7.7 Performance and Capacity Benchmark | BMS-NF-001, BMS-NF-007 | Low | 5 | 4 |

**7.1** — As a developer, I want all money handled as 64-bit integer paise with overflow checked
before every operation, so that no rounding error or silent wrap can ever alter a balance.

**7.2** — As a Bank Manager, I want the system to journal an intended change before applying it and
recover cleanly on restart, so that a power cut mid-transaction cannot corrupt the ledger.

**7.3** — As a Customer, I want my PIN stored only as a salted hash and never echoed or logged, so
that someone reading the data files cannot learn it.

**7.4** — As a developer, I want every input validated and every buffer bounds-checked, so that a
malformed entry cannot corrupt memory or drive the program into undefined behaviour.

**7.5** — As a Bank Manager, I want data files readable only by their owner, a second instance
prevented from starting, and role checks enforced in the service layer, so that access control cannot
be bypassed by reaching an operation another way.

**7.6** — As a developer, I want a warning-free build on both toolchains and modules testable without
the menu layer, so that defects surface at compile time and in unit tests rather than in a demo.

**7.7** — As a Bank Manager, I want the system measured at 10,000 accounts, so that its stated
performance is a measurement rather than a hope.

---

## Sprint plan

| | Theme | Stories | Points |
|---|---|---|---|
| **Sprint 1** | Foundations — money type, safe writes, credentials, records | 7.1, 7.2, 7.3, 1.1, 1.2 | **26** |
| **Sprint 2** | Authenticate and transact | 2.1, 2.2, 2.3, 3.1, 3.2, 6.1 | **31** |
| **Sprint 3** | Complete the money path, harden input | 3.3, 3.4, 5.1, 5.2, 4.1, 4.2, 4.3, 7.4, 7.5 | **35** |
| **Sprint 4** | Administration, reporting, build quality | 1.3, 1.4, 1.5, 2.4, 6.2, 6.3, 6.4, 7.6, 7.7 | **34** |
| | | **29 stories** | **126** |

### Sequencing rationale

**Sprint 1 is deliberately unglamorous.** It produces no user-visible feature. That is the point:
the money type (7.1), the write path (7.2) and credential storage (7.3) are decisions every later
story is built on top of. Getting deposit working first and *then* discovering that balances are
`double` means rewriting deposit, withdrawal and transfer.

**Sprint 2 delivers the first demonstrable slice** — a customer can sign in, deposit and withdraw,
and every movement is journalled.

**Sprint 3 completes the money path** and hardens input handling while the transaction code is still
fresh.

**Sprint 4 is administration and quality.** Stories 7.6 and 7.7 are the designated spillover: they
are Medium and Low priority, and the system is demonstrable without them. If Sprint 4 overruns,
those two move out rather than reconciliation (6.2), which is High priority and non-negotiable.

### Capacity note

This team has no measured velocity yet — no sprint has been run on this project. The four sprints
are balanced at 26/31/35/34 on the assumption of roughly equal capacity throughout. **Re-plan after
Sprint 1 using its actual completion**, rather than treating these numbers as commitments. Sprint 1
is deliberately the lightest, both because it is the least certain work and because it produces the
first real velocity measurement.
