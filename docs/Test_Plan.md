# Software Test Plan (STP) - Bank Management System

Project: Bank Management System
Version: 1.0
Authors: Adarsha E (PES1UG24AM334)
Date: 01-10-2026
Status: Draft

## 1. Introduction

Purpose: This document defines the test plan for Bank Management System v1.0. It outlines objectives, scope, strategy, resources, schedule, and responsibilities for testing.

Scope: Testing covers customer and account management, authentication, deposits, withdrawals, balance and statements, transfers, ledger, audit, and reporting. The exclusions are listed in section 4.

References: Bank Management System SRS v1.0, Jira Backlog, Software Architecture and Design traceability.

Definitions: SRS (Software Requirements Specification), RTM (Requirements Traceability Matrix), UAT (User Acceptance Testing), PIN (Personal Identification Number), CLI (Command Line Interface), paise (integer unit of currency).

## 2. Test Items

- Account and customer module
- Authentication and authorisation module
- Deposit, withdrawal, and transfer module
- Balance, statement, ledger, and reporting module
- Validation and file persistence module
- CLI and C++17 build



## 3. Features to be Tested

Features mapped to SRS requirement IDs:

- BMS-F-001 to BMS-F-006: Customer records and account lifecycle
- BMS-F-010 to BMS-F-014: Authentication, lockout, and role control
- BMS-F-020 to BMS-F-022: Deposit and journal entry
- BMS-F-030 to BMS-F-033: Withdrawal and account limits
- BMS-F-040 to BMS-F-042: Balance and statements
- BMS-F-050 to BMS-F-052: Atomic fund transfer
- BMS-F-060 to BMS-F-064: Ledger, reconciliation, audit, and reporting
- BMS-NF-001 to BMS-NF-007: Performance, reliability, and build quality
- BMS-SR-001 to BMS-SR-007: Credential, data, input, and access security



## 4. Features Not to be Tested

- ATM hardware, card readers, cash dispensers, and kiosk interfaces
- Inter-bank settlement, cheque clearing, loans, and card issuance
- Internet and mobile banking interfaces and statutory reporting



## 5. Test Approach / Strategy

Levels:

- Unit tests (validation, money arithmetic, and service methods)
- Integration tests (transactions with persistence and ledger)
- System tests (end-to-end CLI workflows)
- Acceptance tests (Customer, Teller, and Manager use cases)

Types:

- Functional and boundary-value testing
- Regression and recovery testing before and after COMMIT and between transfer legs
- Performance testing (latency and 10,000-account startup)
- Usability and security testing

Entry Criteria: Relevant module built, test data and environment ready, expected results defined; journal format and failure hooks agreed.
Exit Criteria: All planned cases executed, high-priority requirements pass, no critical or major defect open, and reconciliation passes.

## 5.1 Security Validation

- Verify salted credential hashes and no plaintext or terminal echo
- Check role restrictions in the service layer
- Test invalid input, long strings, and monetary overflow
- Verify owner-only files, append-only logs, and rejection of a second instance
- Inject write and disk-full failures; check COMMIT replay, account CRC recovery, and reconciliation



## 6. Test Environment

Hardware: Standard desktop or laptop; record specifications for performance tests.
Software: Bank Management System C++17 console application on Linux and Windows.
Tools: g++, MinGW, unit-test harness, scripted CLI tests, Jira for defects.
Test Data: Synthetic Savings and Current accounts, ACTIVE/LOCKED/CLOSED states, transaction histories, and boundary amounts.

## 7. Test Schedule

Milestones:

- Test plan and case design: 02-Oct-2026
- Environment setup: Sprint 1, before execution
- Unit and integration execution: Sprints 1 to 3 as modules merge
- System and regression execution: Sprint 4
- UAT and summary report: After full regression in Sprint 4



## 8. Test Deliverables

- Test Plan (this document)
- Test Cases (manual and automated)
- Test Scripts
- Synthetic Test Data
- Test Execution Logs
- Defect Reports
- Test Summary Report



## 9. Roles and Responsibilities


| Role                  | Name       | Responsibility                                        |
| --------------------- | ---------- | ----------------------------------------------------- |
| QA Lead               | Adarsha E  | Prepare plan, coordinate and record testing           |
| Account and Auth Lead | Dhanush S  | Support account, authentication, and validation tests |
| Transaction Lead      | Vidit Soni | Support transaction and persistence tests and fixes   |
| Review Team           | Team 11    | Review results and release readiness                  |




## 10. Risks and Mitigation


| Risk                            | Mitigation                                                                        |
| ------------------------------- | --------------------------------------------------------------------------------- |
| Delay in stable build delivery  | Request early builds and run module tests as code merges                          |
| Incomplete or repeated recovery | Fail before/after COMMIT and between transfer legs; verify once-only replay       |
| Corrupt record or full disk     | Corrupt an account CRC and simulate disk full; verify recovery or no state change |




## 11. Assumptions & Dependencies

- The SRS v1.0 and its requirement IDs remain the test baseline
- Journal format, COMMIT rules, and test hooks are approved before integration
- Synthetic accounts and a stable build are available before execution



## 12. Suspension & Resumption Criteria

Suspend testing if:

- A build or environment failure blocks execution
- Recovery or reconciliation fails on clean test data

Resume testing if:

- Blocking defects are fixed and the build is stable
- Failed cases and affected regression cases pass



## 13. Test Case Management & Traceability

The SRS section 8 RTM maps each requirement to a test case. TC-REL-01 checks incomplete batches, once-only replay, and account-record recovery. Examples:

- BMS-F-011 (account lockout) -> TC-AUT-02
- BMS-F-050 (fund transfer) -> TC-TRF-01
- BMS-NF-002 (crash-safe journal) -> TC-REL-01



## 14. Test Metrics & Reporting

Metrics collected:

- % test cases executed
- % passed/failed
- Defect density
- Defect aging
- Requirement coverage

Reports:

- Sprint execution status
- Final Test Summary Report

## Approvals

| Role | Name | Signature / Email | Date |
|---|---|---|---|
| Course Coordinator |  |  |  |
| Person 1 - Requirements | Dhanush S (PES1UG24AM360) |  |  |
| Person 2 - Architecture & Design | Vidit Soni (PES1UG24AM318) |  |  |
| Person 3 - Test Plan | Adarsha E (PES1UG24AM334) |  |  |