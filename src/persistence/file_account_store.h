#pragma once
// File-backed AccountStore implementation (SAD 9.3, BMS-38).
// Persists customer records to customers.dat, account records to accounts.dat,
// and monotonic counters to counters.dat in a specified data directory.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "account/account.h"
#include "account/account_store.h"
#include "core/error.h"

namespace bms::persistence {

class FileAccountStore final : public AccountStore {
public:
    explicit FileAccountStore(std::string data_dir);

    // Returns the storage initialization status.
    Error status() const { return init_status_; }

    Error nextCustomerId(CustomerId& out) override;
    Error nextAccountNo(AccountNo& out) override;

    Error writeCustomer(const Customer& customer) override;
    Error readCustomer(CustomerId id, Customer& out) override;

    Error createAccount(const AccountRecord& account) override;
    Error updateAccount(const AccountRecord& account) override;
    Error readAccount(AccountNo no, AccountRecord& out) override;
    Error listAccounts(CustomerId id, std::vector<AccountRecord>& out) override;

private:
    Error scanExistingFiles();
    Error loadCounters();
    Error saveCounters();

    std::string data_dir_;
    std::string customers_path_;
    std::string accounts_path_;
    std::string counters_path_;

    CustomerId next_customer_ = 1;
    AccountNo next_account_ = 100001;

    std::unordered_map<CustomerId, std::uint64_t> customer_offsets_;
    std::unordered_map<AccountNo, std::uint64_t> account_offsets_;

    Error init_status_ = Error::Ok;
};

}  // namespace bms::persistence