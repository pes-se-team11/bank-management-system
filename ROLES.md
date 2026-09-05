# Team roles & responsibilities

Assigned per mini-project instruction #4. Every deliverable has exactly one owner who is
accountable for it being complete and on time; everyone reviews.

Fill in the names, SRNs and GitHub handles below, then mirror the same assignment in Jira so the
board and the repository agree.

---

## 1. Requirements Lead / Repository Maintainer

**Member:** Dhanush S · PES1UG24AM360 · `@dhanushs1912-svg`

- Owns `docs/SRS.md` and the generated `.docx` submission copy
- Owns the Requirements Traceability Matrix and the `PGS-F` / `PGS-NF` / `PGS-SR` ID scheme
- Administers the GitHub organisation and repository: collaborator invites, branch protection,
  merge decisions on `main`
- Final reviewer on any PR that changes a requirement ID or its acceptance criteria

## 2. Design Lead

**Member:** _<name>_ · _<SRN>_ · `@<handle>`

- Owns the Software Architecture & Design document against `templates/SAD_Template.docx`
- Produces the UML class diagram and at least two sequence diagrams for flows specific to this
  project (booking confirmation, and outline approval → run-of-show generation)
- Owns the API design section: endpoint definitions for at least two components
- Owns architecture decision records and the STRIDE threat model, tracing to `SO-1..SO-3`

## 3. QA / Test Lead

**Member:** _<name>_ · _<SRN>_ · `@<handle>`

- Owns `docs/Test_Plan.md` against `templates/Test_Plan_Template for SE.docx` — **due 15 Sept**
- Writes the test cases behind every `TC-` id referenced in the SRS acceptance criteria
- Owns entry/exit criteria, defect logging and the test summary report
- Reviews every PR for whether the change invalidates an existing test case

## 4. Jira & Traceability Lead

**Member:** _<name>_ · _<SRN>_ · `@<handle>`

- Owns the Jira board: epics, stories, story points, sprints — **backlog complete by 15 Sept**
- Keeps the Jira backlog and `docs/Jira_Backlog.md` in agreement
- Verifies that every SRS requirement maps to at least one Jira story and one test case, and
  raises the gap when one does not
- Owns sprint reports and burndown evidence for the submission

---

## Shared responsibilities

Everyone: reviews at least one PR per week, keeps their own Jira items current, and raises a
blocker the day it appears rather than at the sprint boundary.

## Working agreement

| | |
|---|---|
| Default branch | `main`, protected — no direct pushes |
| Branch naming | `docs/<topic>`, `feat/<topic>`, `fix/<topic>` |
| PR approvals | 1 required, and not from the PR author |
| Requirement changes | Requirements Lead must approve; bump the SRS revision history |
| Generated files | Never hand-edit `docs/SRS.md` or the `.docx` — edit `tools/srs_content.py` and rebuild |
