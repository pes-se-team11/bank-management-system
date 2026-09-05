# Software Test Plan (STP) — Bank Management System

**Project:** Bank Management System · **Team 11** · Implementation language: C / C++
**Version:** 0.9 (draft) · **Date:** 05-09-2025
**Status:** Draft — **due 15 September 2025**

> Written against `templates/Test_Plan_Template for SE.docx`, section for section. Every `TC-` id
> below is already cited by an acceptance criterion in [`SRS.md`](SRS.md), so the two documents agree
> by construction. Items needing team input are marked **`<TBC>`**.

---

## 1. Introduction

**Purpose.** Defines the test plan for the Bank Management System v1.0 — objectives, scope, strategy,
resources, schedule and responsibilities for verifying the system against the SRS.

**Scope.** Customer and account management, authentication and role enforcement, deposit, withdrawal,
balance inquiry and statements, funds transfer, and the ledger, audit and reporting behaviour.
Exclusions in section 4.

**References.** SRS v1.0 (`docs/SRS.md`), RTM (`docs/RTM.md`), Jira backlog (`docs/Jira_Backlog.md`),
SAD (pending).

**Definitions.** STP (Software Test Plan), SRS, RTM, UAT (User Acceptance Testing), WAL
(write-ahead log), paise (smallest currency unit), reconciliation (balance equals opening balance
plus journal entries).

---

## 2. Test items

`AuthModule` · `AccountModule` · `TransactionModule` · `LedgerModule` · `ReportModule` ·
`ValidationModule` · `PersistenceModule` · `CLI / MenuLayer` · the build itself (a warning-free
compile is a tested property, per BMS-NF-004).

---

## 3. Features to be tested

| Requirement(s) | Feature | Test case(s) |
|---|---|---|
| BMS-F-001 – BMS-F-006 | Customer records, account open/close/lookup, residual transfer | TC-ACC-01 … TC-ACC-06 |
| BMS-F-010 – BMS-F-013 | Authentication, lockout, role enforcement, unlock | TC-AUT-01 … TC-AUT-04 |
| BMS-F-014, BMS-SR-001 | No echo, credential hashing | TC-SEC-01, TC-SEC-02 |
| BMS-F-020 – BMS-F-022 | Deposit, rejection cases, journalling | TC-DEP-01 … TC-DEP-03 |
| BMS-F-030, BMS-F-032, BMS-F-033 | Limits, minimum balance, overdraft | TC-WDR-01 … TC-WDR-03 |
| BMS-F-031, BMS-NF-002 | Atomic debit + journal, crash recovery | TC-REL-01 |
| BMS-F-040 – BMS-F-042 | Balance, mini-statement, date-range statement | TC-BAL-01 … TC-BAL-03 |
| BMS-F-050 – BMS-F-052 | Transfer atomicity, rejection cases, ceilings | TC-TRF-01 … TC-TRF-03 |
| BMS-F-060 – BMS-F-064 | Append-only journal, reconciliation, EOD report, audit capture and view | TC-LED-01 … TC-LED-05 |
| BMS-NF-001, BMS-NF-007 | Transaction latency, capacity soak | TC-PERF-01, TC-PERF-02 |
| BMS-NF-003, BMS-SR-003 | Integer paise, overflow refusal | TC-SEC-03 |
| BMS-SR-002, BMS-SR-004 | Buffer safety, input validation | TC-SEC-04, TC-SEC-05 |
| BMS-SR-005 – BMS-SR-007 | Append-only enforcement, file permissions, service-layer role check | TC-SEC-06 … TC-SEC-08 |
| BMS-NF-004, BMS-NF-006 | Warning-free portable build, modules testable in isolation | TC-PORT-01, TC-PORT-02 |
| BMS-NF-005 | Every rejection states reason and remedy | TC-UX-01 |

---

## 4. Features not to be tested

- Compiler and standard-library correctness
- Operating-system file-permission enforcement itself (we test that we *request* owner-only
  permissions, not that the OS honours them)
