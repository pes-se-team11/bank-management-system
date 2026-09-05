# Jira Backlog — Podcast Guest Scheduling & Outline Builder

**Problem Statement #60 — Media, Events & Community**
**Status:** extended from the Lab 2 backlog · **complete backlog due 15 September 2025**

Jira project: `Podcast Guest Scheduling` — Software Development → **SCRUM** → Company-managed.
Keep this file and the Jira board in agreement; the Jira & Traceability Lead owns both.

> **What changed since Lab 2.** Lab 2 carried 11 stories / 55 points against 7 Lab-1 requirements.
> The mini-project SRS expands that to 36 requirements, so 8 new stories and one new epic were
> added. New items are marked **`NEW`** — nothing from Lab 2 was silently dropped or reworded.

---

## Epic 1 — Scheduling & Availability

Enables a Show Host to publish when they are free to record, and a Podcast Guest to reserve a slot
that is guaranteed conflict-free.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 1.1 Publish Availability Windows | PGS-F-001, PGS-F-002 | High | 5 | 1 |
| 1.2 Book an Interview Slot | PGS-F-003 | High | 8 | 1 |
| 1.3 Validate Slot Availability | PGS-F-004 | High | 3 | 1 |
| 1.4 Request Reschedule | PGS-F-005 | Low | 3 | 3 |
| **`NEW`** 1.5 Timezone-Correct Slot Display | PGS-NF-007 | High | 5 | 3 |

**1.1** — As a Show Host, I want to publish recurring availability windows and mark specific dates
as blocked, so that guests can only book times I am genuinely free to record.

**1.2** — As a Podcast Guest, I want to select an available interview time slot, so that my
recording is confirmed and lands on both our calendars.

**1.3** — As a Podcast Guest, I want the system to re-check my chosen slot at the moment I confirm,
so that two guests can never end up booked into the same recording time.

**1.4** — As a Podcast Guest, I want to release my reserved slot and choose a different one, so that
I can move the interview without emailing the host directly.

**1.5 `NEW`** — As a Podcast Guest in a different timezone from the host, I want every slot shown in
my own local time with the zone named, so that I do not book a recording at the wrong hour or miss
it after a daylight-saving change.

---

## Epic 2 — Guest Content Submission

Lets a Podcast Guest supply the discussion material, so the host can prepare around what the guest
actually wants to cover.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 2.1 Submit Talking-Point Outline | PGS-F-010, PGS-F-012, PGS-F-013 | High | 5 | 1 |
| 2.2 Attach Biography Links | PGS-F-011 | Low | 2 | 3 |

**2.1** — As a Podcast Guest, I want to submit structured bullet-point discussion topics for my
episode and revise them until the host reviews them, so that the host can build the show around the
subjects I want to talk about.

**2.2** — As a Podcast Guest, I want to attach biography and reference links to my submission, so
that the host can introduce me accurately on air.

---

## Epic 3 — Episode Production Output

Turns approved guest material into the timestamped run-of-show production sheet the host records
from.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 3.1 Review and Approve Talking Points | PGS-F-020, PGS-F-021 | Medium | 5 | 2 |
| 3.2 Generate Run-of-Show Sheet | PGS-F-030, PGS-F-031 | High | 8 | 2 |
| 3.3 Export Production Sheet as PDF | PGS-F-033, PGS-NF-001 | High | 5 | 2 |
| **`NEW`** 3.4 Insert Host Segment | PGS-F-022 | Medium | 3 | 2 |
| **`NEW`** 3.5 Edit Segment Duration and Recompute | PGS-F-032 | Medium | 3 | 2 |

**3.1** — As a Show Host, I want to approve, reject or reorder submitted talking points and return a
rejected point with a comment, so that only vetted content reaches the production sheet.

**3.2** — As a Show Host, I want a run-of-show generated from the approved topics with a start
timestamp and duration per segment, so that I can record the episode to a schedule instead of
improvising the order.

**3.3** — As a Show Host, I want the run-of-show exported to a formatted PDF in under one second, so
that I can hand it to my producer minutes before we start recording.

**3.4 `NEW`** — As a Show Host, I want to insert my own segments — intro, sponsor read, outro — at
any point in the running order, so that the sheet reflects the whole episode and not only the guest
conversation.

**3.5 `NEW`** — As a Show Host, I want to change a segment's planned duration and have every later
timestamp update automatically, so that I can rebalance the episode without recomputing times by
hand.

---

## Epic 4 — Notifications & Access Security

