#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <string>

#include "account/account_store.h"
#include "core/error.h"
#include "ledger/ledger.h"

namespace bms {

// Business limits the transaction rules enforce. Every default means "no
// limit", so a service built without limits behaves as before; main.cpp
// supplies the bank's real values.
struct TransactionLimits {
    // BMS-F-032: a Savings withdrawal or transfer may not leave less than this.
    Paise savings_minimum_balance = 0;
    // BMS-F-030: total a single account may withdraw in one bank day.
    Paise daily_withdrawal_limit = std::numeric_limits<Paise>::max();
    // BMS-F-052: one transfer may not exceed this ...
    Paise transfer_per_txn_ceiling = std::numeric_limits<Paise>::max();
    // ... and one source account may not transfer more than this per day.
    Paise transfer_daily_ceiling = std::numeric_limits<Paise>::max();
};

// The bank day as YYYYMMDD. Injected so tests can move the clock.
using DateProvider = std::function<std::uint32_t()>;

// Today's date in India Standard Time (UTC+05:30), from the system clock.
std::uint32_t bankDateToday();

class TransactionService {
public:
    TransactionService(
        AccountStore& account_store,
        ledger::Ledger& ledger,
        Paise deposit_ceiling,
        TransactionLimits limits = {},
        DateProvider today = {}
    );

    Error deposit(
        AccountNo account,
        Paise amount,
        const std::string& operator_id
    );

    // BMS-F-030, BMS-F-032, BMS-F-033. Refusals: InvalidInput (bad amount,
    // not ACTIVE, not enough funds), BelowMinimumBalance, DailyLimitExceeded.
    Error withdraw(
        AccountNo account,
        Paise amount,
        const std::string& operator_id
    );

    // BMS-F-050..052. Moves `amount` from `from` to `to` as one unit: both
    // balances change or neither does. Refusals, all before any balance is
    // touched: SameAccount, AccountNotFound, AccountClosed, AccountLocked,
    // TransferCeilingExceeded, DailyLimitExceeded, BelowMinimumBalance,
    // InvalidInput (bad amount, insufficient funds, overflow).
    Error transfer(
        AccountNo from,
        AccountNo to,
        Paise amount,
        const std::string& operator_id
    );

    // BMS-F-040: read the stored balance without writing a journal entry.
    // Leaves out unchanged if the account lookup fails.
    Error balance(AccountNo account, Paise& out) const;

private:
    // Lowest balance a debit may leave behind for this account.
    Paise floorFor(const AccountRecord& record) const;
    // Zeroes the daily counters when the record belongs to an earlier day.
    void rollDailyCounters(AccountRecord& record) const;

    AccountStore& account_store_;
    ledger::Ledger& ledger_;
    Paise deposit_ceiling_;
    TransactionLimits limits_;
    DateProvider today_;
};

}  // namespace bms