- Terminal emulator behaviour
- Anything in SRS §1.2 out-of-scope: ATM hardware, inter-bank settlement, cheque clearing, loans and
  interest, card issuance, internet/mobile banking
- Concurrent multi-instance operation — explicitly unsupported and prevented by the instance lock;
  we test that the *lock* works (TC-SEC-07), not that concurrency is safe

---

## 5. Test approach / strategy

**Levels**

- **Unit** — the highest-value target in this project. The money arithmetic (BMS-SR-003) and the
  reconciliation function (BMS-F-061) are pure logic and can be exercised exhaustively at boundaries
  without any file or menu involvement. BMS-NF-006 exists so that this is possible.
- **Integration** — service modules against the real `PersistenceModule` and real files, which is
  where the atomicity requirements actually live.
- **System** — end-to-end journeys through the menu.
- **Acceptance (UAT)** — two journeys: *open an account, deposit, withdraw, check balance*; and
  *transfer, close an account, run the end-of-day report and reconcile*.

**Types.** Functional · regression on every PR touching a tested module · boundary-value analysis
(the money and limit requirements are almost entirely boundary conditions) · negative testing ·
fault injection (BMS-NF-002, BMS-F-050) · performance · security · usability · portability.

**Entry criteria.** Build compiles clean under `-Wall -Wextra -Werror`; seed data loaded; smoke suite
green.

**Exit criteria.** 100% of planned cases executed; zero open critical defects; no open major defect
against a High-priority requirement; reconciliation passing across the full data set; every RTM row
at status `A`.

**Boundary values to test explicitly** — these are where this system will actually fail:

| Requirement | Boundaries |
|---|---|
| BMS-F-021 | amount = -1, 0, 1, ceiling-1, ceiling, ceiling+1 |
| BMS-F-032 | balance after withdrawal = minimum-1, minimum, minimum+1 |
| BMS-F-033 | overdraft used = limit-1, limit, limit+1 |
| BMS-F-030, BMS-F-052 | daily total = ceiling-1, ceiling, ceiling+1 |
| BMS-SR-003 | balance near `INT64_MAX`; deposit that would overflow |
| BMS-F-041 | account with 0, 9, 10, 11 transactions |
| BMS-F-042 | range with no entries; range boundaries inclusive on both ends |

### 5.1 Security validation

Each check traces to a security objective from SRS §5.1.1.

| Check | Requirement | Objective |
|---|---|---|
| Inspect data files for plaintext PIN or password; confirm per-record salt | BMS-SR-001 | SO-1 |
| Confirm terminal echo disabled and credential buffer zeroed after use | BMS-F-014 | SO-1 |
| Deposit near `INT64_MAX`; confirm refusal, not wraparound | BMS-SR-003 | SO-3 |
| Source search for `gets`, `strcpy`, `strcat`, `sprintf`, unbounded `scanf("%s")` | BMS-SR-002 | SO-3 |
| Over-length name, alphabetic input to amount field, out-of-range menu choice | BMS-SR-004 | SO-3 |
| Attempt to modify a journal record through any menu path; inspect file open modes | BMS-SR-005 | SO-2 |
| Confirm data files created owner-only; loosen directory and confirm refusal to start | BMS-SR-006 | SO-1 |
| Call a Manager-only service function directly with a Teller role, bypassing the menu | BMS-SR-007 | SO-4 |
| Edit a balance in the data file by hand; confirm reconciliation names the account | BMS-F-061 | SO-2 |

The last two are the ones worth arguing about in a viva: they test that the security property holds
when the attacker does *not* go through the front door.

---

## 6. Test environment

**Hardware.** Reference machine `<TBC — fix one machine and record its spec; every BMS-NF-001 and
BMS-PERF result is meaningless without it>`.

**Software.** g++ with `-std=c++17` on Linux, MinGW on Windows. No third-party libraries.

