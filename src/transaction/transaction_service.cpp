
#include "transaction/transaction_service.h"

#include <limits>

namespace bms {

TransactionService::TransactionService(
    AccountStore& account_store,
    ledger::Ledger& ledger,
    Paise deposit_ceiling
)
    : account_store_(account_store),
      ledger_(ledger),
      deposit_ceiling_(deposit_ceiling) {}

Error TransactionService::deposit(
    AccountNo account,
    Paise amount,
    const std::string& operator_id
) {
    if (amount <= 0 || deposit_ceiling_ <= 0 ||
        amount > deposit_ceiling_ || operator_id.empty()) {
        return Error::InvalidInput;
    }

    AccountRecord record{};
    Error result = account_store_.readAccount(account, record);
    if (result != Error::Ok) {
        return result;
    }

    if (record.state != AccountState::Active) {
        return Error::InvalidInput;
    }

    if (record.balance >
        std::numeric_limits<Paise>::max() - amount) {
        return Error::InvalidInput;
    }

    const Paise new_balance = record.balance + amount;

    std::uint64_t sequence = 0;
    result = ledger_.appendTransaction(
        ledger::TxnType::Deposit,
        account,
        amount,
        new_balance,
        operator_id,
        sequence
    );

    if (result != Error::Ok) {
        return result;
    }

    record.balance = new_balance;
    record.last_seq = sequence;

    return account_store_.updateAccount(record);
}

}  // namespace bms
