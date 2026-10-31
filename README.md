# Bank Management System

**PES University — Dept. of CSE · Software Engineering Mini Project**
**Team 11 · Implementation language: C / C++**

A scaffold for a planned system handling core banking operations — deposit, withdrawal, balance
inquiry, funds transfer — together with customer and account records, an append-only ledger and an
audit trail. The current application only prints its version; banking operations, authentication,
and persistence are planned and are not implemented yet.

The application uses C++17 and the standard library, plus platform APIs where required. Tests use
doctest, a test-only third-party dependency downloaded during CMake configuration. Configure with
`-DBUILD_TESTING=OFF` to build without downloading test dependencies.

`src/core/money.h` is a placeholder. Checked money arithmetic is the first implementation task
after these scaffold corrections. The design decisions below describe planned behavior.

> **Scope note.** This is the *bank-side* system. ATM terminal hardware and self-service kiosk
> interfaces, inter-bank settlement (NEFT/RTGS/UPI), cheque clearing, loans and card issuance are
> explicitly out of scope — see [`docs/SRS.md`](docs/SRS.md) §1.2.

---

## Start here

**Everyone:** read [`ROLES.md`](ROLES.md) — who owns what, and which requirement IDs map to your
modules.

| If you are | Read these three, ignore the rest |
|---|---|
| **Vidit (P2)** — architecture & transactions | [`docs/SRS.md`](docs/SRS.md) §4.3–4.6 (your requirements) · [`ROLES.md`](ROLES.md) Person 2 · [`docs/Jira_Backlog.md`](docs/Jira_Backlog.md) Epics 3, 5, 7 |
| **Adarsha (P3)** — test plan & reporting | [`docs/Test_Plan.md`](docs/Test_Plan.md) · [`docs/SRS.md`](docs/SRS.md) §8 (RTM) · [`ROLES.md`](ROLES.md) Person 3 |
| **Dhanush (P1)** — requirements & auth | [`tools/srs_content.py`](tools/srs_content.py) (the SRS source) · [`ROLES.md`](ROLES.md) Person 1 |

**`tools/` is Person 1's build machinery — nobody else needs to open it.** The SRS and its diagrams
are generated from there; see *Rebuilding the documents* below.

---

## Team & roles

| Person | Name | SRN | GitHub | Documentation | Code | Points |
|---|---|---|---|---|---|---|
| 1 | Dhanush S | PES1UG24AM360 | `@dhanushs1912-svg` | SRS, RTM, use-case diagrams | `AccountModule`, `AuthModule`, `ValidationModule` | 43 |
| 2 | Vidit Soni | PES1UG24AM318 | `@itsvidit1702` | SAD, sequence diagrams, threat model | `TransactionModule`, `PersistenceModule` | 48 |
| 3 | Adarsha E | PES1UG24AM334 | `@Adarsh-031` | Test Plan, test cases, Jira board | `LedgerModule` (read), `ReportModule` | 35 |

Team of three. `PES1UG24AM305` has moved to another team. Adarsha owns the Jira site
(`pes1ug24am334.atlassian.net`, project key `BMS`).

Full responsibility breakdown: [`ROLES.md`](ROLES.md). **Names and GitHub handles still to be filled in.**

---

## Submission schedule

| # | Deliverable | Due | Status |
|---|---|---|---|
| 1 | Project SRS · Jira update started | **6 September 2026** | Done — [`docs/SRS.md`](docs/SRS.md) · [`.docx`](docs/SRS_Bank_Management_System_v1.0.docx) |
| 2 | Project Test Plan | **2 October 2026** | Submission draft — [`docs/Test_Plan.md`](docs/Test_Plan.md) · [`.docx`](docs/Test_Plan_Bank_Management_System_v1.0.docx); review pending |
| 3 | Software Architecture & Design (SAD) | **30 September 2026** | Done — [`.docx`](docs/SAD_Bank_Management_System_v1.0.docx) · [traceability](docs/SAD_Traceability.md) |

---

## What's in the SRS

| Template minimum | Delivered |
|---|---|
| ≥ 15 functional requirements | **29** (`BMS-F-001`…`064`, seven feature areas) |
| ≥ 5 non-functional requirements | **7** (`BMS-NF-001`…`007`) |
| ≥ 2 security objectives | **4** (`SO-1`…`SO-4`) |
| ≥ 5 security requirements | **7** (`BMS-SR-001`…`007`) |
| ≥ 2 UML use-case diagrams | **2** — 18 use cases, 8 `«include»`, 2 `«extend»` |
| RTM | **43 rows**, every one naming a test case |

