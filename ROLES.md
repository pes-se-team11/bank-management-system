# Team roles & responsibilities

**Team 11 — Bank Management System (C/C++)** · **three members**

| | Name | SRN | GitHub | Atlassian |
|---|---|---|---|---|
| Person 1 | Dhanush S | PES1UG24AM360 | `@dhanushs1912-svg` | member |
| Person 2 or 3 | Adarsha E | PES1UG24AM334 | `@Adarsh-031` | **site owner** — `pes1ug24am334.atlassian.net` |
| Person 2 or 3 | Vidit Soni | PES1UG24AM318 | `@itsvidit1702` | member |

`PES1UG24AM305` has moved to another team.

> **One thing still to settle:** which of Adarsha and Vidit is Person 2 (Architecture/Design +
> Transactions) and which is Person 3 (Test Plan + Reporting). Everything below is written against
> the person numbers, so agreeing it is a two-line edit to this table.

---

## Person 1 — Requirements & Auth/Account module

**Dhanush S · PES1UG24AM360 · `@dhanushs1912-svg`**

**Documentation — the SRS.** Functional requirements, non-functional requirements, security
objectives and requirements, use-case diagrams, RTM.

**Code — `AccountModule`, `AuthModule`, `ValidationModule`.** Account creation, login by PIN or
password, input validation.

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-001, F-002, F-005 | Customer record, account opening, lookup | 1.1, 1.2 |
| BMS-F-003, F-004 | Modify customer details, close account | 1.3, 1.4 |
| BMS-F-006 | Residual transfer on closure — **shared with Person 2** | 1.5 |
| BMS-F-010, F-014 | Authenticate customer and staff; no terminal echo | 2.1 |
| BMS-F-011, F-013 | Lockout after 3 failures; manager unlock | 2.2, 2.4 |
| BMS-F-012, BMS-SR-007 | Role-based authorisation, in the service layer | 2.3 |
| BMS-SR-001 | Salted credential hashing | 7.3 |
| BMS-SR-002, BMS-SR-004 | Bounds-checked buffers, input validation | 7.4 |

**43 points.** Sprint 1: stories 1.1, 1.2, 7.3.

---

## Person 2 — Architecture/Design & Transaction module

**Documentation — the SAD.** Layered architecture pattern, component diagram, two or more sequence
diagrams (**withdraw** and **transfer** are the right pair — between them they exercise every
layer), module interface definitions, technology stack, threat model against `SO-1`…`SO-4`.

**Code — `TransactionModule`, `PersistenceModule`.** Deposit, withdrawal, balance update, fund
transfer, file persistence.

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-020, F-021, F-022 | Deposit, rejection cases, journal entry | 3.1 |
| BMS-F-030, F-031 | Withdrawal limits; atomic debit plus journal | 3.2 |
| BMS-F-032, F-033 | Savings minimum balance; Current overdraft | 3.3, 3.4 |
| BMS-F-050, F-051, F-052 | Atomic two-leg transfer, rejections, ceilings | 5.1, 5.2 |
| BMS-NF-003, BMS-SR-003 | Integer-paise money type, overflow checks | 7.1 |
| BMS-NF-002, BMS-SR-005 | Crash-safe write-ahead journal, append-only files | 7.2 |
| BMS-SR-006 | Owner-only file permissions, instance lock | 7.5 |

**48 points**, including the two hardest requirements in the project — `BMS-NF-002` crash-safety and
`BMS-F-050` transfer atomicity. Sprint 1: stories 7.1, 7.2.

---

## Person 3 — Test Plan & Reporting/Admin module

**Documentation — the Test Plan.** Test items, features to and not to be tested, test levels, entry
and exit criteria, schedule, risk table, and **linking Person 1's requirement IDs to real test-case
IDs**.

