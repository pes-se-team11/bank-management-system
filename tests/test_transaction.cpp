
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "account/account.h"
#include "core/error.h"
#include "ledger/ledger.h"
#include "transaction/transaction_service.h"
#include "fake_stores.h"

namespace {

TEST_CASE("Deposit rejects invalid amounts") {
    bms::test::MemoryAccountStore store;

    const std::string journal = "test_deposit_invalid_journal.tmp";
    const std::string audit = "test_deposit_invalid_audit.tmp";

    std::remove(journal.c_str());
    std::remove(audit.c_str());

    bms::ledger::Ledger ledger(journal, audit);
    bms::TransactionService service(store, ledger, 100000);

    CHECK(service.deposit(1, 0, "operator") == bms::Error::InvalidInput);
    CHECK(service.deposit(1, -1, "operator") == bms::Error::InvalidInput);
    CHECK(service.deposit(1, 100001, "operator") == bms::Error::InvalidInput);
    CHECK(service.deposit(1, 100, "") == bms::Error::InvalidInput);

    std::remove(journal.c_str());
    std::remove(audit.c_str());
}

TEST_CASE("Deposit increases an active account balance") {
    bms::test::MemoryAccountStore store;

    bms::AccountRecord account{};
    account.acct_no = 100001;
    account.cust_id = 1;
    account.state = bms::AccountState::Active;
    account.balance = 5000;
    account.opening_balance = 5000;

    REQUIRE(store.createAccount(account) == bms::Error::Ok);

    const std::string journal = "test_deposit_success_journal.tmp";
    const std::string audit = "test_deposit_success_audit.tmp";

    std::remove(journal.c_str());
    std::remove(audit.c_str());

    {
        bms::ledger::Ledger ledger(journal, audit);
        bms::TransactionService service(store, ledger, 100000);

        CHECK(service.deposit(100001, 1500, "test-operator") == bms::Error::Ok);

        bms::AccountRecord updated{};
        REQUIRE(store.readAccount(100001, updated) == bms::Error::Ok);
        CHECK(updated.balance == 6500);
        CHECK(updated.last_seq > 0);

        std::vector<bms::ledger::JournalRecord> entries;
        REQUIRE(ledger.readJournal(entries) == bms::Error::Ok);
        REQUIRE(entries.size() == 1);
        CHECK(entries[0].type == bms::ledger::TxnType::Deposit);
        CHECK(entries[0].account == 100001);
        CHECK(entries[0].amount == 1500);
        CHECK(entries[0].balance_after == 6500);
    }

    std::remove(journal.c_str());
    std::remove(audit.c_str());
}

}  // namespace

TEST_CASE("Withdrawal decreases an active account balance") {
    bms::test::MemoryAccountStore store;

    bms::AccountRecord account{};
    account.acct_no = 200001;
    account.cust_id = 1;
    account.type = bms::AccountType::Savings;
    account.state = bms::AccountState::Active;
    account.balance = 5000;
    account.opening_balance = 5000;

    REQUIRE(store.createAccount(account) == bms::Error::Ok);

    const std::string journal = "test_withdraw_success_journal.tmp";
    const std::string audit = "test_withdraw_success_audit.tmp";
    std::remove(journal.c_str());
    std::remove(audit.c_str());

    {
        bms::ledger::Ledger ledger(journal, audit);
        bms::TransactionService service(store, ledger, 100000);

        CHECK(service.withdraw(200001, 1500, "test-operator") == bms::Error::Ok);

        bms::AccountRecord updated{};
        REQUIRE(store.readAccount(200001, updated) == bms::Error::Ok);
        CHECK(updated.balance == 3500);
        CHECK(updated.last_seq > 0);

        std::vector<bms::ledger::JournalRecord> entries;
        REQUIRE(ledger.readJournal(entries) == bms::Error::Ok);
        REQUIRE(entries.size() == 1);
        CHECK(entries[0].type == bms::ledger::TxnType::Withdraw);
        CHECK(entries[0].account == 200001);
        CHECK(entries[0].amount == 1500);
        CHECK(entries[0].balance_after == 3500);
    }

    std::remove(journal.c_str());
    std::remove(audit.c_str());
}

TEST_CASE("Withdrawal rejects invalid amounts and insufficient funds") {
    bms::test::MemoryAccountStore store;

    bms::AccountRecord account{};
    account.acct_no = 200002;
    account.cust_id = 1;
    account.type = bms::AccountType::Savings;
    account.state = bms::AccountState::Active;
    account.balance = 1000;
    account.opening_balance = 1000;

    REQUIRE(store.createAccount(account) == bms::Error::Ok);

    const std::string journal = "test_withdraw_invalid_journal.tmp";
    const std::string audit = "test_withdraw_invalid_audit.tmp";
    std::remove(journal.c_str());
    std::remove(audit.c_str());

    {
        bms::ledger::Ledger ledger(journal, audit);
        bms::TransactionService service(store, ledger, 100000);

        CHECK(service.withdraw(200002, 0, "operator") == bms::Error::InvalidInput);
        CHECK(service.withdraw(200002, -1, "operator") == bms::Error::InvalidInput);
        CHECK(service.withdraw(200002, 1001, "operator") == bms::Error::InvalidInput);
        CHECK(service.withdraw(200002, 100, "") == bms::Error::InvalidInput);

        bms::AccountRecord unchanged{};
        REQUIRE(store.readAccount(200002, unchanged) == bms::Error::Ok);
        CHECK(unchanged.balance == 1000);

        std::vector<bms::ledger::JournalRecord> entries;
        REQUIRE(ledger.readJournal(entries) == bms::Error::Ok);
        CHECK(entries.empty());
    }

    std::remove(journal.c_str());
    std::remove(audit.c_str());
}

TEST_CASE("Current account withdrawal respects overdraft limit") {
    bms::test::MemoryAccountStore store;

    bms::AccountRecord account{};
    account.acct_no = 200003;
    account.cust_id = 1;
    account.type = bms::AccountType::Current;
    account.state = bms::AccountState::Active;
    account.balance = 1000;
    account.opening_balance = 1000;
    account.overdraft_limit = 500;

    REQUIRE(store.createAccount(account) == bms::Error::Ok);

    const std::string journal = "test_withdraw_overdraft_journal.tmp";
    const std::string audit = "test_withdraw_overdraft_audit.tmp";
    std::remove(journal.c_str());
    std::remove(audit.c_str());

    {
        bms::ledger::Ledger ledger(journal, audit);
        bms::TransactionService service(store, ledger, 100000);

        CHECK(service.withdraw(200003, 1400, "operator") == bms::Error::Ok);
        CHECK(service.withdraw(200003, 200, "operator") == bms::Error::InvalidInput);

        bms::AccountRecord updated{};
        REQUIRE(store.readAccount(200003, updated) == bms::Error::Ok);
        CHECK(updated.balance == -400);
    }

    std::remove(journal.c_str());
    std::remove(audit.c_str());
}
