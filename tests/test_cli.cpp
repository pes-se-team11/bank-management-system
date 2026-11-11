#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "account/account_service.h"
#include "auth/auth.h"
#include "cli/in_memory_stores.h"
#include "cli/menu.h"
#include "ledger/ledger.h"
#include "report/report_service.h"
#include "transaction/transaction_service.h"

namespace {

// Runs the menu on a typed script and returns everything it printed.
std::string runScript(const std::string& script, const std::string& tag) {
    const std::string journal = "test_cli_" + tag + "_journal.tmp";
    const std::string audit = "test_cli_" + tag + "_audit.tmp";
    std::remove(journal.c_str());
    std::remove(audit.c_str());

    std::string output;
    {
        bms::cli::InMemoryAccountStore account_store;
        bms::cli::InMemoryCredentialStore credential_store;
        bms::ledger::Ledger ledger(journal, audit);
        bms::AccountService accounts(account_store, ledger);
        bms::AuthService auth(credential_store, account_store, ledger, 1000);
        bms::TransactionService transactions(account_store, ledger, 20000000);
        bms::ReportService reports(ledger);

        std::istringstream in(script);
        std::ostringstream out;
        bms::cli::Menu menu(in, out, accounts, auth, transactions, ledger, reports,
                            !credential_store.hasStaff());
        menu.run();
        output = out.str();
    }

    std::remove(journal.c_str());
    std::remove(audit.c_str());
    return output;
}

// First run creates manager "admin", who adds teller "teller1" and logs
// out; teller1 then logs in, ready at the teller menu (BMS-F-012).
const std::string kTellerOnDuty =
    "admin\nPassword123\n"
    "1\nadmin\nPassword123\n"
    "1\nteller1\n1\nTellerPass1\n"
    "0\n"
    "1\nteller1\nTellerPass1\n";

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

TEST_CASE("formatPaise shows rupees with two decimals") {
    CHECK(bms::cli::formatPaise(0) == "0.00");
    CHECK(bms::cli::formatPaise(5) == "0.05");
    CHECK(bms::cli::formatPaise(125050) == "1250.50");
    CHECK(bms::cli::formatPaise(-1500) == "-15.00");
}

TEST_CASE("formatTimestamp shows Indian Standard Time") {
    CHECK(bms::cli::formatTimestamp(0) == "01-01-1970 05:30");
    // 2026-10-29 13:10:00 UTC is 18:40 IST.
    CHECK(bms::cli::formatTimestamp(1793279400) == "29-10-2026 18:40");
    // Late UTC evening rolls over to the next day in IST.
    CHECK(bms::cli::formatTimestamp(1798741800) == "01-01-2027 00:00");
}

TEST_CASE("Mini-statement lists the newest transactions for staff and customers") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"   // open with Rs 500
        "3\n100001\n250\n"         // deposit Rs 250
        "4\n100001\n100\n"         // withdraw Rs 100
        "6\n100001\n"               // staff mini-statement
        "6\n424242\n"               // unknown account
        "0\n"
        "2\n100001\n1234\n"
        "2\n0\n"                    // customer mini-statement, logout
        "0\n";

    const std::string out = runScript(script, "statement");

    CHECK(contains(out, "Mini-statement for account 100001 (newest first, IST)"));
    CHECK(contains(out, "WITHDRAW    -100.00  balance Rs 650.00"));
    CHECK(contains(out, "DEPOSIT     +250.00  balance Rs 750.00"));
    CHECK(contains(out, "OPEN        +500.00  balance Rs 500.00"));
    CHECK(contains(out, "No account has that number."));
    // Newest first: the withdrawal is listed before the opening.
    CHECK(out.find("WITHDRAW    -100.00") < out.find("OPEN        +500.00"));
    // Shown once for staff and once for the customer.
    const std::string heading = "Mini-statement for account 100001";
    const std::size_t first = out.find(heading);
    REQUIRE(first != std::string::npos);
    CHECK(out.find(heading, first + 1) != std::string::npos);
}