**Code — `LedgerModule` (read side), `ReportModule`.** Transaction history, mini-statement, admin and
reporting functions. Also runs the test cases against Persons 1 and 2's modules once integration
starts.

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-040, F-041, F-042 | Balance, mini-statement, date-range statement | 4.1, 4.2, 4.3 |
| BMS-F-060, F-063 | Append-only journal, audit capture — **shared with Person 2** | 6.1 |
| BMS-F-061 | Reconciliation check | 6.2 |
| BMS-F-062, F-064 | End-of-day report, audit log view | 6.3, 6.4 |
| BMS-NF-004, NF-005, NF-006 | Portable build, error messages, layered modules | 7.6 |
| BMS-NF-001, BMS-NF-007 | Performance and capacity benchmark | 7.7 |

**35 points.**

**Person 3 also owns the Jira board** — the 3-way split left it unassigned, and *"complete Jira
backlog"* is graded on 15 September. It belongs here because Person 3 already owns the RTM, and the
backlog is the third leg of the same traceability chain. Adarsha owns the Atlassian site, so if
Adarsha is Person 2 the site stays theirs and Person 3 is given project admin.

---

## Re-scope — the team is three, and the plan was built for more

The backlog is **126 points across 3 people — about 42 each**, where four would have been ~31. Cut
deliberately now rather than discovering it in Sprint 3. Cut in this order, all Low or Medium
priority and none of them load-bearing:

| Order | Story | Points | Why it is safe to cut |
|---|---|---|---|
| 1 | 7.7 Performance and capacity benchmark | 5 | Low priority. State the targets in the SRS, measure only if time allows. |
| 2 | 6.4 View the Audit Log | 2 | Low. The log is still written (6.1) and reconciled (6.2); only the viewer goes. |
| 3 | 3.4 Current account overdraft | 3 | Medium. Savings accounts alone demonstrate the withdrawal path. |
| 4 | 1.5 Residual transfer on closure | 5 | Medium, and it is the awkward two-owner interface. Closure at zero balance still works. |
| 5 | 4.3 Date-range statement | 3 | Medium. The mini-statement (4.2) already proves the read path. |

Cutting all five drops the backlog to **108 points, ~36 each**. Do **not** cut 6.2 (reconciliation),
7.1 (money type) or 7.2 (crash-safe journal) — those are the requirements the project is actually
judged on.

---

## Interfaces that span two owners

Agree these before either side writes code.

| Boundary | Between | Why it matters |
|---|---|---|
| `BMS-F-006` residual transfer | P1 `AccountModule` ↔ P2 `TransactionModule` | Closure calls transfer. Agree the signature and who owns atomicity. |
| `BMS-F-060` journal format | P2 writes ↔ P3 reads for statements | Fix the on-disk record format **first** — P3 is blocked on it. |
| `BMS-F-063` audit entries | P1 writes on auth events ↔ P3 displays | One append helper, owned by P2's persistence layer. |
| `BMS-SR-007` role check | P1 `AuthModule` ↔ everyone | The check lives in the service layer, so every module calls P1's. |

---

## Still unowned

**The CLI / menu layer.** Every module needs a menu entry. It currently sits with Person 3 under
story 7.6 by default. Either confirm that, or make it a shared file each person extends for their own
module — the second is usually less painful, as long as everyone stays out of each other's sections.

---

## Working agreement

| | |
|---|---|
| Default branch | `main`, protected — no direct pushes |
| Branch naming | `docs/<topic>`, `feat/<module>`, `fix/<topic>` |
| PR approvals | 1 required, and not from the PR author |
| Requirement changes | Person 1 approves; add a row to the SRS revision history |
| Generated files | Never hand-edit `docs/SRS.md`, `docs/RTM.md` or the `.docx` — edit `tools/srs_content.py` and rebuild |
| Naming | Jira issue = `BMS-12`. Requirement = `BMS-F-012`. Never write `BMS-12` meaning a requirement. |
| C/C++ build gate | `g++ -std=c++17 -Wall -Wextra -Werror` must pass before review (BMS-NF-004) |
| Banned functions | `gets`, `strcpy`, `strcat`, `sprintf`, unbounded `scanf("%s")` (BMS-SR-002) |
| Money type | `int64_t` paise only. A `float` or `double` in the money path is an automatic request for changes (BMS-NF-003) |
| UI | Line-based menu for v1.0 so tests stay scriptable. A full TUI is a Sprint 4 stretch behind the service layer. |