Keeps both parties informed of their commitments and keeps unreleased episode material private.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| 4.1 Booking Confirmation and Reminder | PGS-F-040, PGS-F-041 | Medium | 3 | 1 |
| 4.2 Secure Episode Board Access | PGS-F-051, PGS-NF-004, PGS-SR-001, PGS-SR-003 | High | 8 | 2 |
| **`NEW`** 4.3 Notification Retry and Failure Logging | PGS-F-042 | Medium | 3 | 3 |
| **`NEW`** 4.4 Account Sign-Up and Email Verification | PGS-F-050, PGS-SR-002 | High | 5 | 1 |
| **`NEW`** 4.5 Rate-Limit Sign-In and Booking | PGS-SR-004 | Medium | 3 | 3 |
| **`NEW`** 4.6 Escape Guest Content and Restrict URL Schemes | PGS-SR-005 | High | 3 | 3 |

**4.1** — As a Podcast Guest, I want a confirmation when I book and a reminder 24 hours before the
interview, so that I do not miss the recording.

**4.2** — As a Show Host, I want episode boards restricted to me and the invited guest and served
over TLS, so that unreleased episode content and guest contact details stay confidential.

**4.3 `NEW`** — As a Show Host, I want a failed invite or reminder retried and then recorded as a
permanent failure, so that a silent delivery failure never costs me a recording.

**4.4 `NEW`** — As a Show Host, I want to create an account and verify my email before publishing
availability, so that guests are booking against a real, reachable host.

**4.5 `NEW`** — As an Administrator, I want sign-in and booking endpoints rate-limited, so that the
public booking page cannot be brute-forced or flooded with reservations.

**4.6 `NEW`** — As a Show Host, I want guest-submitted text and links rendered safely, so that
opening my own episode board cannot execute someone else's script.

---

## Epic 5 `NEW` — Administration & Audit

Makes approval decisions attributable and gives the operator a way to intervene. Introduced by the
mini-project SRS (security objective SO-2); had no Lab 2 equivalent.

| Story | Requirement(s) | Priority | Points | Sprint |
|---|---|---|---|---|
| **`NEW`** 5.1 Append-Only Audit Log | PGS-F-023, PGS-SR-006 | High | 5 | 3 |
| **`NEW`** 5.2 Administer Accounts and Inspect Audit Log | PGS-F-052 | Low | 5 | 3 |

**5.1 `NEW`** — As a Show Host, I want every approval, rejection and reorder recorded immutably with
who did it and when, so that a dispute about the agreed running order can be settled from evidence.

**5.2 `NEW`** — As an Administrator, I want to suspend an abusive account and read the audit log, so
that I can intervene without touching episode content.

---

## Sprint plan

| | Theme | Stories | Points |
|---|---|---|---|
| **Sprint 1** | Book a guest end to end | 4.4, 1.1, 1.2, 1.3, 2.1, 4.1 | **29** |
| **Sprint 2** | Produce the episode | 3.1, 3.2, 3.3, 3.4, 3.5, 4.2 | **32** |
| **Sprint 3** | Harden and operate | 1.4, 1.5, 2.2, 4.3, 4.5, 4.6, 5.1, 5.2 | **29** |
| | | **19 stories** | **90** |

**Capacity note.** The Lab 2 retrospective put observed velocity at roughly 27 points per sprint,
against simulated rather than real work. All three sprints here sit above that, and Sprint 2 sits
furthest above at 32. Stories **3.4** and **3.5** are the designated spillover: they are Medium
priority, the run-of-show is usable without them, and moving them to Sprint 3 brings Sprint 2 to 26.
This is recorded now rather than discovered at the sprint boundary.

**Sequencing rationale.** Account sign-up (4.4) moves into Sprint 1 because publishing availability
requires a verified host, so 1.1 cannot be demonstrated honestly without it. Secure board access
(4.2) is High priority and was left in Sprint 2 in Lab 2 — the Lab 2 retrospective flagged that as
the one thing to change, and it stays in Sprint 2 here only because Sprint 1 is already at 29 points.

---

## Traceability — Lab 1 → Lab 2 → mini project

| Lab 1 | Lab 2 story | Mini-project requirement(s) |
|---|---|---|
| FR-001 | 1.2, 2.1 | PGS-F-003, PGS-F-010, PGS-F-013 |
| FR-002 | 1.1 | PGS-F-001, PGS-F-002 |
| FR-003 | 3.2 | PGS-F-030, PGS-F-031 |
| FR-004 | 3.1 | PGS-F-020, PGS-F-021 |
| FR-005 | 4.1 | PGS-F-040, PGS-F-041 |
| NFR-001 | 3.3 | PGS-F-033, PGS-NF-001 |
| NFR-002 | 4.2 | PGS-F-051, PGS-NF-004 |
| UC-01 «include» Validate Slot Availability | 1.3 | PGS-F-004 |
| UC-01 «extend» Request Reschedule | 1.4 | PGS-F-005 |
| UC-02 «extend» Attach Biography Links | 2.2 | PGS-F-011 |

Every Lab 1 requirement still has a home, and every new requirement has a story. That is the
property the RTM in SRS section 8 is checked against.
