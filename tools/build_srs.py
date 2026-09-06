# -*- coding: utf-8 -*-
"""Render the SRS to docs/SRS.md and a template-styled .docx.

    python tools/build_srs.py

The .docx is built on top of the instructor's SRS_Template for SE.docx so it
inherits that file's page setup, heading styles and table style. Content comes
from tools/srs_content.py, so the Markdown and the Word document cannot drift.
"""
import os
import sys

import docx
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Inches, Pt

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import srs_content as C  # noqa: E402

TEMPLATE = os.path.join(ROOT, "templates", "SRS_Template for SE.docx")
DOCX_OUT = os.path.join(ROOT, "docs", "SRS_Bank_Management_System_v1.0.docx")
MD_OUT = os.path.join(ROOT, "docs", "SRS.md")

TEAM = C.TEAM   # defined alongside the requirements, in srs_content.py

# Column widths in twips; each row must total 8610 to match the template width.
W_FR = [860, 2370, 645, 645, 1150, 1935, 1005]
W_NFR = [1080, 3168, 1368, 864, 2130]
W_SR = [1080, 3600, 1080, 792, 2058]
W_RTM = [1224, 1944, 1512, 1512, 1224, 648, 546]
W_REV = [1080, 1296, 2304, 2880, 1050]
W_APR = [2160, 2448, 2592, 1410]


# --------------------------------------------------------------------------- document model