---

## Repository layout

```
README.md          <- you are here
ROLES.md           <- who owns what
CONTRIBUTING.md    <- branch, commit, PR

docs/              <- the deliverables
├── SRS.md                                # readable + diffable
├── SRS_Bank_Management_System_v1.0.docx  # the submission copy
├── Test_Plan.md                          # editable submission draft, due 2 Oct
├── Test_Plan_Bank_Management_System_v1.0.docx  # Word submission copy
├── SAD_Bank_Management_System_v1.0.docx  # architecture & design (Person 2)
├── SAD_Traceability.md                   # requirement -> component input for the SAD
└── Jira_Backlog.md                       # 7 epics, 29 stories (live in Jira as BMS-1..BMS-36)

diagrams/          <- editable SVG source for the two use-case diagrams

tools/             <- Person 1 only: generates docs/ and diagrams/
├── srs_content.py       # single source of truth for the SRS
├── build_srs.py         # -> SRS.md + .docx
├── make_diagrams.py     # -> the SVGs, and PNGs into build/ (gitignored)
└── srs_template.docx    # instructor's template, used only for its styles
```

---

## Building the code

Requires CMake 3.20 or newer and a C++17 compiler. CI uses GCC on Linux and MSVC on Windows.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel 2
ctest --test-dir build -C Debug --output-on-failure
bash tools/check_banned.sh
./build/bms
```

On Windows with the default Visual Studio generator, run `.\build\Debug\bms.exe`.
For an application-only build, add `-DBUILD_TESTING=OFF` to the configure command.

The module layout below includes planned implementation areas:

```
src/platform/    the ONLY place OS-specific code lives (BMS-NF-004)
src/core/        money type, error codes, shared helpers
src/persistence/ journal + account store        (Vidit)
src/account/ src/auth/                           (Dhanush)
src/transaction/                                 (Vidit)
src/ledger/ src/report/                          (Adarsha)
src/cli/         line-based menu; main.cpp
tests/           doctest unit tests (fetched by CMake), one test_<module>.cpp per module
data/            planned runtime data directory
```

Core implementation files are explicitly listed in `bms_core`; CLI files belong in `bms`,
so tests can link the core without the menu (BMS-NF-006). CI builds on Ubuntu and Windows/MSVC
when a PR targeting `main` is opened, updated, reopened, or merged.

---

## Rebuilding the documents

`docs/SRS.md` and the `.docx` are **generated** from `tools/srs_content.py`, so they
cannot disagree with each other. Edit the source, never the outputs.

```bash
pip install python-docx cairosvg
python tools/make_diagrams.py    # diagrams first — the SRS embeds them
python tools/build_srs.py        # -> docs/SRS.md, docs/SRS_...v1.0.docx
```

For a PDF of the SRS, open the `.docx` in Word and **File ▸ Export ▸ Create PDF/XPS**.

---

## Requirement ID scheme

| Prefix | Meaning | Range |
|---|---|---|
| `BMS-F-###` | Functional requirement | 001–064, numbered in blocks of ten per feature area |
| `BMS-NF-###` | Non-functional requirement | 001–007 |
| `BMS-SR-###` | Security requirement | 001–007 |
| `SO-#` | Security objective | 1–4 |
| `UC-##` | Use case | 01–18 |
| `TC-XXX-##` | Test case | by suite (`ACC`, `AUT`, `DEP`, `WDR`, `BAL`, `TRF`, `LED`, `PERF`, `REL`, `SEC`, `UX`, `PORT`) |

Blocks of ten leave room to insert a requirement into a feature area later without renumbering
everything after it — which would otherwise invalidate every cross-reference in the RTM.

---

## Three planned design decisions

1. **Money will use `int64_t` paise, never `float` or `double`** (`BMS-NF-003`, `BMS-SR-003`). Binary
   floating point cannot represent decimal currency exactly. Every addition and subtraction will be
   overflow-checked before it happens and refuse rather than wrap.
2. **The journal will be append-only** (`BMS-F-060`, `BMS-SR-005`). Persistence will never rewrite an
   existing record. Planned reconciliation (`BMS-F-061`) will compare balances against the journal
   to detect tampering.
3. **Role checks will live in the service layer, not the menu** (`BMS-SR-007`). A menu that hides an option
   is a convenience; it is not a security boundary.

---

## Contributing

Branch, commit, open a PR — `main` is protected. See [`CONTRIBUTING.md`](CONTRIBUTING.md).
