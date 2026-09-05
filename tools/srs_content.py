# -*- coding: utf-8 -*-
"""Single source of truth for the SRS.

build_srs.py renders this structure to both docs/SRS.md and the
template-styled docs/SRS_Podcast_Guest_Scheduling_v1.0.docx, so the two
can never drift apart.
"""

META = {
    "title": "Software Requirements Specification (SRS)",
    "project": "Podcast Guest Scheduling & Outline Builder",
    "problem_statement": "#60 - Media, Events & Community",
    "version": "1.0",
    "date": "05-09-2025",
    "status": "Draft - for review",
}

# ---------------------------------------------------------------------------
# Requirement IDs use the PGS- prefix (Podcast Guest Scheduling).
#   PGS-F-###   functional
#   PGS-NF-###  non-functional
#   PGS-SR-###  security
# ---------------------------------------------------------------------------

FR_HEADER = ["Req ID", "Requirement (shall...)", "Type", "Priority",
             "Source/Stakeholder", "Acceptance criteria / Test case ref",
             "Comments / Dependencies"]

FR_SCHEDULING = [
    ["PGS-F-001",
     "The system shall allow a Show Host to publish recurring weekly availability windows, each with a start time, end time and IANA timezone.",
     "Functional", "High", "Show Host / FR-002 (Lab 1)",
     "AC-PGS-F-001: A published window appears on the guest booking page in the guest local timezone. Test: TC-SCH-01",
     "Timezone data from IANA tzdb; depends on PGS-NF-007"],
    ["PGS-F-002",
     "The system shall allow a Show Host to mark specific dates or date ranges as blocked, overriding any recurring window that falls inside them.",
     "Functional", "High", "Show Host / FR-002 (Lab 1)",
     "AC-PGS-F-002: No bookable slot is offered on a blocked date; booking on a blocked date is rejected. Test: TC-SCH-02",
     "Realises the Fail condition of the supplied FR-001"],
    ["PGS-F-003",
     "The system shall present a Podcast Guest only those slots that are inside a published window, not blocked, and not already reserved.",
     "Functional", "High", "Podcast Guest / FR-001 (Lab 1)",
     "AC-PGS-F-003: The slot list excludes blocked and reserved slots for every query. Test: TC-SCH-03",
     "Read path over PGS-F-001, PGS-F-002, PGS-F-004"],
    ["PGS-F-004",
     "The system shall re-validate slot availability inside a single atomic transaction at the moment the Guest confirms, and reject the booking if the slot was taken in the interim.",
     "Functional", "High", "Podcast Guest / UC-01 include",
     "AC-PGS-F-004: Two concurrent confirmations for one slot yield exactly one booking and one rejection with a retry prompt. Test: TC-SCH-04",
     "Requires a DB unique constraint on (host_id, slot_start)"],
    ["PGS-F-005",
     "The system shall allow a Podcast Guest to release a confirmed slot and select a different one up to 24 hours before the scheduled start.",
     "Functional", "Low", "Podcast Guest / UC-01 extend",
     "AC-PGS-F-005: Reschedule succeeds at more than 24h and is refused with an explanatory message at 24h or less. Test: TC-SCH-05",
     "Triggers re-issue of the calendar invite (PGS-F-040)"],
]

FR_CONTENT = [
    ["PGS-F-010",
     "The system shall allow a Podcast Guest to submit an ordered list of structured bullet-point talking points, each up to 280 characters.",
     "Functional", "High", "Podcast Guest / FR-001 (Lab 1)",
     "AC-PGS-F-010: Submitted points persist in the order given; a point over 280 characters is rejected at input. Test: TC-OUT-01",
     "Ordering is user-controlled, not alphabetical"],
    ["PGS-F-011",
     "The system shall allow a Podcast Guest to attach biography and reference links, accepting only well-formed absolute http or https URLs.",
     "Functional", "Low", "Podcast Guest / UC-02 extend",
     "AC-PGS-F-011: A valid https URL is stored; javascript:, file: and relative URLs are rejected. Test: TC-OUT-02",
     "Optional per the extend relationship; see PGS-SR-005"],
    ["PGS-F-012",
     "The system shall persist an outline as an editable draft and allow the Guest to revise it until the Host records an approval decision.",
     "Functional", "Medium", "Podcast Guest",
     "AC-PGS-F-012: A draft survives sign-out and sign-in; edits are refused once the outline is approved. Test: TC-OUT-03",
     "State machine: DRAFT to SUBMITTED to APPROVED or REJECTED"],
    ["PGS-F-013",
     "The system shall attach the submitted outline to the episode board of the booking it belongs to.",
     "Functional", "High", "Show Host / FR-001 (Lab 1)",
     "AC-PGS-F-013: Opening the episode board shows the booking and its outline together. Test: TC-OUT-04",
     "Realises the Pass condition of the supplied FR-001"],
]

