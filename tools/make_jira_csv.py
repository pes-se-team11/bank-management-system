# -*- coding: utf-8 -*-
"""Emit docs/jira_import.csv for Jira's CSV importer.

    python tools/make_jira_csv.py

Same content as docs/Jira_Backlog.md - 7 epics, 29 stories - shaped for
"Import work" in Jira Cloud. The importer shows a field-mapping screen, so
column names only have to be close enough to recognise.
"""
import csv
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "docs", "jira_import.csv")

EPICS = [
    ("Epic 1 - Customer & Account Management",
     "Creation and lifecycle of customers and their accounts, from opening through modification to closure."),
    ("Epic 2 - Authentication & Access Control",
     "Establishing who the operator is and what their role permits. A precondition of every other epic."),
    ("Epic 3 - Core Transactions",
     "Deposit and withdrawal - the operations the system exists for."),
    ("Epic 4 - Balance Inquiry & Statements",
     "Read-only views of an account's current position and its transaction history."),
    ("Epic 5 - Funds Transfer",
     "Moving money between two accounts at the bank, atomically."),
    ("Epic 6 - Ledger, Audit & Reporting",
     "The append-only record that makes every balance explicable, and the reports drawn from it."),
    ("Epic 7 - Platform Quality & Security",
     "Cross-cutting properties every feature depends on. Three of these must land in Sprint 1, because "
     "retrofitting a money type or a crash-safe write path means rewriting the features built on them."),
]

# (id, summary, epic index, priority, points, sprint, owner, requirements, role, want, benefit)
STORIES = [
    ("1.1", "Create Customer Record", 0, "High", 3, 1, "P1", "BMS-F-001",
     "Bank Teller", "register a customer with their contact and ID details and get a unique customer ID",
     "accounts can be opened against a person rather than a loose name"),
    ("1.2", "Open and Look Up Accounts", 0, "High", 5, 1, "P1", "BMS-F-002, BMS-F-005",
     "Bank Teller", "open a Savings or Current account for an existing customer and find any account by number or by customer",
     "I can serve someone at the counter without paperwork"),
    ("1.3", "Modify Customer Details", 0, "Medium", 2, 4, "P1", "BMS-F-003",
     "Bank Teller", "update a customer's address and phone number with the previous value retained",
     "records stay current and a mistaken edit can still be traced"),
    ("1.4", "Close a Zero-Balance Account", 0, "High", 3, 4, "P1", "BMS-F-004",
     "Bank Manager", "close an account that has been emptied and have the system refuse every later transaction on it",
     "a closed account cannot quietly come back to life"),
    ("1.5", "Transfer Residual Balance on Closure", 0, "Medium", 5, 4, "P1+P2", "BMS-F-006",
     "Bank Manager", "a closure request on an account still holding money to move that residual to a nominated account first",
     "closing an account can never destroy a balance"),

    ("2.1", "Authenticate Customer and Staff", 1, "High", 5, 2, "P1", "BMS-F-010, BMS-F-014",
     "Customer", "sign in with my account number and PIN without the PIN appearing on screen",
     "nobody standing behind me at the counter can read it"),
    ("2.2", "Lock Account After Failed Attempts", 1, "High", 3, 2, "P1", "BMS-F-011",
     "Bank Manager", "an account locked after three wrong PINs with the event recorded",
     "a stolen passbook cannot be used to guess a PIN"),
    ("2.3", "Enforce Role-Based Authorisation", 1, "High", 5, 2, "P1", "BMS-F-012",
     "Bank Manager", "each role restricted to its own operations",
     "a Teller cannot close accounts or read the audit log"),
    ("2.4", "Manager Unlocks a Locked Account", 1, "Medium", 2, 4, "P1", "BMS-F-013",
     "Bank Manager", "to unlock an account after verifying the holder",
     "a customer who simply forgot their PIN is not permanently locked out"),

    ("3.1", "Deposit into an Account", 2, "High", 5, 2, "P2", "BMS-F-020, BMS-F-021, BMS-F-022",
     "Customer", "my deposit credited to my account and recorded in the ledger",
     "the money is provably mine from the moment it is handed over"),
    ("3.2", "Withdraw Within Limits", 2, "High", 8, 2, "P2", "BMS-F-030, BMS-F-031",
     "Customer", "to withdraw cash only when my balance and daily limit allow it, with the debit and the ledger entry applied together",
     "a failure part-way through cannot leave my balance and the record disagreeing"),
    ("3.3", "Enforce Savings Minimum Balance", 2, "High", 3, 3, "P2", "BMS-F-032",
     "Bank Manager", "Savings withdrawals refused below the minimum balance",
     "the account stays within the product's terms"),
    ("3.4", "Allow Current Account Overdraft", 2, "Medium", 3, 3, "P2", "BMS-F-033",
     "Customer with a Current account", "to draw against my sanctioned overdraft",
     "a short-term shortfall does not stop a legitimate payment"),

    ("4.1", "Check Current Balance", 3, "High", 2, 3, "P3", "BMS-F-040",
     "Customer", "to see my current balance", "I know what I can spend"),
    ("4.2", "Print a Mini-Statement", 3, "Medium", 3, 3, "P3", "BMS-F-041",
     "Customer", "the last ten transactions listed newest first",
     "I can check a recent entry without asking for a full statement"),
    ("4.3", "Statement for a Date Range", 3, "Medium", 3, 3, "P3", "BMS-F-042",
     "Customer", "a statement between two dates with a closing balance",
     "I can reconcile a period against my own records"),

    ("5.1", "Transfer Between Accounts Atomically", 4, "High", 8, 3, "P2", "BMS-F-050, BMS-F-051",
     "Customer", "a transfer to either complete on both accounts or on neither",
     "money can never be destroyed or duplicated by a failure between the two halves"),
    ("5.2", "Enforce Transfer Ceilings", 4, "Medium", 3, 3, "P2", "BMS-F-052",
     "Bank Manager", "per-transaction and daily transfer ceilings enforced",
     "a compromised account cannot be drained in one sitting"),

    ("6.1", "Append-Only Journal and Audit Capture", 5, "High", 5, 2, "P2+P3", "BMS-F-060, BMS-F-063",
     "Bank Manager", "every balance change and every privileged action appended to a record the application cannot rewrite",
     "the history of an account cannot be quietly edited"),
    ("6.2", "Reconcile Balances Against the Journal", 5, "High", 5, 4, "P3", "BMS-F-061",
     "Bank Manager", "a check that every balance equals its opening balance plus its journal entries",
     "tampering with the data files is detected instead of absorbed"),
    ("6.3", "Generate End-of-Day Report", 5, "Medium", 5, 4, "P3", "BMS-F-062",
     "Bank Manager", "the day's deposits, withdrawals, transfers and closing balances totalled",
     "I can close the branch knowing it reconciles"),
    ("6.4", "View the Audit Log", 5, "Low", 2, 4, "P3", "BMS-F-064",
     "Bank Manager", "to read the audit log filtered by operator or date",
     "I can investigate a disputed action"),

    ("7.1", "Integer-Paise Money Type with Overflow Checks", 6, "High", 5, 1, "P2", "BMS-NF-003, BMS-SR-003",
     "developer", "all money handled as 64-bit integer paise with overflow checked before every operation",
     "no rounding error or silent wrap can ever alter a balance"),
    ("7.2", "Crash-Safe Write-Ahead Journal", 6, "High", 8, 1, "P2", "BMS-NF-002, BMS-SR-005",
     "Bank Manager", "the system to journal an intended change before applying it and recover cleanly on restart",
     "a power cut mid-transaction cannot corrupt the ledger"),
    ("7.3", "Salted Credential Hashing and No-Echo Entry", 6, "High", 5, 1, "P1", "BMS-SR-001",
     "Customer", "my PIN stored only as a salted hash and never echoed or logged",
     "someone reading the data files cannot learn it"),
    ("7.4", "Input Validation and Bounds-Checked Buffers", 6, "High", 5, 3, "P1", "BMS-SR-002, BMS-SR-004",
     "developer", "every input validated and every buffer bounds-checked",
     "a malformed entry cannot corrupt memory or drive the program into undefined behaviour"),
    ("7.5", "File Permissions, Instance Lock, Service-Layer Role Check", 6, "High", 5, 3, "P1", "BMS-SR-006, BMS-SR-007",
     "Bank Manager", "data files readable only by their owner, a second instance prevented from starting, and role checks enforced in the service layer",
     "access control cannot be bypassed by reaching an operation another way"),
    ("7.6", "Portable Warning-Free Build and Layered Modules", 6, "Medium", 5, 4, "P3", "BMS-NF-004, BMS-NF-005, BMS-NF-006",
     "developer", "a warning-free build on both toolchains and modules testable without the menu layer",
     "defects surface at compile time and in unit tests rather than in a demo"),
    ("7.7", "Performance and Capacity Benchmark", 6, "Low", 5, 4, "P3", "BMS-NF-001, BMS-NF-007",
     "Bank Manager", "the system measured at 10,000 accounts",
     "its stated performance is a measurement rather than a hope"),
]