def build_blocks():
    """Return the ordered list of blocks that make up the SRS."""
    b = []
    add = b.append

    add(("h1", "Software Requirements Specification (SRS)"))
    add(("meta", None))

    add(("h2", "Revision history"))
    add(("table", W_REV, [
        ["Version", "Date", "Author", "Change summary", "Approval"],
        ["0.1", "04-09-2025", "Requirements Lead", "Skeleton raised against the SRS template; scope and actor set agreed by the team", "Draft"],
        ["1.0", "05-09-2025", "Team 11", f"Full SRS: {len(C.all_frs())} FRs, {len(C.NFRS)} NFRs, {len(C.SECURITY_OBJECTIVES)} security "
         f"objectives, {len(C.SECURITY_REQS)} security requirements, 2 use-case diagrams, "
         f"{len(C.RTM)}-row RTM", "Pending review"],
    ]))

    add(("h2", "Approvals"))
    add(("table", W_APR, [
        ["Role", "Name", "Signature / Email", "Date"],
        ["Course Coordinator", "", "", ""],
        ["Requirements (Person 1)", "Dhanush S (PES1UG24AM360)", "", ""],
        ["Design / Test (Person 2)", "Adarsha E (PES1UG24AM334)", "", ""],
        ["Design / Test (Person 3)", "Vidit Soni (PES1UG24AM318)", "", ""],
    ]))

    add(("h2", "Table of Contents"))
    add(("p", "1. Introduction\n2. Overall description\n3. External interface requirements\n"
              "4. System features (detailed)\n5. Non-functional requirements (detailed)\n"
              "6. Quality attributes & Acceptance tests\n7. System models and diagrams\n"
              "8. Requirements Traceability Matrix (RTM)"))

    # ---------------------------------------------------------------- 1
    add(("h2", "1. Introduction"))
    add(("p", "1.1 Purpose"))
    add(("p",
         "This document is the Software Requirements Specification for the Bank Management System built by "
         "Team 11. It states the functional and non-functional requirements, the external interfaces, the "
         "security objectives and the verification criteria for a system that handles core banking "
         "operations - deposit, withdrawal, balance inquiry, funds transfer and the account records behind "
         "them. It is the reference against which the C/C++ implementation, the design document and the test "
         "plan are written and assessed."))
    add(("p", "1.2 Scope"))
    add(("p",
         "The system maintains customer and account records for a single bank branch and performs the "
         "transactions against them: opening and closing accounts, authenticating operators, accepting "
         "deposits, paying out withdrawals within the applicable limits, transferring funds between accounts "
         "held at the bank, answering balance and statement queries, and keeping the ledger and audit trail "
         "that make every balance explicable.\n"
         "This is the bank-side system. Explicitly out of scope: ATM terminal hardware and self-service "
         "kiosk interfaces; inter-bank settlement (NEFT, RTGS, UPI); cheque clearing; loan origination and "
         "interest accrual; card issuance and EMV processing; internet and mobile banking front-ends; and "
         "statutory reporting to the central bank. Where an out-of-scope channel would ordinarily feed the "
         "bank, its transactions are assumed to arrive through the Teller interface."))
    add(("p", "1.3 Audience"))
    add(("p",
         "The Team 11 developers implementing the system in C/C++, the QA lead writing and executing the "
         "test plan, the course coordinator and evaluators assessing the deliverable, and any team member "
         "picking up an unfamiliar module later in the project."))
    add(("p", "1.4 Definitions"))
    add(("p",
         "Account - a ledger position held by a customer, of type Savings or Current, in one of the states "
         "ACTIVE, LOCKED or CLOSED.\n"
         "Journal (transaction journal) - the append-only record of every balance-changing operation.\n"
         "Audit log - the append-only record of every privileged, non-monetary action.\n"
         "Paise - the smallest currency unit; all monetary values are stored as integer paise so that no "
         "rounding error can arise.\n"
         "Reconciliation - the check that an account's balance equals its opening balance plus the sum of "
         "its journal entries.\n"
         "Minimum balance - the floor a Savings account may not be taken below by a withdrawal.\n"
         "Overdraft - a sanctioned limit by which a Current account may go negative.\n"
         "Session - the period between a successful authentication and the operator signing out.\n"
         "Acronyms: SRS (Software Requirements Specification), FR (Functional Requirement), NFR "
         "(Non-Functional Requirement), SR (Security Requirement), SO (Security Objective), RTM "
         "(Requirements Traceability Matrix), UC (Use Case), TC (Test Case), CLI (Command Line Interface), "
         "PIN (Personal Identification Number), RBAC (Role-Based Access Control), I/O (Input/Output), UAT "
         "(User Acceptance Testing)."))

    # ---------------------------------------------------------------- 2
    add(("h2", "2. Overall description"))
    add(("p", "2.1 Product perspective"))
    add(("p",
         "The product is a new, self-contained console application, delivered as a single executable built "
         "from C/C++ sources and persisting its data in local files. It is not a component of a larger "
         "system and it consumes no external service. It replaces manual ledgers and spreadsheets at a "
         "single branch counter.\n"
         "The absence of a network is the defining architectural fact about this system. It removes an "
         "entire class of threats, and in exchange it places the whole of the confidentiality obligation on "
         "operating-system file permissions and on how credentials are stored at rest, which is why section "
         "5.1 is weighted towards those concerns rather than towards transport security."))
    add(("p", "2.2 Major product functions (detailed)"))
    add(("p",
         "- Create customer records and open Savings or Current accounts against them\n"
         "- Modify customer contact details, with the previous value retained\n"
         "- Close an account, transferring any residual balance out first\n"
         "- Authenticate customers by PIN and staff by password, and lock an account after repeated failures\n"
         "- Enforce role-based authorisation across Customer, Teller and Manager\n"
         "- Accept deposits and pay out withdrawals within balance, minimum-balance, overdraft and daily limits\n"
         "- Transfer funds between two accounts held at the bank, atomically\n"
         "- Answer balance inquiries and produce mini-statements and date-range statements\n"
         "- Append every balance change to a transaction journal and every privileged action to an audit log\n"
         "- Reconcile stored balances against the journal and detect tampering\n"
         "- Generate the end-of-day report and let a Manager inspect the audit log"))
    add(("p", "2.3 User roles and characteristics (expanded)"))
    add(("p",
         "- Customer: an account holder, of no assumed technical skill, authenticating with an account "
         "number and PIN. Operates only on their own accounts and only through the menu. Uses the system "
         "occasionally and needs every prompt and refusal to be self-explanatory.\n"
         "- Bank Teller: counter staff, the heaviest user of the system, working through it continuously "
         "during banking hours. Opens accounts, records customer detail changes and performs transactions on "
         "a customer's behalf. Values speed of entry and unambiguous error messages over guidance.\n"
         "- Bank Manager: supervisory staff, accountable for the branch reconciling at end of day. Closes "
         "accounts, unlocks locked accounts, runs the end-of-day report and inspects the audit log. Uses the "
         "system briefly but holds its most dangerous privileges, which is why those privileges are "
         "separated in BMS-F-012.\n"
         "There are no system actors. The system integrates with nothing, so every actor in section 7 is a "
         "human operator."))
    add(("p", "2.4 Operating environment"))
    add(("p",
         "A desktop or laptop computer running Windows or Linux, operated from a terminal. Built with g++ "
         "targeting C++17, using MinGW on Windows, with no dependency beyond the C++ standard library and "
         "the platform C runtime. Data is held in files in a single data directory alongside the executable.\n"
         "The system is designed for single-instance operation. Two instances running against one data "
         "directory would race on the account file, so a lock file prevents the second from starting."))
    add(("p", "2.5 Constraints"))
    add(("p",
         "- The implementation language is fixed to C/C++ by the course, and no third-party library may be "
         "used; this rules out an embedded database and makes the file format and its crash-safety the "
         "team's own responsibility.\n"
         "- No network interface may be assumed, so confidentiality rests on file permissions rather than on "
         "transport security.\n"
         "- Monetary values must be integer paise. Floating point is prohibited in the money path, because "
         "binary floating point cannot represent decimal currency exactly and a bank that mis-rounds is "
         "worthless.\n"
         "- Only one instance may run against a data directory at a time, enforced by a lock file.\n"
         "- The system is delivered by four students within one semester, which bounds it to a single "
         "console application with file persistence rather than a client-server deployment."))

    # ---------------------------------------------------------------- 3
    add(("h2", "3. External interface requirements"))
    add(("p", "3.1 User interfaces"))
    add(("p",
         "A menu-driven text console. The menu presented is determined by the authenticated role, so a "
         "Customer is never shown an operation they cannot perform - the menu is a convenience, not the "
         "security boundary, which is stated in BMS-SR-007. PIN and password entry is not echoed to the "
         "terminal (BMS-F-014). Every rejection states the reason and the corrective action on one line and "
         "returns the operator to the menu rather than terminating (BMS-NF-005). Output is formatted for an "
         "80-column terminal so that statements and reports remain aligned on a default console."))
    add(("p", "3.2 Hardware interfaces"))
    add(("p",
         "None beyond a standard keyboard and character display. The system drives no card reader, cash "
         "dispenser, deposit acceptor or receipt printer; those belong to the ATM channel, which section 1.2 "
         "places out of scope. Where a printed record is needed, the system writes a formatted text file "
         "that the operator may print through the operating system."))
    add(("p", "3.3 Software interfaces"))
    add(("p",
         "The system calls no external service API. Its software interfaces are:\n"
         "- The host filesystem, holding the account file, the append-only transaction journal, the audit "
         "log, the configuration file and the instance lock file.\n"
         "- The C++17 standard library, for I/O, containers and time.\n"
         "- The operating system, for two facilities the standard library does not provide portably: "
         "disabling terminal echo during credential entry, and setting owner-only file permissions "
         "(BMS-SR-006). Both are confined to a single portability header, per BMS-NF-004."))
    add(("p", "3.4 Communications"))
    add(("p",
         "None. The system performs no network input or output, opens no socket and listens on no port. "
         "This is a deliberate scope decision rather than an omission: it eliminates spoofing, "
         "man-in-the-middle and remote denial-of-service from the threat model, and concentrates the "
         "remaining risk on local file access and on the integrity of the stored ledger, which security "
         "objectives SO-1 and SO-2 address directly."))

    # ---------------------------------------------------------------- 4
    add(("h2", "4. System features (detailed)"))
    add(("p",
         "Each requirement below carries an acceptance criterion and a reference test case. IDs follow "
         "BMS-F-###, numbered in blocks of ten by feature area so that a requirement can be added to a block "
         "later without renumbering the rest. The Source column names the stakeholder the requirement "
         "answers to."))
    for title, desc, rows in C.FR_SECTIONS:
        add(("h3", title))
        add(("p", desc))
        add(("table", W_FR, [C.FR_HEADER] + rows))

    # ---------------------------------------------------------------- 5
    add(("h2", "5. Non-functional requirements (detailed)"))
    add(("p",
         "NFRs below are measurable and tied to the test plan. IDs follow BMS-NF-###. Two of them - "
         "BMS-NF-002 on crash-safety and BMS-NF-003 on integer currency - are properties the functional "
         "requirements silently depend on, and are stated here so that they are tested rather than assumed."))
    add(("table", W_NFR, [C.NFR_HEADER] + C.NFRS))

    add(("h2", "5.1. Security"))
    add(("h2", "5.1.1 Security Objectives"))
    add(("p",
         "Four security objectives govern this system. They are stated as the properties an attacker must "
         "not be able to violate. Every security requirement in 5.1.2 traces to at least one of them, and "
         "the trace is recorded in the RTM Comments column.\n"
         "The threat model is shaped by section 3.4: with no network, the adversary is someone with access "
         "to the host machine or to the data files, or an operator acting outside their role."))
    for title, text in C.SECURITY_OBJECTIVES:
        add(("p", title + "\n" + text))

    add(("h2", "5.1.2 Security Requirements"))
    add(("table", W_SR, [C.SR_HEADER] + C.SECURITY_REQS))

    # ---------------------------------------------------------------- 6
    add(("h2", "6. Quality attributes & Acceptance tests"))
    add(("p",
         "Exit criteria for acceptance: every High-priority functional requirement implemented and verified; "
         "no failing High-priority non-functional requirement; zero open critical or major defects; "
         "reconciliation (BMS-F-061) passing across the full test data set; and an RTM in which every "
         "requirement maps to at least one executed test case with a Pass result."))
    add(("p",
         "Acceptance test suites, each named for the test-case prefix it owns:\n"
         "- Account (TC-ACC-01..06) - customer creation, account opening, modification, closure, lookup, "
         "residual transfer\n"
         "- Authentication (TC-AUT-01..04) - credential check, lockout, role enforcement, unlock\n"
         "- Deposit (TC-DEP-01..03) - credit, rejection cases, journalling\n"
         "- Withdrawal (TC-WDR-01..03) - limits, minimum balance, overdraft\n"
         "- Balance & Statements (TC-BAL-01..03) - balance, mini-statement, date-range statement\n"
         "- Transfer (TC-TRF-01..03) - atomicity, rejection cases, ceilings\n"
         "- Ledger & Reporting (TC-LED-01..05) - append-only journal, reconciliation, end-of-day report, "
         "audit capture, audit view\n"
         "- Performance (TC-PERF-01..02) - transaction latency, capacity soak\n"
         "- Reliability (TC-REL-01) - crash-safety of the write-ahead journal\n"
         "- Security (TC-SEC-01..08) - credential storage, no echo, integer overflow, buffer safety, input "
         "validation, append-only enforcement, file permissions, service-layer role check\n"
         "- Usability (TC-UX-01) - every rejection states reason and remedy\n"
         "- Portability (TC-PORT-01..02) - clean build on both toolchains, modules unit-testable in isolation"))
    add(("p",
         "Quality attributes in priority order. Data integrity comes first: a banking system that loses or "
         "invents money has failed regardless of every other property, which is why BMS-NF-003 and "
         "BMS-F-061 are High priority. Security follows, because the data is personal and the ledger must "
         "be trustworthy. Reliability is third - a crash mid-transaction must not corrupt the ledger. "
         "Performance, usability and portability follow, in that order; none of them can be traded against "
         "the first three."))

    # ---------------------------------------------------------------- 7
    add(("h2", "7. System models and diagrams"))
    add(("h2", "7.1 UML Use-Case diagrams"))
    add(("p",
         "Two use-case diagrams model the system. Diagram 1 covers the account transactions a Customer or "
         "Teller performs; Diagram 2 covers account administration, audit and reporting. Together they carry "
         "eighteen use cases, eight «include» relationships and two «extend» relationships.\n"
         "Arrow direction follows UML: «include» points from the base use case to the included one; "
         "«extend» points from the extending use case to the base it extends. Two included use cases are "
         "deliberately shared by several bases - UC-07 Record Ledger Entry is included by Deposit, Withdraw "
         "Cash and Funds Transfer, and UC-15 Record Audit Entry by Close Account, Modify Customer Details "
         "and Unlock Locked Account. That sharing is the point of «include»: the behaviour is specified once "
         "and reused, which is also why BMS-F-060 and BMS-F-063 are each a single requirement rather than "
         "one per calling feature.\n"
         "The included and extending use cases (UC-06, UC-07, UC-08, UC-15, UC-16, UC-17, UC-18) carry no "
         "direct actor association, because they are entered from within a base use case rather than "
         "initiated on their own."))
    add(("image", os.path.join(ROOT, "diagrams", "UseCase_1_Transactions.png"),
         "Figure 1 - Use-Case Diagram 1: Account Transactions"))
    add(("image", os.path.join(ROOT, "diagrams", "UseCase_2_Administration.png"),
         "Figure 2 - Use-Case Diagram 2: Account Administration, Audit & Reporting"))
    add(("p",
         "Use-case inventory: UC-01 Deposit, UC-02 Withdraw Cash, UC-03 Funds Transfer, UC-04 Balance "
         "Inquiry, UC-05 Mini Statement, UC-06 Validate Sufficient Balance, UC-07 Record Ledger Entry, "
         "UC-08 Apply Overdraft, UC-09 Open Account, UC-10 Close Account, UC-11 Modify Customer Details, "
         "UC-12 Unlock Locked Account, UC-13 Generate End-of-Day Report, UC-14 View Audit Log, UC-15 Record "
         "Audit Entry, UC-16 Verify Zero Balance, UC-17 Authenticate User, UC-18 Transfer Residual Balance."))

    # ---------------------------------------------------------------- 8
    add(("h2", "8. Requirements Traceability Matrix (RTM)"))
    add(("p",
         "Status legend: N = Not started, P = Partially implemented, A = Accepted (implemented and test "
         "passed). Every requirement in sections 4, 5 and 5.1.2 appears exactly once. The Module column "
         "names the C/C++ module that will own the behaviour, and the Comments column names either the "
         "backlog story that carries the work or the security objective the requirement serves, so a "
         "requirement can be walked to its code, its test and its plan without leaving the repository."))
    add(("table", W_RTM, [C.RTM_HEADER] + C.RTM))
    return b


