#pragma once
// In-memory stores so service modules can be tested without any files
// (BMS-NF-006). PersistenceModule provides the real, file-backed versions.
#include <map>
#include <string>
#include <vector>

#include "account/account_store.h"
#include "auth/auth.h"

namespace bms::test {

class MemoryAccountStore final : public AccountStore {
public:
    Error nextCustomerId(CustomerId& out) override {
        out = next_customer_++;
        return Error::Ok;
    }
    Error nextAccountNo(AccountNo& out) override {
        out = next_account_++;
        return Error::Ok;
    }
    Error writeCustomer(const Customer& customer) override {
        customers[customer.id] = customer;
        return Error::Ok;
    }
    Error readCustomer(CustomerId id, Customer& out) override {
        const auto it = customers.find(id);
        if (it == customers.end()) {
            return Error::CustomerNotFound;
        }
        out = it->second;
        return Error::Ok;
    }
    Error createAccount(const AccountRecord& account) override {
        accounts[account.acct_no] = account;
        return Error::Ok;
    }
    Error readAccount(AccountNo no, AccountRecord& out) override {
        const auto it = accounts.find(no);
        if (it == accounts.end()) {
            return Error::AccountNotFound;
        }
        out = it->second;
        return Error::Ok;
    }
    Error listAccounts(CustomerId id, std::vector<AccountRecord>& out) override {
        out.clear();
        for (const auto& entry : accounts) {
            if (entry.second.cust_id == id) {
                out.push_back(entry.second);
            }
        }
        return Error::Ok;
    }

    std::map<CustomerId, Customer> customers;
    std::map<AccountNo, AccountRecord> accounts;

private:
    CustomerId next_customer_ = 1;
    AccountNo next_account_ = 100001;
};

class MemoryCredentialStore final : public CredentialStore {
public:
    Error readCredential(const Principal& who, Credential& out) override {
        const auto it = credentials.find(key(who));
        if (it == credentials.end()) {
            return Error::CredentialNotFound;
        }
        out = it->second;
        return Error::Ok;
    }
    Error writeCredential(const Credential& credential) override {
        credentials[key(credential.principal)] = credential;
        return Error::Ok;
    }

    static std::string key(const Principal& who) {
        return (who.kind == PrincipalKind::Customer ? "C:" : "S:") + who.id;
    }

    std::map<std::string, Credential> credentials;
};

}  // namespace bms::test
