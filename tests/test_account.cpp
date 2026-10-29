#include <doctest/doctest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "account/account_service.h"
#include "fake_stores.h"
#include "ledger/ledger.h"

using namespace bms;

namespace {

Customer asha() {
    Customer c;
    c.name = "Asha Rao";
    c.address = "12 MG Road, Bengaluru";
    c.contact = "9876543210";
    c.gov_id_ref = "ABCDE1234F";
    return c;
}

// Journal files that start empty and are removed afterwards.
struct TempJournal {
    explicit TempJournal(const std::string& tag)
        : journal("test_account_" + tag + "_journal.tmp"),
          audit("test_account_" + tag + "_audit.tmp") {
        clear();
    }
    ~TempJournal() { clear(); }
    void clear() const {
        std::remove(journal.c_str());
        std::remove(audit.c_str());
    }
    std::string journal;
    std::string audit;
};

// Members are built in this order, so the files are cleared before the
// ledger reads them.
struct Bank {
    explicit Bank(const std::string& tag)
        : files(tag), ledger(files.journal, files.audit), accounts(store, ledger) {}
    TempJournal files;
    test::MemoryAccountStore store;
    ledger::Ledger ledger;
    AccountService accounts;
};

}  // namespace

TEST_CASE("TC-ACC-01: a new customer gets an ID never used before") {
    Bank bank("ids");

    CustomerId first = 0;
    CustomerId second = 0;
    REQUIRE(bank.accounts.createCustomer(asha(), first) == Error::Ok);
    REQUIRE(bank.accounts.createCustomer(asha(), second) == Error::Ok);
    CHECK(first != second);
    CHECK(second > first);  // IDs only ever grow, so none is reused

    Customer stored;
    REQUIRE(bank.store.readCustomer(first, stored) == Error::Ok);
    CHECK(stored.name == "Asha Rao");
    CHECK(stored.gov_id_ref == "ABCDE1234F");
}

TEST_CASE("TC-ACC-01: invalid details are refused and use up no ID") {
    Bank bank("invalid");

    Customer bad = asha();
    bad.contact = "123";
    CustomerId id = 0;
    CHECK(bank.accounts.createCustomer(bad, id) == Error::InvalidInput);
    CHECK(bank.store.customers.empty());

    CustomerId good = 0;
    REQUIRE(bank.accounts.createCustomer(asha(), good) == Error::Ok);
    CHECK(good == 1U);  // the failed attempt did not consume ID 1
}

TEST_CASE("TC-ACC-02: an account opens ACTIVE with the deposit as its balance") {
    Bank bank("open");
    CustomerId cust = 0;
    REQUIRE(bank.accounts.createCustomer(asha(), cust) == Error::Ok);

    AccountNo savings = 0;
    AccountNo current = 0;
    REQUIRE(bank.accounts.openAccount(cust, AccountType::Savings, 500000, "teller1",
                                      savings) == Error::Ok);
    REQUIRE(bank.accounts.openAccount(cust, AccountType::Current, 0, "teller1",
                                      current) == Error::Ok);
    CHECK(savings != current);

    AccountRecord rec;
    REQUIRE(bank.accounts.findAccount(savings, rec) == Error::Ok);
    CHECK(rec.cust_id == cust);
    CHECK(rec.type == AccountType::Savings);
    CHECK(rec.state == AccountState::Active);
    CHECK(rec.opening_balance == 500000);
    CHECK(rec.balance == 500000);
}

TEST_CASE("TC-ACC-02: opening writes an OPEN journal record with the opening balance") {
    Bank bank("journal");
    CustomerId cust = 0;
    REQUIRE(bank.accounts.createCustomer(asha(), cust) == Error::Ok);

    AccountNo no = 0;
    REQUIRE(bank.accounts.openAccount(cust, AccountType::Savings, 50000, "teller1", no) ==
            Error::Ok);

    std::vector<ledger::JournalRecord> entries;
    REQUIRE(bank.ledger.readJournal(entries) == Error::Ok);
    REQUIRE(entries.size() == 1);
    CHECK(entries[0].type == ledger::TxnType::Open);
    CHECK(entries[0].account == no);
    CHECK(entries[0].amount == 50000);
    CHECK(entries[0].balance_after == 50000);
    CHECK(entries[0].operator_id == "teller1");

    AccountRecord rec;
    REQUIRE(bank.accounts.findAccount(no, rec) == Error::Ok);
    CHECK(rec.last_seq == entries[0].seq);
}

TEST_CASE("TC-ACC-02: opening is refused for an unknown customer or a negative deposit") {
    Bank bank("refused");
    AccountNo no = 0;
    CHECK(bank.accounts.openAccount(42, AccountType::Savings, 1000, "teller1", no) ==
          Error::CustomerNotFound);

    CustomerId cust = 0;
    REQUIRE(bank.accounts.createCustomer(asha(), cust) == Error::Ok);
    CHECK(bank.accounts.openAccount(cust, AccountType::Savings, -1, "teller1", no) ==
          Error::InvalidInput);
    CHECK(bank.accounts.openAccount(cust, AccountType::Savings, 1000, "", no) ==
          Error::InvalidInput);
    CHECK(bank.store.accounts.empty());

    // Refused openings leave nothing in the journal.
    std::vector<ledger::JournalRecord> entries;
    REQUIRE(bank.ledger.readJournal(entries) == Error::Ok);
    CHECK(entries.empty());
}

TEST_CASE("TC-ACC-05: look up by account number and list by customer") {
    Bank bank("lookup");
    CustomerId a = 0;
    CustomerId b = 0;
    REQUIRE(bank.accounts.createCustomer(asha(), a) == Error::Ok);
    REQUIRE(bank.accounts.createCustomer(asha(), b) == Error::Ok);
    AccountNo a1 = 0;
    AccountNo a2 = 0;
    AccountNo b1 = 0;
    REQUIRE(bank.accounts.openAccount(a, AccountType::Savings, 100, "teller1", a1) ==
            Error::Ok);
    REQUIRE(bank.accounts.openAccount(a, AccountType::Current, 200, "teller1", a2) ==
            Error::Ok);
    REQUIRE(bank.accounts.openAccount(b, AccountType::Savings, 300, "teller1", b1) ==
            Error::Ok);

    std::vector<AccountRecord> list;
    REQUIRE(bank.accounts.listAccounts(a, list) == Error::Ok);
    REQUIRE(list.size() == 2);
    for (const AccountRecord& r : list) {
        CHECK(r.cust_id == a);
    }

    // Unknown keys give a not-found result, never a crash.
    AccountRecord rec;
    CHECK(bank.accounts.findAccount(999999, rec) == Error::AccountNotFound);
    CHECK(bank.accounts.listAccounts(77, list) == Error::CustomerNotFound);
}