# --------------------------------------------------------------------------- docx rendering

def set_cell_text(cell, text, bold=False, size=8):
    cell.text = ""
    p = cell.paragraphs[0]
    p.paragraph_format.space_before = Pt(1)
    p.paragraph_format.space_after = Pt(1)
    run = p.add_run(text)
    run.bold = bold
    run.font.size = Pt(size)


def add_table(doc, widths, rows):
    t = doc.add_table(rows=len(rows), cols=len(rows[0]))
    tblPr = t._tbl.tblPr

    borders = OxmlElement("w:tblBorders")
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        el = OxmlElement("w:" + edge)
        el.set(qn("w:val"), "single")
        el.set(qn("w:sz"), "8")
        el.set(qn("w:space"), "0")
        el.set(qn("w:color"), "000000")
        borders.append(el)
    tblPr.append(borders)

    layout = OxmlElement("w:tblLayout")
    layout.set(qn("w:type"), "fixed")
    tblPr.append(layout)

    width = OxmlElement("w:tblW")
    width.set(qn("w:w"), str(sum(widths)))
    width.set(qn("w:type"), "dxa")
    tblPr.append(width)

    # Under a fixed layout Word lays out from tblGrid, so the grid has to carry
    # the same widths as the cells or the columns come out evenly split.
    grid = t._tbl.find(qn("w:tblGrid"))
    for col, w in zip(grid.findall(qn("w:gridCol")), widths):
        col.set(qn("w:w"), str(w))

    t.autofit = False
    for r_i, row in enumerate(rows):
        # Repeat the header row when a table breaks across pages.
        if r_i == 0:
            trPr = t.rows[0]._tr.get_or_add_trPr()
            trPr.append(OxmlElement("w:tblHeader"))
        for c_i, val in enumerate(row):
            cell = t.cell(r_i, c_i)
            cell.width = docx.shared.Twips(widths[c_i])
            set_cell_text(cell, str(val), bold=(r_i == 0))
    for c_i, w in enumerate(widths):
        for r in t.rows:
            r.cells[c_i].width = docx.shared.Twips(w)
    doc.add_paragraph()
    return t


