#pragma once
// What AccountModule needs from storage. PersistenceModule implements this
// over customers.dat / accounts.dat (SAD 9.3); tests use an in-memory one.
// Depending on this interface instead of the file code keeps the service
// layer unit-testable on its own (BMS-NF-006).
#include <vector>

#include "account/account.h"
#include "core/error.h"

namespace bms {

class AccountStore {
public:
    virtual ~AccountStore() = default;

    // IDs come from a counter the store persists, so they only ever grow and
    // an ID is never handed out twice - not even after a close (BMS-F-001).
    virtual Error nextCustomerId(CustomerId& out) = 0;
    virtual Error nextAccountNo(AccountNo& out) = 0;

    virtual Error writeCustomer(const Customer& customer) = 0;
    virtual Error readCustomer(CustomerId id, Customer& out) = 0;  // CustomerNotFound

    virtual Error createAccount(const AccountRecord& account) = 0;
    virtual Error readAccount(AccountNo no, AccountRecord& out) = 0;  // AccountNotFound
    virtual Error listAccounts(CustomerId id, std::vector<AccountRecord>& out) = 0;
};

}  // namespace bms
