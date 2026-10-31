#include <filesystem>
#include <iostream>
#include <string>

#include "account/account_service.h"
#include "auth/auth.h"
#include "cli/menu.h"
#include "core/version.h"
#include "ledger/ledger.h"
#include "persistence/file_account_store.h"
#include "persistence/file_credential_store.h"
#include "transaction/transaction_service.h"

namespace {

constexpr bms::Paise kDepositCeiling = 20000000;

}  // namespace

int main() {
    std::cout << "Bank Management System v" << bms::kVersion << '\n';

    const std::string data_dir = "data";
    std::error_code ec;

    std::filesystem::create_directories(data_dir, ec);
    if (ec) {
        std::cerr << "Could not create the data folder: "
                  << ec.message() << '\n';
        return 1;
    }

    std::cout << "Data directory: " << data_dir << '\n';

    bms::persistence::FileAccountStore account_store(data_dir);
    bms::persistence::FileCredentialStore credential_store(data_dir);

    if (account_store.status() != bms::Error::Ok ||
        credential_store.status() != bms::Error::Ok) {
        std::cerr << "Storage initialization failed. Exiting.\n";
        return 1;
    }

    bms::ledger::Ledger ledger(
        data_dir + "/journal.log",
        data_dir + "/audit.log"
    );

    bms::AccountService accounts(account_store, ledger);
    bms::AuthService auth(credential_store);

    bms::TransactionService transactions(
        account_store,
        ledger,
        kDepositCeiling
    );

    bms::cli::Menu menu(
        std::cin,
        std::cout,
        accounts,
        auth,
        transactions,
        ledger,
        !credential_store.hasStaff()
    );

    menu.run();

    return 0;
}
