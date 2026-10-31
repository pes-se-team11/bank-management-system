#include <doctest/doctest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "account/account_service.h"
#include "auth/auth.h"
#include "auth/authorization.h"
#include "fake_stores.h"
#include "ledger/ledger.h"

using namespace bms;

namespace {

constexpr std::uint32_t kTestIterations = 1000;

const Session kCustomer{"100001", Role::Customer};
const Session kTeller{"teller1", Role::Teller};
const Session kManager{"manager1", Role::Manager};

Customer asha() {
    Customer c;
    c.name = "Asha Rao";
    c.address = "12 MG Road, Bengaluru";
    c.contact = "9876543210";
    c.gov_id_ref = "ABCDE1234F";
    return c;
}

// Both services over in-memory stores and a ledger that starts empty.
// Members are built in this order, so the files are cleared before the
// ledger reads them.
struct Bank {
    struct Files {
        explicit Files(const std::string& tag)
            : journal("test_authz_" + tag + "_journal.tmp"),
              audit("test_authz_" + tag + "_audit.tmp") {
            clear();
        }
        ~Files() { clear(); }
        void clear() const {
            std::remove(journal.c_str());
            std::remove(audit.c_str());
        }
        std::string journal;
        std::string audit;
    };

    explicit Bank(const std::string& tag)
        : files(tag),
          ledger(files.journal, files.audit),
          accounts(account_store, ledger),
          auth(credential_store, account_store, ledger, kTestIterations) {}

    Files files;
    test::MemoryAccountStore account_store;
    test::MemoryCredentialStore credential_store;
    ledger::Ledger ledger;
    AccountService accounts;
    AuthService auth;
};

}  // namespace

TEST_CASE("TC-AUT-03: a Teller is refused the end-of-day report; a Manager succeeds") {
    CHECK(authorize(kTeller, Operation::EndOfDayReport) == Error::NotAuthorised);
    CHECK(authorize(kManager, Operation::EndOfDayReport) == Error::Ok);
    CHECK(authorize(kCustomer, Operation::EndOfDayReport) == Error::NotAuthorised);
}

TEST_CASE("TC-AUT-03: each operation belongs to exactly one role") {
    const Operation teller_ops[] = {Operation::CreateCustomer, Operation::ModifyCustomer,
                                    Operation::OpenAccount};
    const Operation manager_ops[] = {Operation::CloseAccount, Operation::UnlockAccount,
                                     Operation::AddStaff, Operation::EndOfDayReport,
                                     Operation::ViewAuditLog};
    for (const Operation op : teller_ops) {
        CHECK(authorize(kTeller, op) == Error::Ok);
        CHECK(authorize(kManager, op) == Error::NotAuthorised);
        CHECK(authorize(kCustomer, op) == Error::NotAuthorised);
    }
    for (const Operation op : manager_ops) {
        CHECK(authorize(kManager, op) == Error::Ok);
        CHECK(authorize(kTeller, op) == Error::NotAuthorised);
        CHECK(authorize(kCustomer, op) == Error::NotAuthorised);
    }

    // A session that never came from a login has no operator and gets nothing.
    CHECK(authorize(Session{"", Role::Manager}, Operation::EndOfDayReport) ==
          Error::NotAuthorised);
    CHECK(authorize(Session{"", Role::Teller}, Operation::OpenAccount) ==
          Error::NotAuthorised);
}

TEST_CASE("TC-SEC-08: account services refuse the wrong role with no menu involved") {
    Bank bank("account");

    CustomerId id = 0;
    CHECK(bank.accounts.createCustomer(kManager, asha(), id) == Error::NotAuthorised);
    CHECK(bank.accounts.createCustomer(kCustomer, asha(), id) == Error::NotAuthorised);
    CHECK(bank.account_store.customers.empty());

    REQUIRE(bank.accounts.createCustomer(kTeller, asha(), id) == Error::Ok);
    AccountNo no = 0;
    CHECK(bank.accounts.openAccount(kManager, id, AccountType::Savings, 1000, no) ==
          Error::NotAuthorised);
    CHECK(bank.accounts.openAccount(kCustomer, id, AccountType::Savings, 1000, no) ==
          Error::NotAuthorised);
    CHECK(bank.account_store.accounts.empty());

    // A refused opening leaves nothing in the journal.
    std::vector<ledger::JournalRecord> entries;
    REQUIRE(bank.ledger.readJournal(entries) == Error::Ok);
    CHECK(entries.empty());

    // The same call from a Teller works and records the teller as operator.
    REQUIRE(bank.accounts.openAccount(kTeller, id, AccountType::Savings, 1000, no) ==
            Error::Ok);
    REQUIRE(bank.ledger.readJournal(entries) == Error::Ok);
    REQUIRE(entries.size() == 1);
    CHECK(entries[0].operator_id == "teller1");
}

TEST_CASE("TC-SEC-08: only a Manager creates staff and only a Teller sets a PIN") {
    Bank bank("enrol");

    std::string password = "teller-pass1";
    CHECK(bank.auth.enrol(kTeller, Principal{PrincipalKind::Staff, "T002"}, Role::Teller,
                          password) == Error::NotAuthorised);
    CHECK(password.empty());  // wiped even when refused
    password = "teller-pass1";
    CHECK(bank.auth.enrol(kCustomer, Principal{PrincipalKind::Staff, "T002"}, Role::Teller,
                          password) == Error::NotAuthorised);

    std::string pin = "4821";
    CHECK(bank.auth.enrol(kManager, Principal{PrincipalKind::Customer, "100001"},
                          Role::Customer, pin) == Error::NotAuthorised);
    CHECK(pin.empty());
    CHECK(bank.credential_store.credentials.empty());

    password = "teller-pass1";
    CHECK(bank.auth.enrol(kManager, Principal{PrincipalKind::Staff, "T002"}, Role::Teller,
                          password) == Error::Ok);
    pin = "4821";
    CHECK(bank.auth.enrol(kTeller, Principal{PrincipalKind::Customer, "100001"},
                          Role::Customer, pin) == Error::Ok);
    CHECK(bank.credential_store.credentials.size() == 2);
}

TEST_CASE("TC-SEC-08: the first manager can be created only on a fresh install") {
    Bank bank("first");

    std::string password = "manager-pass1";
    REQUIRE(bank.auth.enrolFirstManager("admin", password) == Error::Ok);
    CHECK(password.empty());

    Session session;
    std::string attempt = "manager-pass1";
    REQUIRE(bank.auth.login(Principal{PrincipalKind::Staff, "admin"}, attempt, session) ==
            Error::Ok);
    CHECK(session.role == Role::Manager);

    // Once any staff login exists, this path can no longer mint a manager.
    std::string second = "intruder-pass1";
    CHECK(bank.auth.enrolFirstManager("intruder", second) == Error::NotAuthorised);
    CHECK(second.empty());
    Credential stored;
    CHECK(bank.credential_store.readCredential(Principal{PrincipalKind::Staff, "intruder"},
                                               stored) == Error::CredentialNotFound);
}
