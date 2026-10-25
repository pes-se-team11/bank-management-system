#include <doctest/doctest.h>

#include <vector>

#include "account/account_service.h"
#include "fake_stores.h"

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

}  // namespace

TEST_CASE("TC-ACC-01: a new customer gets an ID never used before") {
    test::MemoryAccountStore store;
    AccountService accounts(store);

    CustomerId first = 0;
    CustomerId second = 0;
    REQUIRE(accounts.createCustomer(asha(), first) == Error::Ok);
    REQUIRE(accounts.createCustomer(asha(), second) == Error::Ok);
    CHECK(first != second);
    CHECK(second > first);  // IDs only ever grow, so none is reused

    Customer stored;
    REQUIRE(store.readCustomer(first, stored) == Error::Ok);
    CHECK(stored.name == "Asha Rao");
    CHECK(stored.gov_id_ref == "ABCDE1234F");
}

TEST_CASE("TC-ACC-01: invalid details are refused and use up no ID") {
    test::MemoryAccountStore store;
    AccountService accounts(store);

    Customer bad = asha();
    bad.contact = "123";
    CustomerId id = 0;
    CHECK(accounts.createCustomer(bad, id) == Error::InvalidInput);
    CHECK(store.customers.empty());

    CustomerId good = 0;
    REQUIRE(accounts.createCustomer(asha(), good) == Error::Ok);
    CHECK(good == 1U);  // the failed attempt did not consume ID 1
}

TEST_CASE("TC-ACC-02: an account opens ACTIVE with the deposit as its balance") {
    test::MemoryAccountStore store;
    AccountService accounts(store);
    CustomerId cust = 0;
    REQUIRE(accounts.createCustomer(asha(), cust) == Error::Ok);

    AccountNo savings = 0;
    AccountNo current = 0;
    REQUIRE(accounts.openAccount(cust, AccountType::Savings, 500000, savings) == Error::Ok);
    REQUIRE(accounts.openAccount(cust, AccountType::Current, 0, current) == Error::Ok);
    CHECK(savings != current);

    AccountRecord rec;
    REQUIRE(accounts.findAccount(savings, rec) == Error::Ok);
    CHECK(rec.cust_id == cust);
    CHECK(rec.type == AccountType::Savings);
    CHECK(rec.state == AccountState::Active);
    CHECK(rec.opening_balance == 500000);
    CHECK(rec.balance == 500000);
}

TEST_CASE("TC-ACC-02: opening is refused for an unknown customer or a negative deposit") {
    test::MemoryAccountStore store;
    AccountService accounts(store);
    AccountNo no = 0;
    CHECK(accounts.openAccount(42, AccountType::Savings, 1000, no) ==
          Error::CustomerNotFound);

    CustomerId cust = 0;
    REQUIRE(accounts.createCustomer(asha(), cust) == Error::Ok);
    CHECK(accounts.openAccount(cust, AccountType::Savings, -1, no) == Error::InvalidInput);
    CHECK(store.accounts.empty());
}

TEST_CASE("TC-ACC-05: look up by account number and list by customer") {
    test::MemoryAccountStore store;
    AccountService accounts(store);
    CustomerId a = 0;
    CustomerId b = 0;
    REQUIRE(accounts.createCustomer(asha(), a) == Error::Ok);
    REQUIRE(accounts.createCustomer(asha(), b) == Error::Ok);
    AccountNo a1 = 0;
    AccountNo a2 = 0;
    AccountNo b1 = 0;
    REQUIRE(accounts.openAccount(a, AccountType::Savings, 100, a1) == Error::Ok);
    REQUIRE(accounts.openAccount(a, AccountType::Current, 200, a2) == Error::Ok);
    REQUIRE(accounts.openAccount(b, AccountType::Savings, 300, b1) == Error::Ok);

    std::vector<AccountRecord> list;
    REQUIRE(accounts.listAccounts(a, list) == Error::Ok);
    REQUIRE(list.size() == 2);
    for (const AccountRecord& r : list) {
        CHECK(r.cust_id == a);
    }

    // Unknown keys give a not-found result, never a crash.
    AccountRecord rec;
    CHECK(accounts.findAccount(999999, rec) == Error::AccountNotFound);
    CHECK(accounts.listAccounts(77, list) == Error::CustomerNotFound);
}
