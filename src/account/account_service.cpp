#include "account/account_service.h"

#include "validation/validation.h"

namespace bms {

Error AccountService::createCustomer(const Customer& details, CustomerId& out_id) {
    using namespace validation;
    if (validateText(details.name, kMaxName) != Error::Ok ||
        validateText(details.address, kMaxAddress) != Error::Ok ||
        validateContact(details.contact) != Error::Ok ||
        validateGovIdRef(details.gov_id_ref) != Error::Ok) {
        return Error::InvalidInput;
    }

    Customer customer = details;
    if (const Error e = store_.nextCustomerId(customer.id); e != Error::Ok) {
        return e;
    }
    if (const Error e = store_.writeCustomer(customer); e != Error::Ok) {
        return e;
    }
    out_id = customer.id;
    return Error::Ok;
}

Error AccountService::openAccount(CustomerId customer, AccountType type,
                                  Paise opening_deposit, AccountNo& out_no) {
    if (opening_deposit < 0) {
        return Error::InvalidInput;
    }
    Customer holder;
    if (const Error e = store_.readCustomer(customer, holder); e != Error::Ok) {
        return e;  // CustomerNotFound: an account always belongs to a real customer
    }

    AccountRecord account;
    if (const Error e = store_.nextAccountNo(account.acct_no); e != Error::Ok) {
        return e;
    }
    account.cust_id = customer;
    account.type = type;
    account.state = AccountState::Active;
    account.opening_balance = opening_deposit;
    account.balance = opening_deposit;

    // When the journal lands (story 7.2) this also appends an OPEN record
    // holding the full account, so a damaged record can be rebuilt from the
    // journal alone (agreed in the review of PR #3).
    if (const Error e = store_.createAccount(account); e != Error::Ok) {
        return e;
    }
    out_no = account.acct_no;
    return Error::Ok;
}

Error AccountService::findAccount(AccountNo no, AccountRecord& out) {
    return store_.readAccount(no, out);
}

Error AccountService::listAccounts(CustomerId customer, std::vector<AccountRecord>& out) {
    Customer holder;
    if (const Error e = store_.readCustomer(customer, holder); e != Error::Ok) {
        return e;  // distinguishes "no such customer" from "customer with no accounts"
    }
    return store_.listAccounts(customer, out);
}

}  // namespace bms
