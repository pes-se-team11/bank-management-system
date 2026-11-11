
#include "transaction/transaction_service.h"

#include <chrono>
#include <limits>
#include <utility>

namespace bms {

namespace {

constexpr std::int64_t kIndiaOffsetSeconds = 5 * 3600 + 30 * 60;

// True when `amount` can be taken from `balance` without going under `floor`.
// Written so no intermediate value overflows (BMS-SR-003).
bool debitFits(Paise balance, Paise floor, Paise amount) {
    if (balance < floor) {
        return false;
    }
    if (floor < 0 && balance > std::numeric_limits<Paise>::max() + floor) {
        return true;  // more is available than any Paise amount can ask for
    }
    return amount <= balance - floor;
}

// True when `used + amount` stays within `limit`, overflow-free.
bool withinLimit(Paise used, Paise amount, Paise limit) {
    return used <= limit && amount <= limit - used;
}

}  // namespace

std::uint32_t bankDateToday() {
    const std::int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::int64_t z = (now + kIndiaOffsetSeconds) / 86400 + 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const std::int64_t doe = z - era * 146097;
    const std::int64_t yoe =
        (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    const std::int64_t day = doy - (153 * mp + 2) / 5 + 1;
    const std::int64_t month = mp < 10 ? mp + 3 : mp - 9;
    const std::int64_t year = yoe + era * 400 + (month <= 2 ? 1 : 0);
    return static_cast<std::uint32_t>(year * 10000 + month * 100 + day);
}

TransactionService::TransactionService(
    AccountStore& account_store,
    ledger::Ledger& ledger,
    Paise deposit_ceiling,
    TransactionLimits limits,
    DateProvider today
)
    : account_store_(account_store),
      ledger_(ledger),
      deposit_ceiling_(deposit_ceiling),
      limits_(limits),
      today_(today ? std::move(today) : DateProvider(bankDateToday)) {}

Paise TransactionService::floorFor(const AccountRecord& record) const {
    if (record.type == AccountType::Current) {
        return -record.overdraft_limit;
    }
    return limits_.savings_minimum_balance;
}

void TransactionService::rollDailyCounters(AccountRecord& record) const {
    const std::uint32_t today = today_();
    if (record.counter_date != today) {
        record.daily_withdrawn = 0;
        record.daily_transferred = 0;
        record.counter_date = today;
    }
}

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

Error TransactionService::withdraw(
    AccountNo account,
    Paise amount,
    const std::string& operator_id
) {
    if (amount <= 0 || operator_id.empty()) {
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

    if (record.type == AccountType::Current && record.overdraft_limit < 0) {
        return Error::InvalidInput;
    }

    // BMS-F-032 / BMS-F-033: balance floor for this account type.
    const Paise floor = floorFor(record);
    if (!debitFits(record.balance, floor, amount)) {
        const bool savings_floor = record.type == AccountType::Savings &&
                                   limits_.savings_minimum_balance > 0 &&
                                   amount <= record.balance;
        return savings_floor ? Error::BelowMinimumBalance
                             : Error::InvalidInput;
    }

    // BMS-F-030: today's running total, restarted on a new bank day.
    rollDailyCounters(record);
    if (!withinLimit(record.daily_withdrawn, amount,
                     limits_.daily_withdrawal_limit)) {
        return Error::DailyLimitExceeded;
    }

    const Paise new_balance = record.balance - amount;

    std::uint64_t sequence = 0;
    result = ledger_.appendTransaction(
        ledger::TxnType::Withdraw,
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
    record.daily_withdrawn += amount;
    record.last_seq = sequence;

    return account_store_.updateAccount(record);
}

Error TransactionService::transfer(
    AccountNo from,
    AccountNo to,
    Paise amount,
    const std::string& operator_id
) {
    if (amount <= 0 || operator_id.empty()) {
        return Error::InvalidInput;
    }
    if (from == to) {
        return Error::SameAccount;  // BMS-F-051, checked before any read
    }

    AccountRecord src{};
    AccountRecord dst{};
    Error result = account_store_.readAccount(from, src);
    if (result != Error::Ok) {
        return result;
    }
    result = account_store_.readAccount(to, dst);
    if (result != Error::Ok) {
        return result;
    }

    // BMS-F-051: both accounts must be ACTIVE; closed and locked are told apart.
    for (const AccountRecord* side : {&src, &dst}) {
        if (side->state == AccountState::Closed) {
            return Error::AccountClosed;
        }
        if (side->state == AccountState::Locked) {
            return Error::AccountLocked;
        }
    }

    // BMS-F-052: per-transaction ceiling, then the running daily total.
    if (amount > limits_.transfer_per_txn_ceiling) {
        return Error::TransferCeilingExceeded;
    }
    const AccountRecord src_before = src;
    const AccountRecord dst_before = dst;
    rollDailyCounters(src);
    if (!withinLimit(src.daily_transferred, amount,
                     limits_.transfer_daily_ceiling)) {
        return Error::DailyLimitExceeded;
    }

    // The debit leg obeys the same balance floor as a withdrawal.
    if (src.type == AccountType::Current && src.overdraft_limit < 0) {
        return Error::InvalidInput;
    }
    if (!debitFits(src.balance, floorFor(src), amount)) {
        const bool savings_floor = src.type == AccountType::Savings &&
                                   limits_.savings_minimum_balance > 0 &&
                                   amount <= src.balance;
        return savings_floor ? Error::BelowMinimumBalance
                             : Error::InvalidInput;
    }
    if (dst.balance > std::numeric_limits<Paise>::max() - amount) {
        return Error::InvalidInput;  // the credit would overflow
    }

    src.balance -= amount;
    dst.balance += amount;
    src.daily_transferred += amount;

    // Leg 1 and leg 2 go to the append-only journal first (write-ahead).
    std::uint64_t out_seq = 0;
    result = ledger_.appendTransaction(ledger::TxnType::TransferOut, from,
                                       amount, src.balance, operator_id,
                                       out_seq);
    if (result != Error::Ok) {
        return result;
    }

    // The journal cannot be edited, so a failure after leg 1 is undone with
    // reversing entries; reconciliation then still balances (BMS-F-061).
    const auto reverse_out = [&]() {
        std::uint64_t seq = 0;
        (void)ledger_.appendTransaction(ledger::TxnType::TransferIn, from,
                                        amount, src_before.balance,
                                        operator_id, seq);
    };
    const auto reverse_in = [&]() {
        std::uint64_t seq = 0;
        (void)ledger_.appendTransaction(ledger::TxnType::TransferOut, to,
                                        amount, dst_before.balance,
                                        operator_id, seq);
    };

    std::uint64_t in_seq = 0;
    result = ledger_.appendTransaction(ledger::TxnType::TransferIn, to, amount,
                                       dst.balance, operator_id, in_seq);
    if (result != Error::Ok) {
        reverse_out();
        return result;
    }

    src.last_seq = out_seq;
    dst.last_seq = in_seq;

    result = account_store_.updateAccount(src);
    if (result != Error::Ok) {
        reverse_in();
        reverse_out();
        return result;
    }

    result = account_store_.updateAccount(dst);
    if (result != Error::Ok) {
        // Put the source back exactly as it was, then reverse both legs.
        (void)account_store_.updateAccount(src_before);
        reverse_in();
        reverse_out();
        return result;
    }

    return Error::Ok;
}

Error TransactionService::balance(AccountNo account, Paise& out) const {
    AccountRecord record{};
    const Error result = account_store_.readAccount(account, record);
    if (result != Error::Ok) {
        return result;
    }

    out = record.balance;
    return Error::Ok;
}

}  // namespace bms