def render_docx(blocks):
    doc = docx.Document(TEMPLATE)
    body = doc.element.body
    for child in list(body):
        if child.tag != qn("w:sectPr"):
            body.remove(child)

    # The template carries the instructor's identity in its document
    # properties. Overwrite them, or the submitted file names him as the
    # last person to edit it.
    cp = doc.core_properties
    cp.author = "; ".join(f"{n} ({srn})" for n, srn, _ in TEAM)
    cp.last_modified_by = TEAM[-1][0]
    cp.title = f"{C.META['title']} - {C.META['project']}"
    cp.subject = f"{C.META['team']} - PES University, Dept. of CSE"
    cp.category = "Software Engineering Mini Project"
    cp.comments = ""
    cp.keywords = "SRS, Bank Management System, Team 11"

    for blk in blocks:
        kind = blk[0]
        if kind in ("h1", "h2", "h3"):
            doc.add_paragraph(blk[1], style="Heading " + kind[1])
        elif kind == "meta":
            p = doc.add_paragraph()
            authors = "; ".join(f"{n} ({s})" for n, s, _ in TEAM)
            for i, line in enumerate([
                f"Project: {C.META['project']}",
                f"Team: {C.META['team']}",
                f"Implementation language: {C.META['language']}",
                f"Version: {C.META['version']}",
                f"Authors: {authors}",
                f"Date: {C.META['date']}",
                f"Status: {C.META['status']}",
            ]):
                if i:
                    p.add_run("\n")
                p.add_run(line)
        elif kind == "p":
            for chunk in blk[1].split("\n"):
                doc.add_paragraph(chunk)
        elif kind == "table":
            add_table(doc, blk[1], blk[2])
        elif kind == "image":
            doc.add_picture(blk[1], width=Inches(6.0))
            doc.paragraphs[-1].alignment = WD_ALIGN_PARAGRAPH.CENTER
            cap = doc.add_paragraph()
            cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
            run = cap.add_run(blk[2])
            run.italic = True
            run.font.size = Pt(9)

    os.makedirs(os.path.dirname(DOCX_OUT), exist_ok=True)
    doc.save(DOCX_OUT)
    return DOCX_OUT