**Tools.** Manual execution against documented test cases, plus a unit-test harness `<TBC — a
hand-rolled assert harness is acceptable and avoids a third-party dependency>`. Jira for defect
tracking. Shell scripts for fault injection (killing the process mid-write) and for the source scan
in BMS-SR-002.

**Test data.** 10,000 synthetic accounts for the capacity soak; a small hand-built set for functional
work covering: Savings at exactly minimum balance, Current at exactly its overdraft limit, a LOCKED
account, a CLOSED account, an account at a balance near `INT64_MAX`, and an account with exactly 9,
10 and 11 journal entries. **No real customer names or identifiers.**

---

## 7. Test schedule

| Milestone | Date |
|---|---|
| Test plan approved | 15-Sep-2025 |
| Test case design complete | `<TBC>` |
| Unit tests alongside Sprint 1 code | `<TBC>` |
| Integration testing from Sprint 2 | `<TBC>` |
| System testing | `<TBC>` |
| UAT | `<TBC>` |

Dates follow the sprint plan agreed after 15 September.

---

## 8. Test deliverables

Test plan (this document) · test cases · unit-test harness and sources · test data generator ·
execution logs · defect reports · requirement coverage report derived from the RTM · test summary
report.

---

## 9. Roles and responsibilities

| Role | SRN | Responsibility |
|---|---|---|
| QA / Test Lead | PES1UG24AM318 | Owns this plan, coordinates execution, signs off exit criteria |
| Test Engineer | PES1UG24AM334 | Designs and executes cases, logs defects |
| Developer | PES1UG24AM305 | Fixes and triages defects, maintains the build |
| Requirements Lead | PES1UG24AM360 | Arbitrates what an acceptance criterion means |

---

## 10. Risks and mitigation

| Risk | Mitigation |
|---|---|
| Crash-safety (BMS-NF-002) is hard to test by hand | Script the kill: run the transaction under a harness that terminates the process at randomised points, then reconcile. Do not test this manually. |
| Transfer atomicity failure is silent — money quietly disappears | Reconcile after every integration run, not only when a test fails |
| Floating point creeps into the money path late in the project | Add the source scan to the build gate in Sprint 1, not at test time |
| Performance measured on different machines gives incomparable numbers | Fix the reference machine before the first measurement and record it with every result |
| Sprint 1 delivers no visible feature, so testing feels premature | Unit tests for the money type and journal are written in Sprint 1 — that is exactly when they are cheapest |
| Team member unavailable near submission | Every role has a named second reviewer in `ROLES.md` |

---

## 11. Assumptions and dependencies

- The SAD has settled module boundaries and file formats before integration test design begins
- BMS-NF-006 holds, so service modules can be linked into a test binary without the CLI
- Test data is regenerable from a script, so a corrupted run can be reset
- Acceptance criteria are frozen once execution starts; a change means a new SRS revision row and a
  re-run of the affected cases

---

## 12. Suspension and resumption criteria

**Suspend** when the build does not compile, when reconciliation fails on clean data (which
invalidates every other result), or when a critical defect against SO-1 or SO-2 is open.

**Resume** when the blocking defect is fixed and verified and the smoke suite passes.

---

## 13. Test case management and traceability

The RTM (`docs/RTM.md`, mirrored in SRS §8) is the single coverage record and is not duplicated here.
A requirement counts as covered only when its row names at least one `TC-` id **and** that case has
been executed with a result.

Examples: `BMS-F-050` → `TC-TRF-01` · `BMS-NF-002` → `TC-REL-01` · `BMS-SR-003` → `TC-SEC-03`.

---

## 14. Test metrics and reporting

**Metrics.** Cases executed (%) · passed/failed (%) · requirement coverage from the RTM · defect
density by module · defect aging · defects reopened · compiler warnings (target: zero).

**Reports.** Execution status at each stand-up · coverage report at end of execution · final test
summary report submitted with the project.

---

## 15. Approvals

| Role | Name | Signature / Date |
|---|---|---|
| QA / Test Lead | | |
| Design Lead | | |
| Requirements Lead | | |
| Course Coordinator | | |
