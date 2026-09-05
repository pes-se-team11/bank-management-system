# Contributing

## Branching

`main` is protected. Nothing lands on it except through a reviewed pull request.

```bash
git checkout main
git pull
git checkout -b docs/test-plan-section-5
```

| Prefix | Use for |
|---|---|
| `docs/` | SRS, SAD, Test Plan, README, diagrams |
| `feat/` | application code |
| `fix/` | defect fixes, with the defect id in the PR body |
| `chore/` | tooling, CI, repo housekeeping |

## Commits

One logical change per commit. Present tense, no trailing period:

```
Add security requirements BMS-SR-001..007
Fix overflow check in withdrawal debit path
```

Reference requirement or story ids where they apply — it makes traceability free at review time.

## Pull requests

1. Push your branch and open a PR against `main`.
2. Fill in the PR template — especially **which requirement or story this serves**.
3. Request one reviewer who is not you. The role owner for that artefact is the right default
   (see [`ROLES.md`](ROLES.md)).
4. Squash-merge once approved and CI is green.

## Editing the SRS

`docs/SRS.md` and `docs/SRS_Bank_Management_System_v1.0.docx` are **generated**. A PR that edits
either directly will be sent back.

```bash
# edit tools/srs_content.py, then:
python tools/make_diagrams.py
python tools/build_srs.py
git add tools/ docs/ diagrams/
```

Adding or changing a requirement means all four of these move together, or the change is
incomplete:

- the requirement row in `tools/srs_content.py`
- its acceptance criterion, naming a `TC-` id
- its row in the RTM
- the Jira story that carries the work

Any change to a requirement's **id, wording or acceptance criteria** also needs a new row in the
SRS revision history and approval from the Requirements Lead.

## Diagrams

Diagrams are generated from `tools/make_diagrams.py`, not drawn by hand. Change the layout tables
in that file and re-run it, so the `.svg`, `.png` and `.pdf` stay in step.
