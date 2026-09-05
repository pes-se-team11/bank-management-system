# Podcast Guest Scheduling & Outline Builder

**PES University — Dept. of CSE · Software Engineering Mini Project**
**Problem Statement #60 — Media, Events & Community**

A media production organizer that lets podcast hosts publish booking availability, collect guest
talking-point outlines, and generate timestamped run-of-show episode production notes.

> **Scenario source:** Problem Statement #60, from the *Scenarios for LAB 1-3* pack. This repository
> continues the same system modelled in Lab 1 (requirements + use cases) and Lab 2 (Jira backlog),
> so every artefact here traces back to a numbered Lab 1 requirement.

---

## Team & roles

| Role | Member | SRN | Owns |
|---|---|---|---|
| Requirements Lead / Repo Maintainer | Dhanush S | PES1UG24AM360 | SRS, RTM, branch protection, merges |
| Design Lead | _<member 2>_ | _<SRN>_ | SAD, UML class & sequence diagrams, API design |
| QA / Test Lead | _<member 3>_ | _<SRN>_ | Test Plan, test cases, defect triage |
| Jira & Traceability Lead | _<member 4>_ | _<SRN>_ | Jira backlog, sprint board, requirement-to-story mapping |

Full responsibility breakdown: [`ROLES.md`](ROLES.md).

---

## Submission schedule

| # | Deliverable | Due | Status |
|---|---|---|---|
| 1 | Project SRS · Jira update started | **6 September 2025** | Drafted — [`docs/SRS.md`](docs/SRS.md) · [`.docx`](docs/SRS_Podcast_Guest_Scheduling_v1.0.docx) |
| 2 | Project Test Plan · complete Jira backlog | **15 September 2025** | Skeleton — [`docs/Test_Plan.md`](docs/Test_Plan.md) |
| — | Software Architecture & Design (SAD) | template supplied, date TBC | Not started |

---

## Repository layout

```
├── docs/
│   ├── SRS.md                                   # SRS, reviewable + diffable
│   ├── SRS_Podcast_Guest_Scheduling_v1.0.docx   # submission copy, template-styled
│   ├── Test_Plan.md                             # due 15 Sep
│   ├── Jira_Backlog.md                          # epics, stories, sprint plan
│   └── RTM.md                                   # requirement -> module -> test case
├── diagrams/
│   ├── UseCase_1_Scheduling.{svg,png,pdf}
│   └── UseCase_2_Production.{svg,png,pdf}
├── templates/                                   # instructor templates, unmodified
└── tools/
    ├── srs_content.py                           # single source of truth for the SRS
    ├── build_srs.py                             # renders SRS.md + .docx
    └── make_diagrams.py                         # renders both use-case diagrams
```

---

## Rebuilding the documents

The Markdown and the Word document are generated from one source
(`tools/srs_content.py`), so they cannot disagree. Edit the source, never the outputs.

```bash
pip install python-docx cairosvg
python tools/make_diagrams.py    # diagrams first, the SRS embeds them
python tools/build_srs.py        # -> docs/SRS.md + docs/SRS_...v1.0.docx
```

To produce a PDF of the SRS, open the `.docx` in Word and **File ▸ Export ▸ Create PDF/XPS**.

---

## Traceability chain

The property a grader checks is that nothing is invented from nowhere. Every requirement walks
back to the assigned problem statement:

```
PS #60 supplied FR-001 / NFR-001
      ↓
Lab 1   FR-001..005, NFR-001..002, UC-01..05 (+ «include» / «extend»)
      ↓
Lab 2   Epics 1-4, 11 user stories, 55 points, 2 sprints
      ↓
Mini project   PGS-F-001..052 (23) · PGS-NF-001..007 (7) · PGS-SR-001..006 (6)
      ↓
RTM     requirement -> design spec -> module -> test case -> status
```

Requirements new to the mini project (not carried from Lab 1) are marked
`New in mini project` in the RTM Comments column, so the additions are visible rather than
smuggled in.

### Requirement ID scheme

| Prefix | Meaning | Range |
|---|---|---|
| `PGS-F-###` | Functional requirement | 001–052 |
| `PGS-NF-###` | Non-functional requirement | 001–007 |
| `PGS-SR-###` | Security requirement | 001–006 |
| `UC-##` | Use case | 01–15 |
| `TC-XXX-##` | Test case | by suite |
| `SO-#` | Security objective | 1–3 |

---

## Contributing

Branch, commit, open a PR — `main` is protected. See [`CONTRIBUTING.md`](CONTRIBUTING.md).
