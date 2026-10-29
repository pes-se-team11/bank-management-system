#pragma once

#include <cstdint>
#include <string>

#include "account/account_store.h"
#include "core/error.h"
#include "ledger/ledger.h"

namespace bms {

class TransactionService {
public:
    TransactionService(
        AccountStore& account_store,
        ledger::Ledger& ledger,
        Paise deposit_ceiling
    );

    Error deposit(
        AccountNo account,
        Paise amount,
        const std::string& operator_id
    );

    Error withdraw(
        AccountNo account,
        Paise amount,
        const std::string& operator_id
    );

    // BMS-F-040: read the stored balance without writing a journal entry.
    // Leaves out unchanged if the account lookup fails.
    Error balance(AccountNo account, Paise& out) const;

private:
    AccountStore& account_store_;
    ledger::Ledger& ledger_;
    Paise deposit_ceiling_;
};

}  // namespace bms
