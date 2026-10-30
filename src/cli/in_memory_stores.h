#pragma once
// Temporary storage for the CLI until file storage lands (Vidit's story).
// Everything here is lost when the program exits. To switch to files,
// replace these two objects in main.cpp with the file-backed stores - the
// menu and the services only see the AccountStore / CredentialStore
// interfaces, so nothing else changes.
#include <map>
#include <string>
#include <vector>

#include "account/account_store.h"
#include "auth/auth.h"

namespace bms::cli {

class InMemoryAccountStore final : public AccountStore {
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
        customers_[customer.id] = customer;
        return Error::Ok;
    }
    Error readCustomer(CustomerId id, Customer& out) override {
        const auto it = customers_.find(id);
        if (it == customers_.end()) {
            return Error::CustomerNotFound;
        }
        out = it->second;
        return Error::Ok;
    }
    Error createAccount(const AccountRecord& account) override {
        accounts_[account.acct_no] = account;
        return Error::Ok;
    }
    Error readAccount(AccountNo no, AccountRecord& out) override {
        const auto it = accounts_.find(no);
        if (it == accounts_.end()) {
            return Error::AccountNotFound;
        }
        out = it->second;
        return Error::Ok;
    }
    Error updateAccount(const AccountRecord& account) override {
        const auto it = accounts_.find(account.acct_no);
        if (it == accounts_.end()) {
            return Error::AccountNotFound;
        }
        it->second = account;
        return Error::Ok;
    }
    Error listAccounts(CustomerId id, std::vector<AccountRecord>& out) override {
        out.clear();
        for (const auto& entry : accounts_) {
            if (entry.second.cust_id == id) {
                out.push_back(entry.second);
            }
        }
        return Error::Ok;
    }

private:
    CustomerId next_customer_ = 1;
    AccountNo next_account_ = 100001;
    std::map<CustomerId, Customer> customers_;
    std::map<AccountNo, AccountRecord> accounts_;
};

class InMemoryCredentialStore final : public CredentialStore {
public:
    Error readCredential(const Principal& who, Credential& out) override {
        const auto it = credentials_.find(key(who));
        if (it == credentials_.end()) {
            return Error::CredentialNotFound;
        }
        out = it->second;
        return Error::Ok;
    }
    Error writeCredential(const Credential& credential) override {
        credentials_[key(credential.principal)] = credential;
        return Error::Ok;
    }

    // True once any staff login exists; the menu asks for a first manager
    // when there is none.
    bool hasStaff() const {
        for (const auto& entry : credentials_) {
            if (entry.second.principal.kind == PrincipalKind::Staff) {
                return true;
            }
        }
        return false;
    }
    Error anyStaff(bool& out) override {
        out = hasStaff();
        return Error::Ok;
    }

private:
    static std::string key(const Principal& who) {
        return (who.kind == PrincipalKind::Customer ? "C:" : "S:") + who.id;
    }

    std::map<std::string, Credential> credentials_;
};

}  // namespace bms::cli
