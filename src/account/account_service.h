#pragma once
// AccountModule service (Person 1): stories 1.1-1.4. Roles are checked here
// rather than in the menu (BMS-F-012, BMS-SR-007): customer and account
// creation and changes are Teller operations, closure is a Manager one.
#include <string>
#include <vector>

#include "account/account.h"
#include "account/account_store.h"
#include "auth/auth.h"
#include "core/error.h"
#include "ledger/ledger.h"

namespace bms {

class AccountService {
public:
    AccountService(AccountStore& store, ledger::Ledger& journal)
        : store_(store), journal_(journal) {}

    // BMS-F-001. Validates every field, then stores the customer under a new,
    // never-reused ID. Invalid input uses up no ID. Tellers only.
    Error createCustomer(const Session& by, const Customer& details, CustomerId& out_id);

    // BMS-F-002. Opens a Savings or Current account for an existing
    // customer; the opening balance is the amount deposited at opening.
    // An OPEN journal record is written first, so the journal alone can
    // rebuild every balance (story 6.2 reconciliation). Tellers only; the
    // journal records the teller as the operator.
    Error openAccount(const Session& by, CustomerId customer, AccountType type,
                      Paise opening_deposit, AccountNo& out_no);

    // BMS-F-003. Changes a customer's address and contact number; the name
    // and ID reference are never editable. One CUSTOMER_MODIFY audit entry
    // holding the old and the new value of each changed field is written
    // before the record is updated (BMS-F-063). Values equal to the current
    // ones change nothing and write no audit entry. Tellers only.
    Error modifyCustomer(const Session& by, CustomerId id, const std::string& address,
                         const std::string& contact);

    // BMS-F-004. Closes an account whose balance is exactly zero: one
    // ACCOUNT_CLOSE audit entry naming the manager is written first, then the
    // state becomes CLOSED. The record is kept, never deleted, and every
    // later transaction or login on it is refused. Managers only. A non-zero
    // balance gives BalanceNotZero (moving the residual is story 1.5) and an
    // already closed account gives AccountClosed; neither writes anything.
    Error closeAccount(const Session& by, AccountNo no);

    // BMS-F-005. Unknown keys return a not-found error, never a crash.
    Error findCustomer(CustomerId id, Customer& out);
    Error findAccount(AccountNo no, AccountRecord& out);
    Error listAccounts(CustomerId customer, std::vector<AccountRecord>& out);

private:
    AccountStore& store_;
    ledger::Ledger& journal_;
};

}  // namespace bms
