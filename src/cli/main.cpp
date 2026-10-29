// Entry point. Wires the stores and services together and starts the
// line-based menu (v1.0). Business logic never lives here: rules belong in
// the service layer (BMS-SR-007).
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

#include "account/account_service.h"
#include "auth/auth.h"
#include "cli/in_memory_stores.h"
#include "cli/menu.h"
#include "core/version.h"
#include "ledger/ledger.h"
#include "transaction/transaction_service.h"

namespace {

// Largest single deposit: Rs 2,00,000 (in paise).
constexpr bms::Paise kDepositCeiling = 20000000;

}  // namespace

int main() {
    std::cout << "Bank Management System v" << bms::kVersion << '\n';

    // Accounts and logins are kept in memory until file storage (BMS-38)
    // lands, so account numbers restart at 100001 every run. Each run
    // therefore gets its own journal folder: sharing one journal would mix
    // two different accounts under the same number. Once accounts are saved,
    // this goes back to a single data/journal.log.
    const std::string run_dir =
        "data/run-" + std::to_string(static_cast<long long>(std::time(nullptr)));
    std::error_code ec;
    std::filesystem::create_directories(run_dir, ec);
    if (ec) {
        std::cerr << "Could not create the data folder: " << ec.message() << '\n';
        return 1;
    }
    std::cout << "This session's journal: " << run_dir << "/journal.log\n";

    bms::cli::InMemoryAccountStore account_store;
    bms::cli::InMemoryCredentialStore credential_store;
    bms::ledger::Ledger ledger(run_dir + "/journal.log", run_dir + "/audit.log");

    bms::AccountService accounts(account_store, ledger);
    bms::AuthService auth(credential_store);
    bms::TransactionService transactions(account_store, ledger, kDepositCeiling);

    bms::cli::Menu menu(std::cin, std::cout, accounts, auth, transactions,
                        !credential_store.hasStaff());
    menu.run();
    return 0;
}
