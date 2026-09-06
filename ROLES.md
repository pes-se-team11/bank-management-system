# Team roles & responsibilities

**Team 11 — Bank Management System (C/C++)**

Current split is **3-way**: each person owns one documentation deliverable and the code area that
document describes. Nobody writes a document about code they have never touched.

> **Status: provisional.** The official Team 11 roster lists four SRNs — `PES1UG24AM305`,
> `PES1UG24AM318`, `PES1UG24AM334`, `PES1UG24AM360`. One member is asking the course coordinator
> about moving teams, so the team may end up at three or four. Until the coordinator confirms
> otherwise, the roster stands at four and this file records the 3-way working split. See
> **Open questions** at the bottom — two of them need answering before 15 September.

---

## Person 1 — Requirements & Auth/Account module

**PES1UG24AM360 — Dhanush S · `@dhanushs1912-svg`**

**Documentation — the SRS**
Functional requirements (account creation, login, deposit, withdrawal, balance inquiry,
mini-statement, admin functions), non-functional requirements, security objectives and requirements,
use-case diagrams, and the RTM.

**Code — `AccountModule`, `AuthModule`, `ValidationModule`**
Account creation, login by PIN or password, input validation.

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-001, F-002, F-005 | Customer record, account opening, lookup | 1.1, 1.2 |
| BMS-F-003, F-004 | Modify customer details, close account | 1.3, 1.4 |
| BMS-F-006 | Residual transfer on closure — **shared with Person 2** | 1.5 |
| BMS-F-010, F-014 | Authenticate customer and staff; no terminal echo | 2.1 |
| BMS-F-011, F-013 | Lockout after 3 failures; manager unlock | 2.2, 2.4 |
| BMS-F-012, BMS-SR-007 | Role-based authorisation, enforced in the service layer | 2.3 |
| BMS-SR-001 | Salted credential hashing | 7.3 |
| BMS-SR-002, BMS-SR-004 | Bounds-checked buffers, input validation | 7.4 |

**43 story points.** Sprint 1: stories 1.1, 1.2, 7.3.

---

## Person 2 — Architecture/Design & Transaction module

**SRN `<to confirm>` · `@<handle>`**

**Documentation — the SAD**
Architecture pattern (layered, as in the worked example), component diagram, two or more sequence
diagrams — **withdraw** and **transfer** are the right two, because between them they exercise every
layer — API and interface definitions between modules, technology stack, threat model against
`SO-1`…`SO-4`.

**Code — `TransactionModule`, `PersistenceModule`**
Deposit, withdrawal, balance update, fund transfer, and file persistence.

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-020, F-021, F-022 | Deposit, rejection cases, journal entry | 3.1 |
| BMS-F-030, F-031 | Withdrawal limits; atomic debit plus journal | 3.2 |
| BMS-F-032, F-033 | Savings minimum balance; Current overdraft | 3.3, 3.4 |
| BMS-F-050, F-051, F-052 | Atomic two-leg transfer, rejection cases, ceilings | 5.1, 5.2 |
| BMS-NF-003, BMS-SR-003 | Integer-paise money type, overflow checks | 7.1 |
| BMS-NF-002, BMS-SR-005 | Crash-safe write-ahead journal, append-only files | 7.2 |
| BMS-SR-006 | Owner-only file permissions, instance lock | 7.5 |

**48 story points** — the largest share, and it includes the two hardest requirements in the project
(`BMS-NF-002` crash-safety and `BMS-F-050` transfer atomicity). Sprint 1: stories 7.1, 7.2.

---

## Person 3 — Test Plan & Reporting/Admin module

**SRN `<to confirm>` · `@<handle>`**

**Documentation — the Test Plan**
Test items, features to and not to be tested, test levels (unit / integration / system), entry and
exit criteria, schedule, risk table, and **linking Person 1's requirement IDs to real test-case
IDs**.

**Code — `LedgerModule` (read side), `ReportModule`**
Transaction history and mini-statement, admin and reporting functions. Also executes the test cases
against Persons 1 and 2's modules once integration starts.

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-040, F-041, F-042 | Balance, mini-statement, date-range statement | 4.1, 4.2, 4.3 |
| BMS-F-060, F-063 | Append-only journal and audit capture — **shared with Person 2** | 6.1 |
| BMS-F-061 | Reconciliation check | 6.2 |
| BMS-F-062, F-064 | End-of-day report, audit log view | 6.3, 6.4 |
| BMS-NF-004, F-005, F-006 | Portable warning-free build, layered modules, error messages | 7.6 |
| BMS-NF-001, BMS-NF-007 | Performance and capacity benchmark | 7.7 |

**35 story points.**

---

## Interfaces that span two owners

Agree these before either side starts writing code. They are where integration will hurt.

| Boundary | Between | Why it matters |
|---|---|---|
| `BMS-F-006` residual transfer on closure | P1 `AccountModule` ↔ P2 `TransactionModule` | Closure calls transfer. Agree the function signature and who owns the atomicity. |
| `BMS-F-060` journal writes vs reads | P2 writes entries ↔ P3 reads them for statements | Agree the on-disk record format **first**. Person 3 cannot write `BMS-F-041` until it is fixed. |
| `BMS-F-063` audit entries | P1 writes them on auth events ↔ P3 displays them | Same file, two writers. One append helper, owned by P2's persistence layer. |
| Role check placement | P1 `AuthModule` ↔ everyone | `BMS-SR-007` says the check is in the service layer, not the menu. Every module must call it. |

---

## Working agreement

| | |
|---|---|
| Default branch | `main`, protected — no direct pushes |
| Branch naming | `docs/<topic>`, `feat/<module>`, `fix/<topic>` |
| PR approvals | 1 required, and not from the PR author |
| Requirement changes | Person 1 approves; add a row to the SRS revision history |
| Generated files | Never hand-edit `docs/SRS.md`, `docs/RTM.md` or the `.docx` — edit `tools/srs_content.py` and rebuild |
| C/C++ build gate | `g++ -std=c++17 -Wall -Wextra -Werror` must pass before review (BMS-NF-004) |
| Banned functions | `gets`, `strcpy`, `strcat`, `sprintf`, unbounded `scanf("%s")` — rejected in review (BMS-SR-002) |
| Money type | `int64_t` paise only. A `float` or `double` in the money path is an automatic request for changes (BMS-NF-003) |

---

## Open questions

1. **Nobody owns the Jira board.** The 3-way split assigns three documents and three code areas, but
   *"complete Jira backlog"* is a graded deliverable on **15 September**. One member was building the
   board; that needs writing down here against a name, or it will be nobody's job on the 14th.
   Suggested owner: **Person 3**, since they already own the RTM and the requirement-to-test mapping,
   and the backlog is the third leg of that same traceability chain.
2. **Nobody owns the `CLI / MenuLayer` or the build system.** Every module needs a menu entry, and
   `BMS-NF-004`/`005` are currently parked with Person 3 under story 7.6. Either confirm that, or
   make the menu a shared file each person extends for their own module.
3. **If the team stays at three, the plan needs re-scoping, not just redistributing.** The backlog is
   126 points. Across four people that is ~31 each; across three it is ~42 each — a 35% increase per
   person. Stories 7.6, 7.7, 1.5, 3.4 and 6.4 are the Low/Medium-priority items to cut first. Decide
   that deliberately rather than discovering it in Sprint 3.
4. **Person 2 is carrying the most** — 48 points, the SAD, and the two hardest requirements. Worth
   moving story 7.5 (file permissions, instance lock) to Person 1, who already owns the security
   requirements, which would even it to 43 / 43 / 40.