FR_REVIEW = [
    ["PGS-F-020",
     "The system shall allow a Show Host to approve, reject or reorder each submitted talking point individually.",
     "Functional", "Medium", "Show Host / FR-004 (Lab 1)",
     "AC-PGS-F-020: Each point carries an independent decision; reordering persists across sessions. Test: TC-REV-01",
     "Only approved points reach PGS-F-030"],
    ["PGS-F-021",
     "The system shall require a non-empty comment when a Show Host rejects a talking point, and shall return that comment to the Guest.",
     "Functional", "Medium", "Show Host / FR-004 (Lab 1)",
     "AC-PGS-F-021: Rejection without a comment is refused; the Guest sees the comment on the rejected point. Test: TC-REV-02",
     "Notification delivered by PGS-F-042"],
    ["PGS-F-022",
     "The system shall allow a Show Host to insert host-authored segments (intro, sponsor read, outro) at any position in the running order.",
     "Functional", "Medium", "Show Host",
     "AC-PGS-F-022: A host segment appears at the chosen position and is timed like any other segment. Test: TC-REV-03",
     "Host segments bypass the approval state machine"],
    ["PGS-F-023",
     "The system shall record every approval, rejection and reorder action in an append-only audit trail with actor, timestamp and previous value.",
     "Functional", "High", "Assessment / Security",
     "AC-PGS-F-023: Each decision produces exactly one immutable audit record. Test: TC-REV-04",
     "Supports SO-2; storage governed by PGS-SR-006"],
]

FR_RUNOFSHOW = [
    ["PGS-F-030",
     "The system shall generate a run-of-show sheet containing every approved talking point and host segment in running order, each with a planned duration.",
     "Functional", "High", "Show Host / FR-003 (Lab 1)",
     "AC-PGS-F-030: The generated sheet contains all approved segments and no rejected ones. Test: TC-ROS-01",
     "Default duration 5 minutes per segment"],
    ["PGS-F-031",
     "The system shall compute a cumulative start timestamp for each segment from the episode start time and the sum of the preceding durations.",
     "Functional", "High", "Show Host / FR-003 (Lab 1)",
     "AC-PGS-F-031: Segment n starts at episode_start plus the sum of durations 1..n-1, accurate to the second. Test: TC-ROS-02",
     "Pure function; unit-testable in isolation"],
    ["PGS-F-032",
     "The system shall allow a Show Host to edit any segment duration and shall recompute all downstream timestamps on save.",
     "Functional", "Medium", "Show Host",
     "AC-PGS-F-032: Changing segment 2 shifts segments 3..n by the delta and leaves segment 1 unchanged. Test: TC-ROS-03",
     "Recomputation reuses PGS-F-031"],
    ["PGS-F-033",
     "The system shall export the run-of-show as a formatted PDF production sheet carrying the episode title, date, guest name and timed segment table.",
     "Functional", "High", "Show Host / NFR-001 (Lab 1)",
     "AC-PGS-F-033: The PDF opens in a standard reader and contains every field listed. Test: TC-ROS-04",
     "Latency budget defined in PGS-NF-001"],
]

FR_NOTIFY = [
    ["PGS-F-040",
     "The system shall dispatch an iCalendar (RFC 5545) invite to the Guest and the Host within 60 seconds of a confirmed booking.",
     "Functional", "High", "Calendar & Notification Service / FR-005 (Lab 1)",
     "AC-PGS-F-040: Both parties receive an invite whose start time matches the booked slot. Test: TC-NOT-01",
     "Realises the Pass condition of the supplied FR-001"],
    ["PGS-F-041",
     "The system shall send a reminder to both parties 24 hours before the scheduled recording start.",
     "Functional", "Medium", "Calendar & Notification Service / FR-005 (Lab 1)",
     "AC-PGS-F-041: A reminder is delivered in the window T-24h plus or minus 5 minutes. Test: TC-NOT-02",
     "Suppressed if the booking was cancelled"],
    ["PGS-F-042",
     "The system shall notify the Host on outline submission and the Guest on each approval decision, retrying failed deliveries with exponential backoff up to three attempts.",
     "Functional", "Medium", "Calendar & Notification Service",
     "AC-PGS-F-042: A simulated provider failure produces three retries and one logged permanent failure. Test: TC-NOT-03",
     "Delivery outcome written to the audit log"],
]

