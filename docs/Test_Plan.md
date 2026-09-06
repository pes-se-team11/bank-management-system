# Software Test Plan (STP) — Bank Management System

**Project:** Bank Management System · **Team 11** · Implementation language: C / C++
**Author:** Adarsha E (PES1UG24AM334) — Person 3
**Version:** 0.1 (scaffold) · **Status:** Draft — **due 15 September 2025**

---

> ## How to use this file — read this first
>
> This is a **scaffold, not a draft**. It follows `Test_Plan_Template for SE.docx` section for
> section, so the numbering is already what the submission needs.
>
> Sections are marked one of two ways:
>
> - **`[FROM SRS]`** — filled in already, because the content is *derived from the requirements* and
>   Person 1 owns those. The requirement-to-test-case mapping, the scope exclusions, the boundary
>   values and the security checks all fall out of the SRS. Check them, correct anything you
>   disagree with, but you shouldn't need to invent them.
> - **`[YOU WRITE]`** — yours. These are test *thinking*, not requirement restatement, and they're
>   what you'll be asked about. Each one says what belongs there and gives you the SRS facts you'd
>   otherwise have to go digging for.
>
> **One rule that matters more than anything else here:** the `TC-` ids below are already cited by
> acceptance criteria in the SRS and by the RTM in SRS §8. **Use these exact ids**, or tell Dhanush
> to change the SRS. Inventing different ids silently breaks traceability, and that is the first
> thing an evaluator checks.

---

## 1. Introduction `[FROM SRS]`

**Purpose.** Defines the test plan for the Bank Management System v1.0 — objectives, scope,
strategy, resources, schedule and responsibilities for verifying the system against the SRS.

**Scope.** Customer and account management, authentication and role enforcement, deposit,
withdrawal, balance inquiry and statements, funds transfer, and the ledger, audit and reporting
behaviour. Exclusions in section 4.

**References.** SRS v1.0 (`docs/SRS.md`; the RTM is section 8), Jira backlog
(`docs/Jira_Backlog.md`), SAD (pending, Person 2).

**Definitions.** STP (Software Test Plan), SRS, RTM (Requirements Traceability Matrix), UAT (User
Acceptance Testing), WAL (write-ahead log), paise (smallest currency unit — all money is stored as
integer paise), reconciliation (an account's balance equals its opening balance plus the sum of its
journal entries).

---

## 2. Test items `[FROM SRS]`

`AuthModule` · `AccountModule` · `TransactionModule` · `LedgerModule` · `ReportModule` ·
`ValidationModule` · `PersistenceModule` · `CLI / MenuLayer` · and the build itself — a warning-free
compile is a tested property under BMS-NF-004.

---

## 3. Features to be tested `[FROM SRS]`

Every functional, non-functional and security requirement, mapped to the test-case id the SRS
already cites for it.

| Requirement(s) | Feature | Test case(s) |
|---|---|---|
| BMS-F-001 – BMS-F-006 | Customer records, account open/close/lookup, residual transfer | TC-ACC-01 … TC-ACC-06 |
| BMS-F-010 – BMS-F-013 | Authentication, lockout, role enforcement, unlock | TC-AUT-01 … TC-AUT-04 |
| BMS-F-014, BMS-SR-001 | No terminal echo, credential hashing | TC-SEC-01, TC-SEC-02 |
| BMS-F-020 – BMS-F-022 | Deposit, rejection cases, journalling | TC-DEP-01 … TC-DEP-03 |
| BMS-F-030, BMS-F-032, BMS-F-033 | Withdrawal limits, minimum balance, overdraft | TC-WDR-01 … TC-WDR-03 |
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

## 4. Features not to be tested `[FROM SRS]`

Taken from the scope exclusions in SRS §1.2 and §2.4.

- Compiler and standard-library correctness
- OS file-permission enforcement itself — we test that the program *requests* owner-only
  permissions (TC-SEC-07), not that the OS honours the request
- Terminal emulator behaviour
- Out of product scope: ATM terminal hardware, inter-bank settlement (NEFT/RTGS/UPI), cheque
  clearing, loans and interest, card issuance, internet and mobile banking
- Concurrent multi-instance operation — explicitly unsupported and prevented by an instance lock

---

## 5. Test approach / strategy `[YOU WRITE]`

Cover: **test levels** (unit, integration, system, acceptance) and what you'd verify at each;
**test types** (functional, regression, boundary-value, negative, fault injection, performance,
security, usability, portability); and **entry / exit criteria**.

Facts you'll need, so you don't have to dig:

- BMS-NF-006 requires the service modules to be unit-testable *without* the CLI layer — that's what
  makes real unit testing possible here, and worth saying so.
- The money arithmetic (BMS-SR-003) and reconciliation (BMS-F-061) are pure functions — they can be
  tested exhaustively at boundaries with no files involved.
- BMS-NF-002 and BMS-F-050 can only be tested by **fault injection** — killing the process
  mid-write, and forcing a failure between the two legs of a transfer. Decide how.
- The UI is a line-based CLI so tests can be scripted: `printf "2\n5000\n4\n" | ./bank | diff - expected.txt`

### Boundary values `[FROM SRS]`

Derived from the numeric limits in the requirements. This is where the system will actually fail.

