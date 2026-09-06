# Team roles & responsibilities

**Team 11 — Bank Management System (C/C++)**

Assigned per mini-project instruction #4. Each person owns one documentation deliverable *and* one
code area, so nobody is only writing documents and nobody is only writing code.

---

## Person 1 — Requirements & Auth/Account module — **CONFIRMED**

**PES1UG24AM360 — Dhanush S · `@dhanushs1912-svg`**

**Documentation**
- Owns the SRS: functional requirements, non-functional requirements, security objectives and
  security requirements, use-case diagrams, and the RTM
- Owns the `BMS-F` / `BMS-NF` / `BMS-SR` ID scheme and the block-of-ten numbering convention
- Repository maintainer: collaborator invites, branch protection, merge decisions on `main`

**Code — `AccountModule`, `AuthModule`, `ValidationModule`**

| Requirement | What it is | Story |
|---|---|---|
| BMS-F-001, F-002, F-005 | Customer record, account opening, lookup | 1.1, 1.2 |
| BMS-F-003, F-004, F-006 | Modify details, close account, residual transfer | 1.3, 1.4, 1.5 |
| BMS-F-010, F-014 | Authenticate customer and staff; no echo | 2.1 |
| BMS-F-011, F-013 | Lockout after 3 failures; manager unlock | 2.2, 2.4 |
| BMS-F-012, BMS-SR-007 | Role-based authorisation, enforced in the service layer | 2.3 |
| BMS-SR-001 | Salted credential hashing | 7.3 |
| BMS-SR-002, BMS-SR-004 | Bounds-checked buffers, input validation | 7.4 |

**43 of 126 story points.** Sprint 1 items: 1.1, 1.2, 7.3.

---

## Person 2 — Jira board & backlog — **PARTIALLY CONFIRMED**

**SRN: `<PES1UG24AM3__>` · `@<handle>`**

- Owns the Jira board: project setup, epics, stories, points, sprints — **backlog complete by 15 Sept**
- Keeps Jira and [`docs/Jira_Backlog.md`](docs/Jira_Backlog.md) in agreement
- Verifies every SRS requirement maps to at least one story, and raises the gap when one does not
- Owns sprint reports and burndown evidence
- **Code area still to be agreed** — see the proposal below

---

## Persons 3 and 4 — **NOT YET AGREED**

The two remaining documentation deliverables are the **SAD** (template supplied) and the **Test
Plan** (due 15 Sept). The two remaining code areas are the **transaction path** and the
**ledger/reporting path**. A split that keeps each person's document and code aligned:

| | Documentation | Code | Requirements |
|---|---|---|---|
| **Proposed Person 3** | Software Architecture & Design — class diagram, 2 sequence diagrams, module interfaces, STRIDE threat model | `TransactionModule` | BMS-F-020…022, F-030…033, F-050…052 |
| **Proposed Person 4** | Test Plan, test cases, defect triage | `LedgerModule`, `ReportModule`, `PersistenceModule` | BMS-F-060…064, BMS-NF-002 |

Person 2's code area then falls out as whichever of these two the team prefers to share, or the
`CLI / MenuLayer` plus the build system (BMS-NF-004…006).

**This is a proposal, not a decision.** Agree it in the team chat and edit this file.

---

## Coordination points that will bite if ignored

1. **The RTM already names test-case IDs** (`TC-ACC-01`, `TC-AUT-01`, …). The brief said the RTM
   would be an ID-only shell with test cases filled in later — they are in already, as *placeholders*.
   Whoever owns the Test Plan must either write cases matching those IDs or tell Person 1 to change
   the RTM. Silently using different IDs breaks traceability, which is the single thing most likely
   to be checked.
2. **Sprint 1 has no user-visible feature**, by design — money type, crash-safe writes and credential
   hashing come first. Retrofitting an integer money type after deposit and withdrawal are written
   means rewriting them. Whoever presents the sprint review needs to be ready to defend this.
3. **`BMS-F-006` (residual transfer on closure) spans two owners** — Person 1's `AccountModule` and
   the transaction path. Agree the interface before either starts.

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
