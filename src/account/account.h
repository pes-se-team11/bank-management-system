#pragma once
// AccountModule data types (Person 1). AccountRecord mirrors the
// accounts.dat layout in SAD 9.4; the CRC is added by PersistenceModule.
#include <cstdint>
#include <string>

#include "core/money.h"

namespace bms {

using CustomerId = std::uint32_t;
using AccountNo = std::uint32_t;  // SAD 9.1

enum class AccountType { Savings, Current };
enum class AccountState { Active, Locked, Closed };

// One customer (BMS-F-001). A customer may hold several accounts.
struct Customer {
    CustomerId id = 0;
    std::string name;
    std::string address;
    std::string contact;
    std::string gov_id_ref;  // e.g. PAN; a reference, not a scanned document
};

struct AccountRecord {
    AccountNo acct_no = 0;
    CustomerId cust_id = 0;
    AccountType type = AccountType::Savings;
    AccountState state = AccountState::Active;
    Paise opening_balance = 0;
    Paise balance = 0;
    Paise overdraft_limit = 0;    // Current accounts only (BMS-F-033)
    Paise daily_withdrawn = 0;    // BMS-F-030
    Paise daily_transferred = 0;  // BMS-F-052
    std::uint32_t counter_date = 0;  // YYYYMMDD the daily counters belong to
    std::uint64_t last_seq = 0;      // last journal leg applied (SAD 9.5)
};

}  // namespace bms