# --------------------------------------------------------------------------- markdown rendering

def md_table(rows):
    def cell(v):
        return str(v).replace("|", "\\|").replace("\n", " ")
    out = ["| " + " | ".join(cell(c) for c in rows[0]) + " |",
           "|" + "|".join("---" for _ in rows[0]) + "|"]
    for r in rows[1:]:
        out.append("| " + " | ".join(cell(c) for c in r) + " |")
    return "\n".join(out)


def render_md(blocks):
    out = []
    for blk in blocks:
        kind = blk[0]
        if kind == "h1":
            out.append("# " + blk[1])
        elif kind == "h2":
            out.append("## " + blk[1])
        elif kind == "h3":
            out.append("### " + blk[1])
        elif kind == "meta":
            authors = "; ".join(f"{n} ({s})" for n, s, _ in TEAM)
            out.append(
                f"**Project:** {C.META['project']}  \n"
                f"**Team:** {C.META['team']}  \n"
                f"**Implementation language:** {C.META['language']}  \n"
                f"**Version:** {C.META['version']}  \n"
                f"**Authors:** {authors}  \n"
                f"**Date:** {C.META['date']}  \n"
                f"**Status:** {C.META['status']}")
        elif kind == "p":
            out.append(blk[1].replace("\n", "  \n"))
        elif kind == "table":
            out.append(md_table(blk[2]))
        elif kind == "image":
            rel = os.path.relpath(blk[1], os.path.dirname(MD_OUT)).replace("\\", "/")
            out.append(f"![{blk[2]}]({rel})\n\n*{blk[2]}*")
    text = ("<!-- Generated by tools/build_srs.py from tools/srs_content.py. "
            "Edit the source, not this file. -->\n\n" + "\n\n".join(out) + "\n")
    os.makedirs(os.path.dirname(MD_OUT), exist_ok=True)
    with open(MD_OUT, "w", encoding="utf-8") as fh:
        fh.write(text)
    return MD_OUT


