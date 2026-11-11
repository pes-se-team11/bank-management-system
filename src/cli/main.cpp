#include <filesystem>
#include <iostream>
#include <string>

#include "account/account_service.h"
#include "auth/auth.h"
#include "cli/menu.h"
#include "core/version.h"
#include "ledger/ledger.h"
#include "persistence/file_account_store.h"
#include "platform/platform.h"
#include "persistence/file_credential_store.h"
#include "report/report_service.h"
#include "transaction/transaction_service.h"

namespace {

// All amounts in paise (BMS-NF-003).
constexpr bms::Paise kDepositCeiling = 20000000;  // Rs 2,00,000 per deposit

// BMS-F-030, BMS-F-032, BMS-F-052. Agreed values; change here only.
constexpr bms::TransactionLimits kLimits{
    50000,      // Savings minimum balance        Rs 500
    5000000,    // withdrawals per account / day  Rs 50,000
    10000000,   // one transfer                   Rs 1,00,000
    30000000,   // transfers per account / day    Rs 3,00,000
};

}  // namespace

int main() {
    std::cout << "Bank Management System v" << bms::kVersion << '\n';

    const std::string data_dir = "data";

    // BMS-SR-006: owner-only files, refuse a loosened folder, one copy only.
    std::string reason;
    if (!bms::platform::secureDataDirectory(data_dir, reason)) {
        std::cerr << reason << '\n';
        return 1;
    }
    bms::platform::InstanceLock instance_lock;
    if (!instance_lock.acquire(data_dir + "/.lock")) {
        std::cerr << "The bank system is already running on this data "
                     "folder. Close the other copy first.\n";
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
    bms::AuthService auth(credential_store, account_store, ledger);

    bms::TransactionService transactions(
        account_store,
        ledger,
        kDepositCeiling,
        kLimits
    );

    bms::ReportService reports(ledger);

    bms::cli::Menu menu(
        std::cin,
        std::cout,
        accounts,
        auth,
        transactions,
        ledger,
        reports,
        !credential_store.hasStaff()
    );

    menu.run();

    return 0;
}