TEST_CASE("Full teller and customer flow through the menu") {
    const std::string script =
        kTellerOnDuty +      // manager admin adds teller1, who logs in
        "1\nAsha Rao\n12 MG Road, Bengaluru\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n"     // open savings account for customer 1 with Rs 500
        "1234\n"             // customer PIN
        "3\n100001\n250\n"   // deposit Rs 250
        "4\n100001\n100\n"   // withdraw Rs 100
        "5\n100001\n"        // balance
        "0\n"                // logout
        "2\n100001\n9999\n"  // customer login with the wrong PIN
        "2\n100001\n1234\n"  // customer login with the right PIN
        "1\n0\n"             // balance, logout
        "0\n";               // exit

    const std::string out = runScript(script, "flow");

    CHECK(contains(out, "Manager admin created."));
    CHECK(contains(out, "Welcome, admin."));
    CHECK(contains(out, "Staff login teller1 created."));
    CHECK(contains(out, "Welcome, teller1."));
    CHECK(contains(out, "Customer created. Customer ID: 1"));
    CHECK(contains(out, "Savings account opened. Account number: 100001"));
    CHECK(contains(out, "PIN set."));
    CHECK(contains(out, "Deposited Rs 250.00."));
    CHECK(contains(out, "Withdrew Rs 100.00."));
    CHECK(contains(out, "balance: Rs 650.00"));
    CHECK(contains(out, "Login failed"));
    CHECK(contains(out, "Logged in to account 100001."));
    CHECK(contains(out, "Goodbye."));
}

TEST_CASE("Menu explains refusals and recovers from bad input") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n12\n1234\n" // PIN too short, then a valid one
        "4\n100001\n600\n"         // more than the balance
        "3\n100001\n12,000\n250\n" // badly formatted amount, re-prompted
        "5\n424242\n"              // unknown account
        "0\n"
        "9\n"                      // not a main-menu option
        "0\n";

    const std::string out = runScript(script, "refusals");

    CHECK(contains(out, "Please choose 1, 2 or 0."));
    CHECK(contains(out, "PIN must be 4-6 digits."));
    CHECK(contains(out, "Withdrawal refused"));
    CHECK(contains(out, "Enter an amount like 500"));
    CHECK(contains(out, "Deposited Rs 250.00."));
    CHECK(contains(out, "No account has that number."));
    CHECK(contains(out, "Goodbye."));
}

TEST_CASE("TC-SEC-05: invalid numbers and choices re-prompt and change nothing") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\nabc\n1\n3\n1\n500\n1234\n"  // bad customer ID and type, re-prompted
        "3\nxyz\n100001\nfive\n-5\n1e3\n" // bad account, then 3 bad amounts
        "5\n100001\n"
        "42\n"                          // menu choice out of range
        "0\n0\n";

    const std::string out = runScript(script, "reprompt");

    CHECK(contains(out, "Customer ID must be a positive whole number."));
    CHECK(contains(out, "Please choose a number from 1 to 2."));
    CHECK(contains(out, "Savings account opened. Account number: 100001"));
    CHECK(contains(out, "Account number must be a positive whole number."));
    CHECK(contains(out, "Too many invalid entries. Nothing was changed"));
    CHECK_FALSE(contains(out, "Deposited"));
    CHECK(contains(out, "balance: Rs 500.00"));  // unchanged after the bad entries
    CHECK(contains(out, "Please choose one of the listed options."));
}

TEST_CASE("TC-SEC-04: an over-long name is refused safely and the menu carries on") {
    const std::string huge(10000, 'A');
    const std::string script = kTellerOnDuty + "1\n" + huge +
                               "\n12 MG Road\n9876543210\nABCDE1234F\n"
                               "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
                               "0\n0\n";

    const std::string out = runScript(script, "longname");

    CHECK(contains(out, "Customer not created. Name up to 60 characters"));
    CHECK(contains(out, "Customer created. Customer ID: 1"));  // no ID was used up
    CHECK(contains(out, "Goodbye."));
}

