#pragma once
// AccountModule service (Person 1): stories 1.1 and 1.2.
#include <string>
#include <vector>

#include "account/account.h"
#include "account/account_store.h"
#include "core/error.h"
#include "ledger/ledger.h"

namespace bms {

class AccountService {
public:
    AccountService(AccountStore& store, ledger::Ledger& journal)
        : store_(store), journal_(journal) {}

    // BMS-F-001. Validates every field, then stores the customer under a new,
    // never-reused ID. Invalid input uses up no ID.
    Error createCustomer(const Customer& details, CustomerId& out_id);

    // BMS-F-002. Opens a Savings or Current account for an existing
    // customer; the opening balance is the amount deposited at opening.
    // An OPEN journal record is written first, so the journal alone can
    // rebuild every balance (story 6.2 reconciliation).
    Error openAccount(CustomerId customer, AccountType type, Paise opening_deposit,
                      const std::string& operator_id, AccountNo& out_no);

    // BMS-F-005. Unknown keys return a not-found error, never a crash.
    Error findAccount(AccountNo no, AccountRecord& out);
    Error listAccounts(CustomerId customer, std::vector<AccountRecord>& out);

private:
    AccountStore& store_;
    ledger::Ledger& journal_;
};

}  // namespace bms