| Requirement | Boundaries to test |
|---|---|
| BMS-F-021 | amount = −1, 0, 1, ceiling−1, ceiling, ceiling+1 |
| BMS-F-032 | balance after withdrawal = minimum−1, minimum, minimum+1 |
| BMS-F-033 | overdraft used = limit−1, limit, limit+1 |
| BMS-F-030, BMS-F-052 | daily total = ceiling−1, ceiling, ceiling+1 |
| BMS-SR-003 | balance near `INT64_MAX`; a deposit that would overflow |
| BMS-F-011 | 1st, 2nd, 3rd, 4th failed PIN attempt |
| BMS-F-041 | account with 0, 9, 10, 11 transactions |
| BMS-F-042 | range with no entries; boundaries inclusive at both ends |

### 5.1 Security validation `[FROM SRS]`

Each check traces to a security objective in SRS §5.1.1.

| Check | Requirement | Objective |
|---|---|---|
| Inspect data files for plaintext PIN or password; confirm a per-record salt | BMS-SR-001 | SO-1 |
| Confirm terminal echo disabled and the credential buffer zeroed after use | BMS-F-014 | SO-1 |
| Deposit near `INT64_MAX`; confirm refusal, not wraparound | BMS-SR-003 | SO-3 |
| Source search for `gets`, `strcpy`, `strcat`, `sprintf`, unbounded `scanf("%s")` | BMS-SR-002 | SO-3 |
| Over-length name; alphabetic input to an amount field; out-of-range menu choice | BMS-SR-004 | SO-3 |
| Attempt to modify a journal record through any menu path; inspect file open modes | BMS-SR-005 | SO-2 |
| Confirm files created owner-only; loosen the directory and confirm refusal to start | BMS-SR-006 | SO-1 |
| Call a Manager-only service function directly with a Teller role, bypassing the menu | BMS-SR-007 | SO-4 |
| Edit a balance in the data file by hand; confirm reconciliation names the account | BMS-F-061 | SO-2 |

The last two are the ones worth arguing in a viva — they test that the property holds when the
attacker does **not** come through the front door.

---

## 6. Test environment `[YOU WRITE]`

Cover: hardware, software, tools, and test data.

- **Fix one reference machine and record its spec.** Every BMS-NF-001 and TC-PERF number is
  meaningless without it, and results from two different laptops can't be compared.
- Toolchain is g++ `-std=c++17` on Linux and MinGW on Windows, no third-party libraries — so decide
  what your unit-test harness is. A hand-rolled assert harness is fine and avoids a dependency.
- Test data worth building: a Savings account at exactly the minimum balance, a Current account at
  exactly its overdraft limit, a LOCKED account, a CLOSED account, one near `INT64_MAX`, and
  accounts with exactly 9, 10 and 11 journal entries. **Synthetic names only.**

---

## 7. Test schedule `[YOU WRITE]`

Milestones with dates: test case design, environment ready, execution start and end, UAT.
Anchor them to the sprint plan in `Jira_Backlog.md` — Sprint 1 is foundations, so unit tests for the
money type and journal belong there, not at the end.

---

## 8. Test deliverables `[YOU WRITE]`

What testing produces: this plan, test cases, the harness and its sources, test data, execution
logs, defect reports, a coverage report from the RTM, and a final test summary report.

---

## 9. Roles and responsibilities `[YOU WRITE]`

Who does what during testing. The team is three — see `ROLES.md`. Note that you both write the plan
*and* execute the cases against Dhanush's and Vidit's modules, so say who fixes what when a case
fails.

---

## 10. Risks and mitigation `[YOU WRITE]`

A table of risk → mitigation. Some real ones for this project, if useful:

- Crash-safety and transfer atomicity can't be tested by hand — they need scripted fault injection
- A transfer-atomicity failure is *silent*; money just disappears unless you reconcile after runs
- Floating point creeping into the money path late — catch it with a build-time source scan
- Performance measured on different machines gives incomparable numbers

---

## 11. Assumptions and dependencies `[YOU WRITE]`

What you're relying on. Note the hard one: **you are blocked by Person 2 on the journal record
format** (BMS-F-060). You can't write TC-BAL-02 for the mini-statement until the on-disk format is
fixed. Get that from Vidit early.

---

## 12. Suspension and resumption criteria `[YOU WRITE]`

When testing stops and when it restarts. Worth stating that reconciliation failing on clean data
invalidates every other result, so it's a stop condition.

---

## 13. Test case management and traceability

**`[FROM SRS]`** — The RTM in SRS §8 is the single coverage record and is not duplicated here. A
requirement counts as covered only when its RTM row names at least one `TC-` id **and** that case has
been executed with a result. Status legend: `N` not started, `P` partial, `A` accepted.

**`[YOU WRITE]`** — the actual test cases behind each id: preconditions, steps, test data, expected
result. That's the bulk of your work between now and the 15th.

---

## 14. Test metrics and reporting `[YOU WRITE]`

Which metrics you collect (cases executed, pass/fail, requirement coverage, defect density, defect
aging, compiler warnings) and what you report, to whom, how often.

---

## 15. Approvals

| Role | Name | Signature / Date |
|---|---|---|
| Person 3 — Test Plan | Adarsha E (PES1UG24AM334) | |
| Person 2 — Design | Vidit Soni (PES1UG24AM318) | |
| Person 1 — Requirements | Dhanush S (PES1UG24AM360) | |
| Course Coordinator | | |