HEADER = ["Summary", "Issue Type", "Description", "Priority",
          "Story point estimate", "Epic Link", "Sprint", "Labels"]


def main():
    rows = []
    for name, desc in EPICS:
        rows.append([name, "Epic", desc, "Medium", "", "", "", "bank-management-system"])

    for sid, summary, epic_i, prio, pts, sprint, owner, reqs, role, want, benefit in STORIES:
        desc = (f"As a {role},\n"
                f"I want {want},\n"
                f"So that {benefit}.\n\n"
                f"Delivers: {reqs}\n"
                f"Owner: {owner}\n"
                f"Acceptance criteria: see the acceptance criterion for each requirement above in "
                f"docs/SRS.md section 4/5, and the matching TC- id in the RTM at section 8.")
        labels = " ".join([owner.replace("+", "-"), "sprint" + str(sprint)]
                          + [r.strip() for r in reqs.split(",")])
        rows.append([f"{sid} {summary}", "Story", desc, prio, pts,
                     EPICS[epic_i][0], f"Sprint {sprint}", labels])

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", newline="", encoding="utf-8") as fh:
        w = csv.writer(fh)
        w.writerow(HEADER)
        w.writerows(rows)

    pts = sum(s[4] for s in STORIES)
    print(f"wrote {OUT}")
    print(f"{len(EPICS)} epics, {len(STORIES)} stories, {pts} points")
    for n in (1, 2, 3, 4):
        sp = [s for s in STORIES if s[5] == n]
        print(f"  Sprint {n}: {len(sp)} stories, {sum(s[4] for s in sp)} points")


if __name__ == "__main__":
    main()