FR_ACCESS = [
    ["PGS-F-050",
     "The system shall authenticate users with an email address and password, and shall require email verification before a Host may publish availability.",
     "Functional", "High", "Security / Show Host",
     "AC-PGS-F-050: An unverified Host cannot publish availability; a verified Host can. Test: TC-AUT-01",
     "Password storage governed by PGS-SR-002"],
    ["PGS-F-051",
     "The system shall restrict every episode board to its owning Host and the invited Guest, denying access to all other authenticated users.",
     "Functional", "High", "Security / NFR-002 (Lab 1)",
     "AC-PGS-F-051: A third authenticated user requesting the board receives HTTP 403. Test: TC-AUT-02",
     "Enforced server-side per PGS-SR-003"],
    ["PGS-F-052",
     "The system shall provide an Administrator function to suspend an account and to view booking, delivery and approval audit logs.",
     "Functional", "Low", "Administrator",
     "AC-PGS-F-052: A suspended account cannot sign in; audit entries are listed read-only. Test: TC-ADM-01",
     "Administrator role is distinct from the Host role"],
]

FR_SECTIONS = [
    ("4.1 Availability & Scheduling",
     "Description: A Show Host declares when they are free to record; a Podcast Guest reserves one of those "
     "times and is guaranteed it is conflict-free. This section carries the scheduling half of the supplied "
     "FR-001 and all of FR-002.",
     FR_SCHEDULING),
    ("4.2 Guest Content Submission",
     "Description: A Podcast Guest supplies the discussion material for the episode, so the Host can prepare "
     "around what the guest actually wants to cover. This section carries the outline half of the supplied FR-001.",
     FR_CONTENT),
    ("4.3 Host Review & Approval",
     "Description: The Show Host vets submitted material before it reaches the production sheet, and every "
     "decision is attributable after the fact.",
     FR_REVIEW),
    ("4.4 Run-of-Show Generation & Export",
     "Description: Approved material becomes the timestamped running order the episode is recorded from, and "
     "the PDF production sheet handed to the producer.",
     FR_RUNOFSHOW),
    ("4.5 Notifications & Calendar Integration",
     "Description: Both parties are told what they have committed to and are reminded before it happens. All "
     "delivery is via the external Calendar & Notification Service.",
     FR_NOTIFY),
    ("4.6 Accounts, Access & Administration",
     "Description: Identity, authorisation and operational oversight. This section is where NFR-002 from Lab 1 "
     "becomes enforceable behaviour.",
     FR_ACCESS),
]

NFR_HEADER = ["Req ID", "Requirement", "Category", "Priority",
              "Acceptance criteria / Measurement"]

NFRS = [
    ["PGS-NF-001",
     "The run-of-show PDF export shall complete in under 1 second for outlines of up to 30 segments, at the 95th percentile.",
     "Performance", "High",
     "95th percentile at or below 1.000 s over 500 exports at 50 concurrent users. Test: TC-PERF-01"],
    ["PGS-NF-002",
     "The slot-availability page shall render within 2 seconds at the 90th percentile under a load of 100 concurrent guests.",
     "Performance", "Medium",
     "90th percentile at or below 2.0 s in a JMeter run at 100 concurrent users. Test: TC-PERF-02"],
    ["PGS-NF-003",
     "The system shall provide 99.5% monthly availability, excluding announced maintenance windows.",
     "Reliability / Availability", "High",
     "Uptime report shows 99.5% or better per calendar month. Test: TC-OPS-01"],
    ["PGS-NF-004",
     "All episode-board content and guest contact details shall be transmitted over TLS 1.2 or higher and shall not be readable by unauthenticated users.",
     "Security / Confidentiality", "High",
     "TLS scan reports no protocol below 1.2; anonymous board request returns 401. Test: TC-SEC-01"],
    ["PGS-NF-005",
     "The guest booking and outline submission flows shall conform to WCAG 2.1 Level AA.",
     "Usability / Accessibility", "Medium",
     "Automated axe-core scan reports zero Level AA violations; keyboard-only walkthrough passes. Test: TC-UX-01"],
    ["PGS-NF-006",
     "The system shall support 500 active Hosts and 5,000 bookings per month with no change to the deployed architecture.",
     "Scalability", "Low",
     "Soak test at the stated volume holds the PGS-NF-002 latency target. Test: TC-PERF-03"],
    ["PGS-NF-007",
     "The system shall store all instants in UTC and render them in each user's IANA timezone, remaining correct across daylight-saving transitions.",
     "Correctness / Portability", "High",
     "A booking made across a DST boundary displays the same wall-clock time to both parties. Test: TC-SCH-06"],
]

