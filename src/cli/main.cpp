// Entry point. Wires the stores and services together and starts the
// line-based menu (v1.0). Business logic never lives here: rules belong in
// the service layer (BMS-SR-007).
#include <filesystem>
#include <iostream>
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

    std::error_code ec;
    std::filesystem::create_directories("data", ec);
    if (ec) {
        std::cerr << "Could not create the data folder: " << ec.message() << '\n';
        return 1;
    }

    // Accounts and logins are kept in memory until file storage lands; the
    // journal and audit log are already written to data/.
    bms::cli::InMemoryAccountStore account_store;
    bms::cli::InMemoryCredentialStore credential_store;
    bms::ledger::Ledger ledger("data/journal.log", "data/audit.log");

    bms::AccountService accounts(account_store);
    bms::AuthService auth(credential_store);
    bms::TransactionService transactions(account_store, ledger, kDepositCeiling);

    bms::cli::Menu menu(std::cin, std::cout, accounts, auth, transactions,
                        !credential_store.hasStaff());
    menu.run();
    return 0;
}