def render_rtm():
    """Standalone RTM, generated from the same rows the SRS section 8 uses."""
    path = os.path.join(ROOT, "docs", "RTM.md")
    counts = {}
    for row in C.RTM:
        counts[row[5]] = counts.get(row[5], 0) + 1
    text = (
        "<!-- Generated by tools/build_srs.py. Edit tools/srs_content.py, not this file. -->\n\n"
        "# Requirements Traceability Matrix\n\n"
        f"**Project:** {C.META['project']}  \n"
        f"**SRS version:** {C.META['version']}  \n"
        f"**Date:** {C.META['date']}\n\n"
        "Identical to SRS section 8, extracted here so coverage can be reviewed without opening the "
        "full document. Status legend: **N** = not started, **P** = partially implemented, "
        "**A** = accepted (implemented and test passed).\n\n"
        + md_table([C.RTM_HEADER] + C.RTM) + "\n\n"
        "## Coverage summary\n\n"
        f"- Requirements tracked: **{len(C.RTM)}** "
        f"({len(C.all_frs())} functional, {len(C.NFRS)} non-functional, "
        f"{len(C.SECURITY_REQS)} security)\n"
        f"- Every row names at least one test case: "
        f"**{'yes' if all(r[4].strip() for r in C.RTM) else 'NO — fix before submission'}**\n"
        "- Status counts: " + ", ".join(f"`{k}` = {v}" for k, v in sorted(counts.items())) + "\n"
    )
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)
    return path


def main():
    blocks = build_blocks()
    print("wrote", render_md(blocks))
    print("wrote", render_rtm())
    print("wrote", render_docx(blocks))
    frs = C.all_frs()
    print(f"counts: {len(frs)} FRs, {len(C.NFRS)} NFRs, "
          f"{len(C.SECURITY_OBJECTIVES)} security objectives, "
          f"{len(C.SECURITY_REQS)} security requirements, {len(C.RTM)} RTM rows")


if __name__ == "__main__":
    main()
