
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
