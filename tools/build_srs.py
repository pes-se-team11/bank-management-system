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
DOCX_OUT = os.path.join(ROOT, "docs", "SRS_Podcast_Guest_Scheduling_v1.0.docx")
MD_OUT = os.path.join(ROOT, "docs", "SRS.md")

TEAM = [
    ("Dhanush S", "PES1UG24AM360", "Requirements Lead / Repo Maintainer"),
    ("<Team member 2>", "<SRN>", "Design Lead"),
    ("<Team member 3>", "<SRN>", "QA / Test Lead"),
    ("<Team member 4>", "<SRN>", "Jira & Traceability Lead"),
]

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
        ["0.1", "02-09-2025", "Dhanush S", "Skeleton raised from the Lab 1 requirements table and Lab 2 backlog", "Draft"],
        ["1.0", "05-09-2025", "Mini-project team", "Full SRS: 23 FRs, 7 NFRs, 3 security objectives, 6 security requirements, 2 use-case diagrams, RTM", "Pending review"],
    ]))

    add(("h2", "Approvals"))
    add(("table", W_APR, [
        ["Role", "Name", "Signature / Email", "Date"],
        ["Course Coordinator", "", "", ""],
        ["Requirements Lead", TEAM[0][0], "", ""],
        ["QA / Test Lead", TEAM[2][0], "", ""],
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
         "This document is the Software Requirements Specification for the Podcast Guest Scheduling & "
         "Outline Builder, the system defined by Problem Statement #60 (Media, Events & Community). It "
         "states the functional and non-functional requirements, the external interfaces, the security "
         "objectives and the verification criteria that the mini-project team will build and test against. "
         "It is the contract between the requirements captured in Lab 1, the backlog planned in Lab 2, and "
         "the design and test artefacts that follow."))
    add(("p", "1.2 Scope"))
    add(("p",
         "The system lets a Show Host publish recording availability, lets a Podcast Guest reserve a "
         "conflict-free interview slot and submit a structured talking-point outline, lets the Host vet that "
         "outline, and generates a timestamped run-of-show production sheet exportable as PDF. Calendar "
         "invites and reminders are dispatched through an external Calendar & Notification Service.\n"
         "Out of scope: audio or video recording, editing and hosting; payment or sponsorship management; "
         "transcript generation; publishing to podcast directories; and the internal implementation of the "
         "external calendar and email providers, which are consumed only through their published APIs."))
    add(("p", "1.3 Audience"))
    add(("p",
         "Mini-project developers, QA engineers, the course coordinator and evaluators, and any team "
         "member joining the repository later and needing the authoritative requirement set."))
    add(("p", "1.4 Definitions"))
    add(("p",
         "Episode board - the per-booking workspace holding the guest outline, host notes and generated "
         "run-of-show.\n"
         "Run-of-show - the ordered, timestamped list of segments the episode is recorded from.\n"
         "Slot - a bookable interval inside a published availability window.\n"
         "Talking point - one guest-supplied bullet of discussion material.\n"
         "Acronyms: SRS (Software Requirements Specification), FR (Functional Requirement), NFR "
         "(Non-Functional Requirement), RTM (Requirements Traceability Matrix), UC (Use Case), TLS "
         "(Transport Layer Security), HSTS (HTTP Strict Transport Security), IANA (Internet Assigned "
         "Numbers Authority), RBAC (Role-Based Access Control), WCAG (Web Content Accessibility "
         "Guidelines), DST (Daylight Saving Time), UTC (Coordinated Universal Time), SO (Security "
         "Objective), UAT (User Acceptance Testing)."))

    # ---------------------------------------------------------------- 2
    add(("h2", "2. Overall description"))
    add(("p", "2.1 Product perspective"))
    add(("p",
         "The product is a new, self-contained web application. It is not a replacement for an existing "
         "system. It sits between two human parties who today coordinate by email, and it depends on one "
         "external system boundary: a Calendar & Notification Service that delivers iCalendar invites, "
         "confirmations and reminders. That service is modelled as a supporting actor rather than as an "
         "internal component, because the acceptance criteria of the supplied FR-001 require a calendar "
         "invite to be sent, which places the delivery mechanism outside the system boundary."))
    add(("p", "2.2 Major product functions (detailed)"))
    add(("p",
         "- Publish and block Host recording availability\n"
         "- Present and reserve conflict-free interview slots\n"
         "- Reschedule a confirmed booking\n"
         "- Submit, draft and revise a structured talking-point outline\n"
         "- Attach biography and reference links to an outline\n"
         "- Review, approve, reject and reorder submitted talking points\n"
         "- Insert host-authored segments into the running order\n"
         "- Generate a timestamped run-of-show and recompute it on edit\n"
         "- Export the run-of-show as a formatted PDF production sheet\n"
         "- Dispatch calendar invites, confirmations and 24-hour reminders\n"
         "- Authenticate users and restrict episode boards to their two participants\n"
         "- Administer accounts and inspect the audit log"))
    add(("p", "2.3 User roles and characteristics (expanded)"))
    add(("p",
         "- Podcast Guest: an invited interviewee, often non-technical and in a different timezone from the "
         "Host. Interacts with the system a handful of times per episode and must be able to book and submit "
         "material without training or an account tutorial.\n"
         "- Show Host: the producer of the show. The heaviest user, returning weekly, and the only role that "
         "publishes availability, approves content and generates the production sheet.\n"
         "- Administrator: operates the deployment. Suspends abusive accounts and reads the audit log. Does "
         "not read episode content in the normal course of duty.\n"
         "- Calendar & Notification Service: supporting system actor. Accepts delivery requests and reports "
         "success or failure; has no user interface within this system."))
    add(("p", "2.4 Operating environment"))
    add(("p",
         "Server: a Linux host running the application server and a relational database, reachable over "
         "HTTPS. Client: current versions of Chrome, Edge, Firefox and Safari on desktop and mobile; no "
         "installed client software. The system must operate correctly for users in different IANA "
         "timezones simultaneously, including across daylight-saving transitions."))
    add(("p", "2.5 Constraints"))
    add(("p",
         "- The PDF export latency target of under one second is fixed by the supplied NFR-001 and "
         "constrains the choice of PDF library and the decision to render server-side.\n"
         "- All timestamps are stored in UTC; local time is a presentation concern only.\n"
         "- Episode boards are private to two named parties, which rules out shareable unguessable-link "
         "access as the sole protection and requires authenticated authorisation.\n"
         "- Calendar delivery is best-effort through a third-party provider; the system owns retry and "
         "audit, not delivery itself.\n"
         "- The project is delivered by a four-member student team inside one semester, which bounds the "
         "architecture to a single deployable application rather than a distributed service estate."))

    # ---------------------------------------------------------------- 3
    add(("h2", "3. External interface requirements"))
    add(("p", "3.1 User interfaces"))
    add(("p",
         "A responsive web UI with three surfaces: a public booking page reachable by link without an "
         "account, an authenticated episode board shared by Host and Guest, and a Host console for "
         "availability, review and run-of-show generation. All slot times are rendered in the viewer's "
         "local timezone with the timezone abbreviation shown explicitly. Booking and submission flows "
         "conform to WCAG 2.1 Level AA per PGS-NF-005."))
    add(("p", "3.2 Hardware interfaces"))
    add(("p",
         "None. The system requires no special-purpose hardware and interacts with no peripheral devices. "
         "It runs on commodity server hardware or a cloud virtual machine and is reached from standard "
         "personal computing devices."))
    add(("p", "3.3 Software interfaces"))
    add(("p",
         "- Calendar & Notification Service API: outbound HTTPS/JSON. Send iCalendar (RFC 5545) invites, "
         "confirmations and reminders; returns a per-message delivery status consumed by PGS-F-042.\n"
         "- Relational database: bookings, outlines, approvals and the append-only audit log. The unique "
         "constraint that makes PGS-F-004 correct lives here.\n"
         "- PDF rendering library: server-side, invoked by ExportService under the PGS-NF-001 latency "
         "budget.\n"
         "- IANA time zone database: timezone rules for PGS-NF-007."))
    add(("p", "3.4 Communications"))
    add(("p",
         "HTTPS with TLS 1.2 or higher for all browser and API traffic, with HSTS enforced (PGS-SR-001). "
         "Outbound notification calls retry with exponential backoff to a maximum of three attempts, after "
         "which a permanent failure is written to the audit log rather than silently dropped."))

    # ---------------------------------------------------------------- 4
    add(("h2", "4. System features (detailed)"))
    add(("p",
         "Each requirement below carries an acceptance criterion and a reference test case. IDs follow "
         "PGS-F-###. The Source column names the originating stakeholder and, where the requirement was "
         "carried forward, the Lab 1 requirement or Lab 1 use-case relationship it derives from."))
    for title, desc, rows in C.FR_SECTIONS:
        add(("h3", title))
        add(("p", desc))
        add(("table", W_FR, [C.FR_HEADER] + rows))

    # ---------------------------------------------------------------- 5
    add(("h2", "5. Non-functional requirements (detailed)"))
    add(("p", "NFRs below are measurable and tied to the test plan. IDs follow PGS-NF-###."))
    add(("table", W_NFR, [C.NFR_HEADER] + C.NFRS))

    add(("h2", "5.1. Security"))
    add(("h2", "5.1.1 Security Objectives"))
    add(("p",
         "Three security objectives govern this system. They are stated as the properties an attacker must "
         "not be able to violate, and each security requirement in 5.1.2 traces to at least one of them."))
    for title, text in C.SECURITY_OBJECTIVES:
        add(("p", title + "\n" + text))

    add(("h2", "5.1.2 Security Requirements"))
    add(("table", W_SR, [C.SR_HEADER] + C.SECURITY_REQS))

    # ---------------------------------------------------------------- 6
    add(("h2", "6. Quality attributes & Acceptance tests"))
    add(("p",
         "Exit criteria for acceptance: every High-priority functional requirement implemented and "
         "verified; no failing non-functional requirement in the High priority band; zero open critical or "
         "major defects; and an RTM in which every requirement maps to at least one executed test case with "
         "a Pass result."))
    add(("p",
         "Acceptance test suites, each named for the test-case prefix it owns:\n"
         "- Scheduling (TC-SCH-01..06) - availability, blocking, slot presentation, concurrency, "
         "reschedule, timezone correctness\n"
         "- Outline (TC-OUT-01..04) - talking points, links, drafts, episode-board attachment\n"
         "- Review (TC-REV-01..04) - approve, reject with comment, host segments, audit trail\n"
         "- Run-of-show (TC-ROS-01..04) - generation, timestamps, recompute, PDF export\n"
         "- Notification (TC-NOT-01..03) - invite, reminder, retry and permanent-failure logging\n"
         "- Authentication & Admin (TC-AUT-01..02, TC-ADM-01) - verification gate, board isolation, "
         "suspension and audit view\n"
         "- Performance (TC-PERF-01..03) - export latency, page latency, soak\n"
         "- Security (TC-SEC-01..06) - TLS/HSTS, password storage, authorisation, rate limiting, output "
         "escaping, audit immutability\n"
         "- Usability (TC-UX-01) - WCAG 2.1 AA conformance"))
    add(("p",
         "Quality attributes in priority order: security and confidentiality first, because the product "
         "holds unreleased commercial material; then correctness of scheduling, because a double-booked or "
         "mistimed slot destroys the product's core promise; then performance, bounded by the one-second "
         "export target; then usability and accessibility; then scalability, which the expected volume does "
         "not yet stress."))

    # ---------------------------------------------------------------- 7
    add(("h2", "7. System models and diagrams"))
    add(("h2", "7.1 UML Use-Case diagrams"))
    add(("p",
         "Two use-case diagrams model the system. Diagram 1 covers the guest-facing scheduling and "
         "submission half; Diagram 2 covers host-side production and administration. Together they carry "
         "all fifteen use cases, four «include» relationships and three «extend» relationships.\n"
         "Arrow direction follows UML: «include» points from the base use case to the included one; "
         "«extend» points from the extending use case to the base it extends. Extension use cases (UC-08, "
         "UC-09, UC-12) are deliberately not given a direct actor association, because they are entered "
         "from an extension point in the base use case rather than initiated independently."))
    add(("image", os.path.join(ROOT, "diagrams", "UseCase_1_Scheduling.png"),
         "Figure 1 - Use-Case Diagram 1: Scheduling & Guest Content Submission"))
    add(("image", os.path.join(ROOT, "diagrams", "UseCase_2_Production.png"),
         "Figure 2 - Use-Case Diagram 2: Episode Production & Administration"))
    add(("p",
         "Use-case inventory: UC-01 Book Interview Slot, UC-02 Submit Talking-Point Outline, UC-03 Publish "
         "Availability, UC-04 Review & Approve Outline, UC-05 Generate Run-of-Show Sheet, UC-06 Validate "
         "Slot Availability, UC-07 Send Booking Notification, UC-08 Request Reschedule, UC-09 Attach "
         "Biography Links, UC-10 Export PDF Production Sheet, UC-11 Recompute Segment Timestamps, UC-12 "
         "Insert Host Segment, UC-13 Notify Approval Decision, UC-14 Manage User Accounts, UC-15 View "
         "Audit Log."))

    # ---------------------------------------------------------------- 8
    add(("h2", "8. Requirements Traceability Matrix (RTM)"))
    add(("p",
         "Status legend: N = Not started, P = Partially implemented, A = Accepted (implemented and test "
         "passed). Every requirement in sections 4, 5 and 5.1.2 appears exactly once. The Comments column "
         "names the Lab 2 Jira story that carries the work, so a grader can walk requirement to backlog "
         "item to test case without leaving the repository."))
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

    for blk in blocks:
        kind = blk[0]
        if kind in ("h1", "h2", "h3"):
            doc.add_paragraph(blk[1], style="Heading " + kind[1])
        elif kind == "meta":
            p = doc.add_paragraph()
            authors = "; ".join(f"{n} ({s})" for n, s, _ in TEAM)
            for i, line in enumerate([
                f"Project: {C.META['project']}",
                f"Problem Statement: {C.META['problem_statement']}",
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
                f"**Problem Statement:** {C.META['problem_statement']}  \n"
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
