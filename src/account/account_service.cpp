#include "account/account_service.h"

#include "auth/authorization.h"
#include "validation/validation.h"

namespace bms {

Error AccountService::createCustomer(const Session& by, const Customer& details,
                                     CustomerId& out_id) {
    if (const Error e = authorize(by, Operation::CreateCustomer); e != Error::Ok) {
        return e;
    }
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

Error AccountService::openAccount(const Session& by, CustomerId customer,
                                  AccountType type, Paise opening_deposit,
                                  AccountNo& out_no) {
    if (const Error e = authorize(by, Operation::OpenAccount); e != Error::Ok) {
        return e;
    }
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

    // Journal first: if the account write then fails, the journal still
    // shows what was attempted, and the number is never handed out again.
    std::uint64_t seq = 0;
    if (const Error e = journal_.appendTransaction(ledger::TxnType::Open, account.acct_no,
                                                   opening_deposit, opening_deposit,
                                                   by.operator_id, seq);
        e != Error::Ok) {
        return e;
    }
    account.last_seq = seq;

    if (const Error e = store_.createAccount(account); e != Error::Ok) {
        return e;
    }
    out_no = account.acct_no;
    return Error::Ok;
}

Error AccountService::modifyCustomer(const Session& by, CustomerId id,
                                     const std::string& address,
                                     const std::string& contact) {
    if (const Error e = authorize(by, Operation::ModifyCustomer); e != Error::Ok) {
        return e;
    }
    if (validation::validateText(address, validation::kMaxAddress) != Error::Ok ||
        validation::validateContact(contact) != Error::Ok) {
        return Error::InvalidInput;
    }
    Customer customer;
    if (const Error e = store_.readCustomer(id, customer); e != Error::Ok) {
        return e;
    }

    // "field: old -> new" for each field that actually changes.
    std::string details;
    const auto note = [&details](const char* field, const std::string& before,
                                 const std::string& after) {
        if (before != after) {
            details += (details.empty() ? "" : "; ") + std::string(field) + ": " + before +
                       " -> " + after;
        }
    };
    note("address", customer.address, address);
    note("contact", customer.contact, contact);
    if (details.empty()) {
        return Error::Ok;  // nothing to change, so nothing to record
    }

    // Audit first, so a change can never be stored without its record.
    std::uint64_t seq = 0;
    if (const Error e = journal_.appendAudit(by.operator_id,
                                             ledger::AuditAction::CustomerModify,
                                             std::to_string(id), details, seq);
        e != Error::Ok) {
        return e;
    }
    customer.address = address;
    customer.contact = contact;
    return store_.writeCustomer(customer);
}

Error AccountService::findCustomer(CustomerId id, Customer& out) {
    return store_.readCustomer(id, out);
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
