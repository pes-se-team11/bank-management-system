#pragma once
// MenuLayer: the line-based console menu (v1.0). It only reads input, calls
// the service layer and prints results - no business rules live here
// (BMS-SR-007), and every failure is shown with its reason and what to do
// next (BMS-NF-005).
//
// It reads from any std::istream, so tests drive it with a script exactly
// the way a user would type (Test Plan: scriptable CLI).
#include <iosfwd>
#include <string>

#include "account/account_service.h"
#include "auth/auth.h"
#include "ledger/ledger.h"
#include "transaction/transaction_service.h"

namespace bms::cli {

class Menu {
public:
    Menu(std::istream& in, std::ostream& out, AccountService& accounts,
         AuthService& auth, TransactionService& transactions,
         const ledger::Ledger& journal, bool needs_first_manager);

    // Runs until the user chooses Exit or input ends.
    void run();

private:
    bool setupFirstManager();
    void staffLogin();
    void customerLogin();
    void staffMenu(const Session& session);
    void customerMenu(const Session& session);

    void createCustomer();
    void openAccount(const std::string& operator_id);
    void addStaff();
    void deposit(const std::string& operator_id, AccountNo account);
    void withdraw(const std::string& operator_id, AccountNo account);
    void showBalance(AccountNo account);
    void showMiniStatement(AccountNo account);
    void listAccounts();

    // Input helpers. Each returns false when input has ended.
    bool ask(const std::string& prompt, std::string& out);
    bool askSecret(const std::string& prompt, std::string& out);
    bool askChoice(const std::string& prompt, int& out);
    bool askAccountNo(AccountNo& out);
    bool askAmount(const std::string& prompt, Paise& out);

    void fail(Error error);

    std::istream& in_;
    std::ostream& out_;
    AccountService& accounts_;
    AuthService& auth_;
    TransactionService& transactions_;
    const ledger::Ledger& journal_;
    bool needs_first_manager_;
};

// "1250.50" style text for a paise amount; negative for an overdraft.
std::string formatPaise(Paise amount);

// "29-10-2026 18:40" for a journal timestamp, in Indian Standard Time
// (UTC+05:30). Pure arithmetic, so no OS time functions are needed.
std::string formatTimestamp(std::int64_t unix_ts);

// One-line reason and remedy for an error (BMS-NF-005).
std::string describe(Error error);

}  // namespace bms::cli