TEST_CASE("TC-AUT-03: each role gets its own menu and input ending exits cleanly") {
    const std::string script =
        "admin\nPassword123\n"
        "1\nadmin\nPassword123\n"
        "8\n"                            // managers have no option 8
        "1\nteller1\n1\nTellerPass1\n"  // manager adds a teller
        "0\n"
        "1\nteller1\nTellerPass1\n"
        "10\n";                          // tellers have no option 10; input ends

    const std::string out = runScript(script, "roles");

    CHECK(contains(out, "Manager menu (admin)"));
    CHECK(contains(out, "Staff login teller1 created."));
    CHECK(contains(out, "Teller menu (teller1)"));
    // Account work is not offered to a manager, nor staff admin to a teller.
    const std::string manager_menu = out.substr(0, out.find("Teller menu"));
    CHECK_FALSE(contains(manager_menu, "Create customer"));
    CHECK_FALSE(contains(manager_menu, "Open account"));
    const std::string teller_menu = out.substr(out.find("Teller menu"));
    CHECK_FALSE(contains(teller_menu, "Add staff login"));
    const std::string listed = "Please choose one of the listed options.";
    const std::size_t first = out.find(listed);
    REQUIRE(first != std::string::npos);
    CHECK(out.find(listed, first + 1) != std::string::npos);  // once for each
}

TEST_CASE("Opening an account journals it, and input ending at the PIN creates nothing") {
    const std::string journal = "test_cli_open_journal.tmp";
    const std::string audit = "test_cli_open_audit.tmp";
    std::remove(journal.c_str());
    std::remove(audit.c_str());
    {
        bms::cli::InMemoryAccountStore account_store;
        bms::cli::InMemoryCredentialStore credential_store;
        bms::ledger::Ledger ledger(journal, audit);
        bms::AccountService accounts(account_store, ledger);
        bms::AuthService auth(credential_store, account_store, ledger, 1000);
        bms::TransactionService transactions(account_store, ledger, 20000000);

        // First session: one account opened with Rs 500, then a second
        // opening abandoned at the PIN prompt.
        std::istringstream in(
            kTellerOnDuty +
            "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
            "2\n1\n1\n500\n1234\n"
            "2\n1\n1\n700\n");
        std::ostringstream out;
        bms::ReportService reports(ledger);
        bms::cli::Menu menu(in, out, accounts, auth, transactions, ledger, reports, true);
        menu.run();

        std::vector<bms::AccountRecord> list;
        REQUIRE(accounts.listAccounts(1, list) == bms::Error::Ok);
        CHECK(list.size() == 1);  // the abandoned opening left no account

        std::vector<bms::ledger::JournalRecord> entries;
        REQUIRE(ledger.readJournal(entries) == bms::Error::Ok);
        REQUIRE(entries.size() == 1);
        CHECK(entries[0].type == bms::ledger::TxnType::Open);
        CHECK(entries[0].amount == 50000);
        CHECK(entries[0].operator_id == "teller1");
    }
    std::remove(journal.c_str());
    std::remove(audit.c_str());
}

TEST_CASE("TC-AUT-02: three wrong PINs lock the account in the menu") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"
        "0\n"
        "2\n100001\n1111\n"   // wrong
        "2\n100001\n2222\n"   // wrong
        "2\n100001\n3333\n"   // wrong: locks the account
        "2\n100001\n1234\n"   // right PIN, still refused
        "1\nteller1\nTellerPass1\n"
        "7\n1\n"               // the teller's list shows the account as Locked
        "0\n0\n";

    const std::string out = runScript(script, "lockout");

    CHECK(contains(out, "This account is locked after three wrong PINs."));
    CHECK_FALSE(contains(out, "Logged in to account 100001."));
    CHECK(contains(out, "100001  Savings  Locked"));
}

