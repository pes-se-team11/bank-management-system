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

        std::istringstream in(script);
        std::ostringstream out;
        bms::cli::Menu menu(in, out, accounts, auth, transactions, ledger,
                            !credential_store.hasStaff());
        menu.run();
        output = out.str();
    }

    std::remove(journal.c_str());
    std::remove(audit.c_str());
    return output;
}

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
        "admin\nPassword123\n"
        "1\nadmin\nPassword123\n"
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
        "admin\n"            // first-run manager ID
        "Password123\n"      // manager password
        "1\nadmin\nPassword123\n"  // staff login
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
        "admin\nPassword123\n"
        "9\n"                      // not a menu option
        "1\nadmin\nPassword123\n"
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n100\n12\n1234\n" // PIN too short, then a valid one
        "4\n100001\n500\n"         // more than the balance
        "3\n100001\n12,000\n"      // badly formatted amount
        "5\n424242\n"              // unknown account
        "0\n0\n";

    const std::string out = runScript(script, "refusals");

    CHECK(contains(out, "Please choose 1, 2 or 0."));
    CHECK(contains(out, "PIN must be 4-6 digits."));
    CHECK(contains(out, "Withdrawal refused"));
    CHECK(contains(out, "Enter an amount like 500"));
    CHECK(contains(out, "No account has that number."));
    CHECK(contains(out, "Goodbye."));
}

TEST_CASE("Tellers cannot add staff and input ending exits cleanly") {
    const std::string script =
        "admin\nPassword123\n"
        "1\nadmin\nPassword123\n"
        "8\nteller1\n1\nTellerPass1\n"  // manager adds a teller
        "0\n"
        "1\nteller1\nTellerPass1\n"
        "8\n";                          // teller tries to add staff; input ends

    const std::string out = runScript(script, "roles");

    CHECK(contains(out, "Staff login teller1 created."));
    CHECK(contains(out, "Welcome, teller1."));
    CHECK(contains(out, "Only a manager can add staff."));
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
            "admin\nPassword123\n"
            "1\nadmin\nPassword123\n"
            "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
            "2\n1\n1\n500\n1234\n"
            "2\n1\n1\n700\n");
        std::ostringstream out;
        bms::cli::Menu menu(in, out, accounts, auth, transactions, ledger, true);
        menu.run();

        std::vector<bms::AccountRecord> list;
        REQUIRE(accounts.listAccounts(1, list) == bms::Error::Ok);
        CHECK(list.size() == 1);  // the abandoned opening left no account

        std::vector<bms::ledger::JournalRecord> entries;
        REQUIRE(ledger.readJournal(entries) == bms::Error::Ok);
        REQUIRE(entries.size() == 1);
        CHECK(entries[0].type == bms::ledger::TxnType::Open);
        CHECK(entries[0].amount == 50000);
        CHECK(entries[0].operator_id == "admin");
    }
    std::remove(journal.c_str());
    std::remove(audit.c_str());
}

TEST_CASE("TC-AUT-02: three wrong PINs lock the account in the menu") {
    const std::string script =
        "admin\nPassword123\n"
        "1\nadmin\nPassword123\n"
        "1\nAsha Rao\n12 MG Road\n9876543210\nABCDE1234F\n"
        "2\n1\n1\n500\n1234\n"
        "0\n"
        "2\n100001\n1111\n"   // wrong
        "2\n100001\n2222\n"   // wrong
        "2\n100001\n3333\n"   // wrong: locks the account
        "2\n100001\n1234\n"   // right PIN, still refused
        "1\nadmin\nPassword123\n"
        "7\n1\n"               // staff list shows the account as Locked
        "0\n0\n";

    const std::string out = runScript(script, "lockout");

    CHECK(contains(out, "This account is locked after three wrong PINs."));
    CHECK_FALSE(contains(out, "Logged in to account 100001."));
    CHECK(contains(out, "100001  Savings  Locked"));
}

TEST_CASE("Staff IDs must be 3-16 letters or digits") {
    const std::string out = runScript("ab\nadmin 1\nadmin\nPassword123\n0\n", "staffid");
    CHECK(contains(out, "Staff ID must be 3-16 letters or digits."));
    CHECK(contains(out, "Manager admin created."));
}

}  // namespace
