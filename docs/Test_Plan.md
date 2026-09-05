# Software Test Plan (STP) — Podcast Guest Scheduling & Outline Builder

**Project:** Podcast Guest Scheduling & Outline Builder
**Problem Statement:** #60 — Media, Events & Community
**Version:** 0.9 (draft)
**Authors:** QA / Test Lead, with the mini-project team
**Date:** 05-09-2025
**Status:** Draft — **due 15 September 2025**

> Built against `templates/Test_Plan_Template for SE.docx`, section for section. Every `TC-` id
> below is already referenced by an acceptance criterion in [`SRS.md`](SRS.md), so the two documents
> are consistent by construction. Items still needing team input are marked **`<TBC>`**.

---

## 1. Introduction

**Purpose.** This document defines the test plan for the Podcast Guest Scheduling & Outline Builder
v1.0. It states the objectives, scope, strategy, resources, schedule and responsibilities for
verifying that the system meets the requirements in the SRS.

**Scope.** Testing covers availability publication and blocking, slot presentation and booking,
booking concurrency, rescheduling, outline submission and revision, host review and approval,
run-of-show generation and PDF export, notification dispatch and retry, authentication and episode
board isolation, and administration. Excluded items are listed in section 4.

**References.** SRS v1.0 (`docs/SRS.md`), Jira backlog (`docs/Jira_Backlog.md`), RTM (SRS section 8),
Problem Statement #60, WCAG 2.1, RFC 5545 (iCalendar).

**Definitions.** STP (Software Test Plan), SRS (Software Requirements Specification), RTM
(Requirements Traceability Matrix), UAT (User Acceptance Testing), p90/p95 (90th/95th percentile),
DST (Daylight Saving Time), TLS (Transport Layer Security).

---

## 2. Test items

- SchedulingService — availability, blocking, slot query, booking, reschedule
- OutlineService — talking points, links, draft lifecycle, episode-board attachment
- ReviewService — approve, reject, reorder, host segments
- RunOfShowService — running order, timestamp computation, recompute on edit
- ExportService — PDF production sheet
- NotificationService — invites, reminders, decision notices, retry
- AuthService — sign-in, email verification, episode-board authorisation
- AuditService — append-only event log
- AdminConsole — account suspension, audit inspection
- WebUI — booking page, episode board, host console

---

## 3. Features to be tested

Mapped to SRS requirement ids.

| Requirement | Feature | Test case(s) |
|---|---|---|
| PGS-F-001, PGS-F-002 | Publish availability, block dates | TC-SCH-01, TC-SCH-02 |
| PGS-F-003 | Present only bookable slots | TC-SCH-03 |
| PGS-F-004 | Atomic slot re-validation on confirm | TC-SCH-04 |
| PGS-F-005 | Reschedule inside the 24-hour window | TC-SCH-05 |
| PGS-NF-007 | UTC storage, timezone-correct display across DST | TC-SCH-06 |
| PGS-F-010 – PGS-F-013 | Outline submission, links, drafts, attachment | TC-OUT-01 … TC-OUT-04 |
| PGS-F-020 – PGS-F-023 | Review, mandatory rejection comment, host segments, audit | TC-REV-01 … TC-REV-04 |
| PGS-F-030 – PGS-F-033 | Run-of-show, timestamps, recompute, PDF export | TC-ROS-01 … TC-ROS-04 |
| PGS-F-040 – PGS-F-042 | Invite, reminder, retry and permanent-failure logging | TC-NOT-01 … TC-NOT-03 |
| PGS-F-050, PGS-F-051 | Verification gate, episode-board isolation | TC-AUT-01, TC-AUT-02 |
| PGS-F-052 | Account suspension and audit view | TC-ADM-01 |
| PGS-NF-001, PGS-NF-002, PGS-NF-006 | Export latency, page latency, soak | TC-PERF-01 … TC-PERF-03 |
| PGS-NF-003 | Monthly availability | TC-OPS-01 |
| PGS-NF-004, PGS-SR-001 – PGS-SR-006 | TLS, credentials, authorisation, rate limits, escaping, audit immutability | TC-SEC-01 … TC-SEC-06 |
| PGS-NF-005 | WCAG 2.1 AA conformance | TC-UX-01 |

---

## 4. Features not to be tested

- Internal behaviour of the external Calendar & Notification Service — verified only at our
  boundary via a stub; delivery to the recipient's inbox is the provider's responsibility
- Third-party PDF rendering library internals — we test our output, not their engine
- Browser rendering engines themselves
- Audio/video recording, editing and podcast hosting — out of product scope per SRS §1.2
- Load beyond the PGS-NF-006 ceiling (500 hosts / 5,000 bookings per month)

---

## 5. Test approach / strategy

**Levels**

- Unit — pure logic in isolation. The timestamp computation behind PGS-F-031 is the highest-value
  unit target: it is a pure function and every run-of-show defect ultimately shows up there.
- Integration — SchedulingService against the database (the unique constraint behind PGS-F-004),
  NotificationService against a stubbed provider.
- System — end-to-end flows through the WebUI.
- Acceptance (UAT) — the two headline journeys: *guest books and submits*, and *host approves and
  exports*.

**Types**

- Functional, against the acceptance criterion quoted in the SRS
- Regression, re-run on every PR that touches a tested module
- Performance, for PGS-NF-001, PGS-NF-002, PGS-NF-006
- Security, per section 5.1
- Usability and accessibility, for PGS-NF-005
- Concurrency, specifically for PGS-F-004 — two simultaneous confirmations of one slot