TEST_CASE("TC-AUT-04: a manager unlocks a locked account from the menu") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"
        "0\n"
        "2\n100001\n1111\n"
        "2\n100001\n2222\n"
        "2\n100001\n3333\n"   // locked
        "1\nadmin\nPassword123\n"
        "5\n100001\n2\n"      // not verified: nothing happens
        "5\n100001\n1\n"      // verified: unlocked
        "5\n100001\n1\n"      // already unlocked
        "0\n"
        "2\n100001\n1234\n"   // the right PIN works again
        "0\n0\n";

    const std::string out = runScript(script, "unlock");

    CHECK(contains(out, "This account is locked after three wrong PINs."));
    CHECK(contains(out, "Not unlocked. Verify the holder first."));
    CHECK(contains(out, "Account 100001 unlocked."));
    CHECK(contains(out, "That account is not locked, so there is nothing to unlock."));
    CHECK(contains(out, "Logged in to account 100001."));
}

TEST_CASE("TC-LED-03: a manager runs the end-of-day report from the menu") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"
        "3\n100001\n250\n"    // deposit Rs 250
        "4\n100001\n100\n"    // withdraw Rs 100
        "0\n"
        "1\nadmin\nPassword123\n"
        "7\n"                 // the manager's end-of-day report
        "0\n0\n";

    const std::string out = runScript(script, "eod");

    CHECK(contains(out, "End-of-day report"));
    CHECK(contains(out, "  Deposits:     Rs 250.00"));
    CHECK(contains(out, "  Withdrawals:  Rs 100.00"));
    CHECK(contains(out, "  Transfer out: Rs 0.00"));
    CHECK(contains(out, "  Transfer in:  Rs 0.00"));
    CHECK(contains(out, "  100001  Rs 650.00"));
    CHECK(contains(out, "Report recorded in the audit log."));
}

TEST_CASE("TC-ACC-03: a teller modifies customer details from the menu") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "8\n1\n\n9123456780\n"   // keep the address, change the contact
        "8\n1\n\n\n"             // keep both: nothing to do
        "8\n1\n\n123\n"          // invalid contact
        "8\n42\n"                // unknown customer
        "7\n1\n"
        "0\n0\n";

    const std::string out = runScript(script, "modify");

    CHECK(contains(out, "Current contact: 9876543210"));
    CHECK(contains(out, "Customer 1 updated."));
    CHECK(contains(out, "No changes made."));
    CHECK(contains(out, "Not changed. Address up to 120 characters"));
    CHECK(contains(out, "No customer has that ID."));
}

TEST_CASE("TC-ACC-04: a manager closes an emptied account from the menu") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"
        "0\n"
        "1\nadmin\nPassword123\n"
        "6\n100001\n1\n"          // money left: refused
        "0\n"
        "1\nteller1\nTellerPass1\n"
        "4\n100001\n500\n"        // empty it
        "0\n"
        "1\nadmin\nPassword123\n"
        "6\n100001\n2\n"          // changed mind
        "6\n100001\n1\n"          // closed
        "0\n"
        "1\nteller1\nTellerPass1\n"
        "3\n100001\n100\n"        // deposit refused
        "7\n1\n"
        "0\n"
        "2\n100001\n1234\n"       // customer login refused
        "0\n";

    const std::string out = runScript(script, "close");

    CHECK(contains(out, "Only an account with a zero balance can be closed."));
    CHECK(contains(out, "Not closed."));
    CHECK(contains(out, "Account 100001 closed."));
    CHECK(contains(out, "100001  Savings  Closed"));
    CHECK(contains(out, "That account is closed."));
    CHECK_FALSE(contains(out, "Deposited Rs 100.00."));
    CHECK_FALSE(contains(out, "Logged in to account 100001."));
}