SECURITY_OBJECTIVES = [
    ("SO-1 - Confidentiality of unreleased episode material",
     "Episode outlines, guest contact details and run-of-show sheets are commercially sensitive before broadcast. "
     "Only the owning Host and the invited Guest may read an episode board, in transit and at rest. This objective "
     "is the security reading of NFR-002 from Lab 1."),
    ("SO-2 - Integrity and non-repudiation of the approved running order",
     "The run-of-show is the artefact the episode is recorded from. Every approval, rejection and reorder must be "
     "attributable to an actor and a time, and must not be silently alterable after the fact."),
    ("SO-3 - Availability and abuse resistance of the public booking endpoint",
     "The booking page is reachable without authentication by design. It must resist automated slot-hoarding and "
     "credential brute-forcing without degrading service for legitimate guests."),
]

SR_HEADER = ["Req ID", "Requirement (shall...)", "Type", "Priority",
             "Acceptance criteria / Test case ref"]

SECURITY_REQS = [
    ["PGS-SR-001",
     "The system shall enforce TLS 1.2 or higher on all network connections and shall send an HTTP Strict-Transport-Security header on every response.",
     "Security", "High",
     "AC: TLS scan shows no protocol below 1.2 and an HSTS header with max-age of at least 31536000. Test: TC-SEC-01"],
    ["PGS-SR-002",
     "The system shall store passwords only as Argon2id hashes with a per-user salt, and shall never write a password or session token to any log.",
     "Security", "High",
     "AC: Database inspection shows no plaintext password; a log search for credential patterns returns nothing. Test: TC-SEC-02"],
    ["PGS-SR-003",
     "The system shall perform a server-side authorisation check on every episode-board resource, keyed to the authenticated principal rather than to a client-supplied identifier.",
     "Security", "High",
     "AC: Substituting another user's board id returns HTTP 403, not 200. Test: TC-SEC-03"],
    ["PGS-SR-004",
     "The system shall rate-limit the sign-in endpoint to 5 attempts per account per 15 minutes and the booking endpoint to 10 requests per IP address per minute.",
     "Security", "Medium",
     "AC: The 6th sign-in attempt and the 11th booking request return HTTP 429. Test: TC-SEC-04"],
    ["PGS-SR-005",
     "The system shall HTML-escape all guest-supplied talking points and link text on output, and shall reject any URL scheme other than http or https.",
     "Security", "High",
     "AC: A stored script payload renders as inert text; a javascript: URL is refused at input. Test: TC-SEC-05"],
    ["PGS-SR-006",
     "The system shall write booking, approval and notification-delivery events to an append-only audit log retained for 12 months, with no delete or update path exposed to any application role.",
     "Security / Audit", "Medium",
     "AC: An attempted UPDATE on the audit table fails on privileges; a 12-month-old entry is still retrievable. Test: TC-SEC-06"],
]

RTM_HEADER = ["Req ID", "Requirement short", "Section ref / Design Spec",
              "Module", "Test case(s)", "Status (N/P/A)", "Comments"]

