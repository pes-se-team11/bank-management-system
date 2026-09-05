# Team roles & responsibilities

**Team 11 — Bank Management System (C/C++)**

Assigned per mini-project instruction #4. Every deliverable has exactly one accountable owner;
everyone reviews. Fill in names and GitHub handles below, then mirror the same assignment in Jira so
the board and the repository agree.

---

## 1. Requirements Lead / Repository Maintainer

**PES1UG24AM360 — Dhanush S · `@dhanushs1912-svg`**

- Owns `docs/SRS.md`, `docs/RTM.md` and the generated `.docx` submission copy
- Owns the `BMS-F` / `BMS-NF` / `BMS-SR` ID scheme and the block-of-ten numbering convention
- Administers the GitHub organisation and repository: collaborator invites, branch protection, merges
- Final reviewer on any PR that changes a requirement ID or its acceptance criteria

## 2. Design Lead

**PES1UG24AM305 — _<name>_ · `@<handle>`**

- Owns the Software Architecture & Design document against `templates/SAD_Template.docx`
- Produces the UML class diagram and at least two sequence diagrams for flows specific to this
  project. Recommended pair: **funds transfer** (the two-leg atomic write, `BMS-F-050`) and
  **withdrawal with lockout** (`BMS-F-030` + `BMS-F-011`) — between them they exercise every layer.
- Owns the module interface definitions: `AuthModule`, `AccountModule`, `TransactionModule`,
  `LedgerModule`, `ReportModule`, `ValidationModule`, `PersistenceModule`
- Owns the STRIDE threat model, tracing to `SO-1`…`SO-4`
- Owns the on-disk file formats and the write-ahead journal design behind `BMS-NF-002`

## 3. QA / Test Lead

**PES1UG24AM318 — _<name>_ · `@<handle>`**

- Owns `docs/Test_Plan.md` against `templates/Test_Plan_Template for SE.docx` — **due 15 Sept**
- Writes the test cases behind every `TC-` id referenced in an SRS acceptance criterion
- Owns entry/exit criteria, defect logging and the test summary report
- Owns the two hardest tests in the project: `TC-REL-01` (kill the process mid-write and prove
  recovery) and `TC-TRF-01` (inject a failure between transfer legs and prove neither applied)

## 4. Jira & Traceability Lead

**PES1UG24AM334 — _<name>_ · `@<handle>`**

- Owns the Jira board: epics, stories, story points, sprints — **backlog complete by 15 Sept**
- Keeps Jira and `docs/Jira_Backlog.md` in agreement
- Verifies every SRS requirement maps to at least one story and one test case, and raises the gap
  when one does not
- Owns sprint reports and burndown evidence for the submission

---

## Shared responsibilities

Everyone reviews at least one PR per week, keeps their own Jira items current, and raises a blocker
the day it appears rather than at the sprint boundary.

## Working agreement

| | |
|---|---|
| Default branch | `main`, protected — no direct pushes |
| Branch naming | `docs/<topic>`, `feat/<module>`, `fix/<topic>` |
| PR approvals | 1 required, and not from the PR author |
| Requirement changes | Requirements Lead approves; add a row to the SRS revision history |
| Generated files | Never hand-edit `docs/SRS.md`, `docs/RTM.md` or the `.docx` — edit `tools/srs_content.py` and rebuild |
| C/C++ build gate | `g++ -std=c++17 -Wall -Wextra -Werror` must pass before review (`BMS-NF-004`) |
| Banned functions | `gets`, `strcpy`, `strcat`, `sprintf`, unbounded `scanf("%s")` — rejected in review (`BMS-SR-002`) |
| Money type | `int64_t` paise only. A `float` or `double` in the money path is an automatic request for changes (`BMS-NF-003`) |