TEST_CASE("TC-ACC-04: a closed account cannot be reopened through lockout and unlock") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n2\n0\n1234\n"     // open a zero-balance Current account
        "0\n"
        "1\nadmin\nPassword123\n"
        "6\n100001\n1\n"         // closed
        "0\n"
        "2\n100001\n1111\n"
        "2\n100001\n2222\n"
        "2\n100001\n3333\n"      // three wrong PINs
        "1\nadmin\nPassword123\n"
        "5\n100001\n1\n"         // unlock refused
        "0\n"
        "1\nteller1\nTellerPass1\n"
        "3\n100001\n100\n"       // deposit refused
        "7\n1\n"
        "0\n"
        "2\n100001\n1234\n"      // login refused
        "0\n";

    const std::string out = runScript(script, "close_reopen");

    CHECK(contains(out, "Account 100001 closed."));
    CHECK_FALSE(contains(out, "locked after three wrong PINs"));
    CHECK_FALSE(contains(out, "Account 100001 unlocked."));
    CHECK_FALSE(contains(out, "Deposited Rs 100.00."));
    CHECK_FALSE(contains(out, "Logged in to account 100001."));
    CHECK(contains(out, "100001  Current  Closed"));
}

TEST_CASE("Staff IDs must be 3-16 letters or digits") {
    const std::string out = runScript("ab\nadmin 1\nadmin\nPassword123\n0\n", "staffid");
    CHECK(contains(out, "Staff ID must be 3-16 letters or digits."));
    CHECK(contains(out, "Manager admin created."));
}

}  // namespace

TEST_CASE("TC-TRF-01/02: transfer through the teller and customer menus") {
    const std::string script =
        kTellerOnDuty +
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"          // account 100001, Rs 500
        "2\n1\n1\n500\n5678\n"          // account 100002, Rs 500
        "9\n100001\n100002\n200\n"      // teller moves Rs 200 across
        "9\n100001\n100001\n10\n"       // same account: refused
        "9\n100001\n100002\n9999\n"     // more than the balance: refused
        "9\n100001\n999999\n10\n"       // unknown destination: refused
        "0\n"
        "2\n100002\n5678\n"             // the receiving customer logs in
        "5\n100001\n50\n"               // and sends Rs 50 back
        "0\n0\n";

    const std::string out = runScript(script, "transfer");

    CHECK(contains(out, "9. Transfer funds"));
    CHECK(contains(out, "5. Transfer to another account"));
    CHECK(contains(out, "Transferred Rs 200.00 from account 100001 to account 100002."));
    CHECK(contains(out, "The two accounts are the same."));
    CHECK(contains(out, "No money was moved."));
    CHECK(contains(out, "No account has that number."));
    CHECK(contains(out, "Transferred Rs 50.00 from account 100002 to account 100001."));
}

TEST_CASE("Menu describes the new limit refusals in plain words") {
    bms::TransactionLimits limits;
    limits.savings_minimum_balance = 50000;
    limits.daily_withdrawal_limit = 10000;
    limits.transfer_per_txn_ceiling = 5000;
    const std::string journal = "test_cli_limits_journal.tmp";
    const std::string audit = "test_cli_limits_audit.tmp";
    std::remove(journal.c_str());
    std::remove(audit.c_str());
    std::string output;
    {
        bms::cli::InMemoryAccountStore account_store;
        bms::cli::InMemoryCredentialStore credential_store;
        bms::ledger::Ledger ledger(journal, audit);
        bms::AccountService accounts(account_store, ledger);
        bms::AuthService auth(credential_store, account_store, ledger, 1000);
        bms::TransactionService transactions(account_store, ledger, 20000000, limits);
        bms::ReportService reports(ledger);
        std::istringstream in(
            kTellerOnDuty +
            "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
            "2\n1\n1\n1000\n1234\n"
            "2\n1\n1\n500\n5678\n"
            "4\n100001\n600\n"            // would leave Rs 400, under the Rs 500 minimum
            "4\n100001\n150\n"            // Rs 150 > the Rs 100 daily limit
            "9\n100001\n100002\n60\n"     // Rs 60 > the Rs 50 transfer ceiling
            "0\n0\n");
        std::ostringstream out;
        bms::cli::Menu menu(in, out, accounts, auth, transactions, ledger, reports, true);
        menu.run();
        output = out.str();
    }
    std::remove(journal.c_str());
    std::remove(audit.c_str());
    CHECK(contains(output, "below its minimum balance"));
    CHECK(contains(output, "pass today's limit"));
    CHECK(contains(output, "more than one transfer may carry"));
}