**Entry criteria.** Build deploys to the test environment; seed data loaded; notification stub
reachable; smoke suite green.

**Exit criteria.** 100% of planned test cases executed; zero open critical defects; no open major
defect against a High-priority requirement; every RTM row at status `A`; performance targets
PGS-NF-001 and PGS-NF-002 met on the production-like configuration.

### 5.1 Security validation

Each item traces to a security objective from SRS §5.1.1.

| Check | Requirement | Objective |
|---|---|---|
| TLS protocol scan and HSTS header inspection | PGS-SR-001 | SO-1 |
| Database and log inspection for plaintext credentials or tokens | PGS-SR-002 | SO-1 |
| Horizontal privilege escalation — substitute another user's board id, expect 403 | PGS-SR-003 | SO-1 |
| Brute-force sign-in and booking flood, expect HTTP 429 | PGS-SR-004 | SO-3 |
| Stored-XSS payload in a talking point; `javascript:` and `file:` URLs in a biography link | PGS-SR-005 | SO-1 |
| Attempt UPDATE/DELETE on the audit table as the application role, expect a privilege error | PGS-SR-006 | SO-2 |
| Fuzzing of outline text, link and duration fields | PGS-SR-005 | SO-1 |

---

## 6. Test environment

**Software.** Application v1.0 on a Linux host; relational database with the production schema;
Calendar & Notification Service **stub** exposing the same contract, with an injectable failure
mode to exercise the PGS-F-042 retry path.

**Clients.** Current Chrome, Edge, Firefox and Safari; one mobile viewport.

**Tools.** `<TBC — align with the stack the Design Lead settles in the SAD>`. Working assumption:
Postman for API, JMeter for load, axe-core for accessibility, Jira for defects.

**Test data.** Two host accounts (one verified, one unverified), four guest accounts, one suspended
account, published availability spanning a DST boundary, blocked date ranges, and outlines at 1, 30
and 31 segments to sit either side of the PGS-NF-001 boundary.

---

## 7. Test schedule

| Milestone | Date |
|---|---|
| Test plan approved | 15-Sep-2025 |
| Test case design complete | `<TBC>` |
| Environment ready | `<TBC>` |
| Test execution start | `<TBC>` |
| Test execution end | `<TBC>` |
| UAT | `<TBC>` |

Dates depend on the development schedule the team agrees after the 15 September class.

---

## 8. Test deliverables

Test plan (this document) · test cases · test data set · execution logs · defect reports ·
requirement coverage report from the RTM · test summary report.

---

## 9. Roles and responsibilities

| Role | Name | Responsibility |
|---|---|---|
| QA / Test Lead | `<member 3>` | Owns this plan, coordinates execution, signs off exit criteria |
| Test Engineer | `<member 4>` | Designs and executes test cases, logs defects |
| Developer | `<member 2>` | Fixes and triages defects, supports environment issues |
| Requirements Lead | Dhanush S | Arbitrates disputes over what an acceptance criterion means |

---

## 10. Risks and mitigation

| Risk | Mitigation |
|---|---|
| Concurrency defect in PGS-F-004 is hard to reproduce by hand | Automate the two-client race in the integration suite; do not rely on manual timing |
| DST boundary bugs surface only twice a year | Freeze system time in tests rather than waiting for a real transition |
| Notification provider unavailable or rate-limited during testing | Test against the stub by default; treat live-provider runs as a separate, scheduled smoke test |
| PGS-NF-001 measured on a developer laptop, not production-like hardware | Fix the performance configuration before the first measurement and record it with every result |
| Test data with real personal details | Use synthetic guests only; no real names or email addresses |
| Team member unavailable near submission | Every role has a named second reviewer in `ROLES.md` |

---

## 11. Assumptions and dependencies

- The notification stub is available before execution starts and matches the real provider contract
- Seed data is loaded and reset between runs
- The SAD has settled the technology stack before test case design begins
- Requirement acceptance criteria are frozen once execution starts; a change means a new SRS
  revision row and a re-run of the affected cases

---

## 12. Suspension and resumption criteria

**Suspend** when the environment is unavailable for more than 4 hours, when a build blocks more than
30% of planned cases, or when a critical security defect (SO-1 or SO-2) is open.

**Resume** when the blocking defect is fixed and verified, the environment is stable, and the smoke
suite passes.

---

## 13. Test case management and traceability

The RTM in SRS section 8 is the single coverage record — it is not duplicated here. A requirement is
covered only when its row names at least one `TC-` id and that case has been executed.

Examples:

- `PGS-F-004` (atomic slot re-validation) → `TC-SCH-04`
- `PGS-F-033` (PDF export) → `TC-ROS-04`, and `PGS-NF-001` (latency) → `TC-PERF-01`
- `PGS-SR-003` (server-side authorisation) → `TC-SEC-03`

---

## 14. Test metrics and reporting

**Metrics.** Test cases executed (%) · passed/failed (%) · requirement coverage from the RTM ·
defect density by module · defect aging · defects reopened.

**Reports.** Execution status at each stand-up; a coverage report when execution ends; a final test
summary report submitted with the project.

---

## 15. Approvals

| Role | Name | Signature / Date |
|---|---|---|
| QA / Test Lead | | |
| Design Lead | | |
| Requirements Lead | | |
| Course Coordinator | | |