RTM = [
    ["PGS-F-001", "Publish availability windows", "4.1 / DS-SCH-01", "SchedulingService", "TC-SCH-01", "N", "Jira story 1.1"],
    ["PGS-F-002", "Block dates", "4.1 / DS-SCH-02", "SchedulingService", "TC-SCH-02", "N", "Jira story 1.1"],
    ["PGS-F-003", "Show bookable slots only", "4.1 / DS-SCH-03", "SchedulingService, WebUI", "TC-SCH-03", "N", "Jira story 1.2"],
    ["PGS-F-004", "Atomic slot re-validation", "4.1 / DS-SCH-04", "SchedulingService", "TC-SCH-04", "N", "Jira story 1.3"],
    ["PGS-F-005", "Reschedule booking", "4.1 / DS-SCH-05", "SchedulingService", "TC-SCH-05", "N", "Jira story 1.4"],
    ["PGS-F-010", "Submit talking points", "4.2 / DS-OUT-01", "OutlineService", "TC-OUT-01", "N", "Jira story 2.1"],
    ["PGS-F-011", "Attach biography links", "4.2 / DS-OUT-02", "OutlineService", "TC-OUT-02", "N", "Jira story 2.2"],
    ["PGS-F-012", "Editable outline drafts", "4.2 / DS-OUT-03", "OutlineService", "TC-OUT-03", "N", "Jira story 2.1"],
    ["PGS-F-013", "Attach outline to episode board", "4.2 / DS-OUT-04", "OutlineService", "TC-OUT-04", "N", "Jira story 2.1"],
    ["PGS-F-020", "Approve, reject or reorder points", "4.3 / DS-REV-01", "ReviewService", "TC-REV-01", "N", "Jira story 3.1"],
    ["PGS-F-021", "Mandatory rejection comment", "4.3 / DS-REV-02", "ReviewService", "TC-REV-02", "N", "Jira story 3.1"],
    ["PGS-F-022", "Insert host segments", "4.3 / DS-REV-03", "ReviewService", "TC-REV-03", "N", "New in mini project"],
    ["PGS-F-023", "Audit approval decisions", "4.3 / DS-REV-04", "AuditService", "TC-REV-04", "N", "Supports SO-2"],
    ["PGS-F-030", "Generate run-of-show", "4.4 / DS-ROS-01", "RunOfShowService", "TC-ROS-01", "N", "Jira story 3.2"],
    ["PGS-F-031", "Cumulative timestamps", "4.4 / DS-ROS-02", "RunOfShowService", "TC-ROS-02", "N", "Jira story 3.2"],
    ["PGS-F-032", "Edit durations and recompute", "4.4 / DS-ROS-03", "RunOfShowService", "TC-ROS-03", "N", "New in mini project"],
    ["PGS-F-033", "Export PDF production sheet", "4.4 / DS-EXP-01", "ExportService", "TC-ROS-04", "N", "Jira story 3.3"],
    ["PGS-F-040", "Send calendar invite", "4.5 / DS-NOT-01", "NotificationService", "TC-NOT-01", "N", "Jira story 4.1"],
    ["PGS-F-041", "24-hour reminder", "4.5 / DS-NOT-02", "NotificationService", "TC-NOT-02", "N", "Jira story 4.1"],
    ["PGS-F-042", "Submission and decision notices with retry", "4.5 / DS-NOT-03", "NotificationService", "TC-NOT-03", "N", "New in mini project"],
    ["PGS-F-050", "Authenticate users", "4.6 / DS-AUT-01", "AuthService", "TC-AUT-01", "N", "Jira story 4.2"],
    ["PGS-F-051", "Restrict episode board", "4.6 / DS-AUT-02", "AuthService", "TC-AUT-02", "N", "Jira story 4.2"],
    ["PGS-F-052", "Admin suspend and audit view", "4.6 / DS-ADM-01", "AdminConsole", "TC-ADM-01", "N", "New in mini project"],
    ["PGS-NF-001", "PDF export under 1 s (p95)", "5 / DS-EXP-01", "ExportService", "TC-PERF-01", "N", "Given NFR-001"],
    ["PGS-NF-002", "Slot page within 2 s (p90)", "5 / DS-SCH-03", "WebUI, SchedulingService", "TC-PERF-02", "N", ""],
    ["PGS-NF-003", "99.5% availability", "5 / DS-OPS-01", "Platform", "TC-OPS-01", "N", ""],
    ["PGS-NF-004", "TLS and private boards", "5 / DS-AUT-02", "Platform, AuthService", "TC-SEC-01", "N", "Given NFR-002"],
    ["PGS-NF-005", "WCAG 2.1 AA", "5 / DS-UX-01", "WebUI", "TC-UX-01", "N", ""],
    ["PGS-NF-006", "500 hosts, 5,000 bookings per month", "5 / DS-OPS-02", "Platform", "TC-PERF-03", "N", ""],
    ["PGS-NF-007", "UTC storage, timezone-correct display", "5 / DS-SCH-06", "SchedulingService, WebUI", "TC-SCH-06", "N", ""],
    ["PGS-SR-001", "TLS 1.2+ and HSTS", "5.1.2 / DS-SEC-01", "Platform", "TC-SEC-01", "N", "SO-1"],
    ["PGS-SR-002", "Argon2id password hashing", "5.1.2 / DS-SEC-02", "AuthService", "TC-SEC-02", "N", "SO-1"],
    ["PGS-SR-003", "Server-side authorisation", "5.1.2 / DS-SEC-03", "AuthService", "TC-SEC-03", "N", "SO-1"],
    ["PGS-SR-004", "Rate limiting", "5.1.2 / DS-SEC-04", "ApiGateway", "TC-SEC-04", "N", "SO-3"],
    ["PGS-SR-005", "Output escaping and URL allowlist", "5.1.2 / DS-SEC-05", "WebUI, OutlineService", "TC-SEC-05", "N", "SO-1"],
    ["PGS-SR-006", "Append-only audit log", "5.1.2 / DS-SEC-06", "AuditService", "TC-SEC-06", "N", "SO-2"],
]


def all_frs():
    out = []
    for _, _, rows in FR_SECTIONS:
        out.extend(rows)
    return out
