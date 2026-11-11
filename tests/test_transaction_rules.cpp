// Part 3 tests for the money rules added in Sprint 3:
//   story 3.3 (BMS-F-032), 3.2 daily limit (BMS-F-030),
//   story 5.1 (BMS-F-050, BMS-F-051), story 5.2 (BMS-F-052),
//   plus the deposit cases TC-DEP-02 and TC-DEP-03.
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "account/account.h"
#include "core/error.h"
#include "ledger/ledger.h"
#include "transaction/transaction_service.h"
#include "fake_stores.h"

namespace {

using bms::AccountRecord;
using bms::AccountState;
using bms::AccountType;
using bms::Error;
using bms::Paise;

constexpr std::uint32_t kDay1 = 20261107;
constexpr std::uint32_t kDay2 = 20261108;

// Wraps the in-memory store so a test can make the Nth updateAccount fail,
// which is how "failure between the two legs" is injected (TC-TRF-01).
class FlakyStore final : public bms::AccountStore {
public:
    Error nextCustomerId(bms::CustomerId& out) override { return inner.nextCustomerId(out); }
    Error nextAccountNo(bms::AccountNo& out) override { return inner.nextAccountNo(out); }
    Error writeCustomer(const bms::Customer& c) override { return inner.writeCustomer(c); }
    Error readCustomer(bms::CustomerId id, bms::Customer& out) override { return inner.readCustomer(id, out); }
    Error createAccount(const AccountRecord& a) override { return inner.createAccount(a); }
    Error readAccount(bms::AccountNo no, AccountRecord& out) override { return inner.readAccount(no, out); }
    Error listAccounts(bms::CustomerId id, std::vector<AccountRecord>& out) override { return inner.listAccounts(id, out); }
    Error updateAccount(const AccountRecord& a) override {
        ++update_calls;
        if (fail_on_update != 0 && update_calls == fail_on_update) {
            return Error::Io;
        }
        return inner.updateAccount(a);
    }

    bms::test::MemoryAccountStore inner;
    int update_calls = 0;
    int fail_on_update = 0;  // 1-based; 0 means never fail
};

AccountRecord makeAccount(bms::AccountNo no, AccountType type, Paise balance,
                          AccountState state = AccountState::Active) {
    AccountRecord a{};
    a.acct_no = no;
    a.cust_id = 1;
    a.type = type;
    a.state = state;
    a.balance = balance;
    a.opening_balance = balance;
    a.counter_date = kDay1;
    return a;
}

struct Rig {
    explicit Rig(const std::string& tag, bms::TransactionLimits limits = {})
        : journal("test_rules_" + tag + "_journal.tmp"),
          audit("test_rules_" + tag + "_audit.tmp") {
        std::remove(journal.c_str());
        std::remove(audit.c_str());
        ledger = std::make_unique<bms::ledger::Ledger>(journal, audit);
        service = std::make_unique<bms::TransactionService>(
            store, *ledger, 1000000, limits, [this] { return today; });
    }
    ~Rig() {
        service.reset();
        ledger.reset();
        std::remove(journal.c_str());
        std::remove(audit.c_str());
    }
    Paise balanceOf(bms::AccountNo no) {
        AccountRecord a{};
        REQUIRE(store.readAccount(no, a) == Error::Ok);
        return a.balance;
    }
    std::vector<bms::ledger::JournalRecord> entries() {
        std::vector<bms::ledger::JournalRecord> out;
        REQUIRE(ledger->readJournal(out) == Error::Ok);
        return out;
    }
    // BMS-F-061: every stored balance must equal its journal replay.
    bool reconciles() {
        const auto all = entries();
        std::vector<bms::ledger::ReconciliationRow> rows;
        const bms::ledger::StoredBalance stored =
            [this](bms::AccountNo no, Paise& out) {
                AccountRecord a{};
                const Error e = store.readAccount(no, a);
                out = a.balance;
                return e;
            };
        REQUIRE(bms::ledger::reconcile(all, stored, rows) == Error::Ok);
        return rows.empty();
    }

    std::string journal, audit;
    FlakyStore store;
    std::uint32_t today = kDay1;
    std::unique_ptr<bms::ledger::Ledger> ledger;
    std::unique_ptr<bms::TransactionService> service;
};

// An OPEN record per account so reconciliation has a starting point.
void journalOpen(Rig& rig, bms::AccountNo no, Paise balance) {
    std::uint64_t seq = 0;
    REQUIRE(rig.ledger->appendTransaction(bms::ledger::TxnType::Open, no,
                                          balance, balance, "setup", seq) ==
            Error::Ok);
}

}  // namespace

// ---------------------------------------------------------------- deposit

TEST_CASE("TC-DEP-02: each of the four deposit rejections leaves the balance alone") {
    Rig rig("dep02");
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 5000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings, 5000,
                                        AccountState::Closed));

    CHECK(rig.service->deposit(100001, 0, "op") == Error::InvalidInput);        // zero
    CHECK(rig.service->deposit(100001, -5, "op") == Error::InvalidInput);       // negative
    CHECK(rig.service->deposit(100001, 1000001, "op") == Error::InvalidInput);  // over ceiling
    CHECK(rig.service->deposit(100002, 100, "op") == Error::InvalidInput);      // not ACTIVE

    CHECK(rig.balanceOf(100001) == 5000);
    CHECK(rig.balanceOf(100002) == 5000);
    CHECK(rig.entries().empty());
}

TEST_CASE("TC-DEP-03: exactly one journal record per successful deposit") {
    Rig rig("dep03");
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 5000));

    REQUIRE(rig.service->deposit(100001, 700, "teller1") == Error::Ok);
    REQUIRE(rig.service->deposit(100001, 300, "teller1") == Error::Ok);
    CHECK(rig.service->deposit(100001, 0, "teller1") == Error::InvalidInput);

    const auto e = rig.entries();
    REQUIRE(e.size() == 2);
    CHECK(e[0].type == bms::ledger::TxnType::Deposit);
    CHECK(e[0].account == 100001);
    CHECK(e[0].amount == 700);
    CHECK(e[0].balance_after == 5700);
    CHECK(e[0].operator_id == "teller1");
    CHECK(e[1].balance_after == 6000);
}

// ------------------------------------------------------------- withdrawal

TEST_CASE("TC-WDR-01: a withdrawal that passes today's limit is refused") {
    bms::TransactionLimits limits;
    limits.daily_withdrawal_limit = 10000;
    Rig rig("wdr01", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 50000));

    CHECK(rig.service->withdraw(100001, 6000, "op") == Error::Ok);
    // Balance is plenty, but 6000 + 5000 would pass the 10000 daily limit.
    CHECK(rig.service->withdraw(100001, 5000, "op") == Error::DailyLimitExceeded);
    CHECK(rig.balanceOf(100001) == 44000);
    // Exactly reaching the limit is allowed.
    CHECK(rig.service->withdraw(100001, 4000, "op") == Error::Ok);
    CHECK(rig.service->withdraw(100001, 1, "op") == Error::DailyLimitExceeded);
    CHECK(rig.entries().size() == 2);
}

TEST_CASE("TC-WDR-01: the daily withdrawal total restarts on a new bank day") {
    bms::TransactionLimits limits;
    limits.daily_withdrawal_limit = 10000;
    Rig rig("wdr01b", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 50000));

    REQUIRE(rig.service->withdraw(100001, 10000, "op") == Error::Ok);
    CHECK(rig.service->withdraw(100001, 1, "op") == Error::DailyLimitExceeded);
    rig.today = kDay2;
    CHECK(rig.service->withdraw(100001, 10000, "op") == Error::Ok);
    CHECK(rig.balanceOf(100001) == 30000);
}

TEST_CASE("TC-WDR-01: a withdrawal over the balance is refused and changes nothing") {
    Rig rig("wdr01c");
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 1000));
    CHECK(rig.service->withdraw(100001, 1001, "op") == Error::InvalidInput);
    CHECK(rig.balanceOf(100001) == 1000);
    CHECK(rig.entries().empty());
}

TEST_CASE("TC-WDR-02: Savings minimum balance - below is refused, exactly the minimum succeeds") {
    bms::TransactionLimits limits;
    limits.savings_minimum_balance = 50000;
    Rig rig("wdr02", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 80000));

    CHECK(rig.service->withdraw(100001, 30001, "op") == Error::BelowMinimumBalance);
    CHECK(rig.balanceOf(100001) == 80000);
    CHECK(rig.service->withdraw(100001, 30000, "op") == Error::Ok);   // leaves exactly 50000
    CHECK(rig.balanceOf(100001) == 50000);
    CHECK(rig.service->withdraw(100001, 1, "op") == Error::BelowMinimumBalance);
    // Asking for more than the whole balance is plain insufficient funds.
    CHECK(rig.service->withdraw(100001, 90000, "op") == Error::InvalidInput);
}

TEST_CASE("TC-WDR-02: the minimum balance applies to Savings only") {
    bms::TransactionLimits limits;
    limits.savings_minimum_balance = 50000;
    Rig rig("wdr02b", limits);
    rig.store.createAccount(makeAccount(200001, AccountType::Current, 10000));
    CHECK(rig.service->withdraw(200001, 10000, "op") == Error::Ok);
    CHECK(rig.balanceOf(200001) == 0);
}

TEST_CASE("TC-WDR-03: Current overdraft still works with the new limits in place") {
    bms::TransactionLimits limits;
    limits.savings_minimum_balance = 50000;
    Rig rig("wdr03", limits);
    AccountRecord cur = makeAccount(200001, AccountType::Current, 1000);
    cur.overdraft_limit = 5000;
    rig.store.createAccount(cur);
    CHECK(rig.service->withdraw(200001, 6000, "op") == Error::Ok);  // to -5000
    CHECK(rig.balanceOf(200001) == -5000);
    CHECK(rig.service->withdraw(200001, 1, "op") == Error::InvalidInput);
}

// --------------------------------------------------------------- transfer

TEST_CASE("TC-TRF-01: a transfer moves the money and journals both legs") {
    Rig rig("trf01");
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 10000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings, 2000));
    journalOpen(rig, 100001, 10000);
    journalOpen(rig, 100002, 2000);

    REQUIRE(rig.service->transfer(100001, 100002, 2500, "teller1") == Error::Ok);
    CHECK(rig.balanceOf(100001) == 7500);
    CHECK(rig.balanceOf(100002) == 4500);

    const auto e = rig.entries();
    REQUIRE(e.size() == 4);
    CHECK(e[2].type == bms::ledger::TxnType::TransferOut);
    CHECK(e[2].account == 100001);
    CHECK(e[2].balance_after == 7500);
    CHECK(e[3].type == bms::ledger::TxnType::TransferIn);
    CHECK(e[3].account == 100002);
    CHECK(e[3].balance_after == 4500);
    CHECK(rig.reconciles());
}

TEST_CASE("TC-TRF-01: failure between the two legs leaves both balances as they were") {
    for (int failing_update : {1, 2}) {
        CAPTURE(failing_update);
        Rig rig("trf01f" + std::to_string(failing_update));
        rig.store.inner.createAccount(makeAccount(100001, AccountType::Savings, 10000));
        rig.store.inner.createAccount(makeAccount(100002, AccountType::Savings, 2000));
        journalOpen(rig, 100001, 10000);
        journalOpen(rig, 100002, 2000);

        rig.store.fail_on_update = failing_update;  // 1 = debit write, 2 = credit write
        CHECK(rig.service->transfer(100001, 100002, 2500, "op") == Error::Io);

        CHECK(rig.balanceOf(100001) == 10000);
        CHECK(rig.balanceOf(100002) == 2000);
        // The journal still explains every balance: reversals were appended.
        CHECK(rig.reconciles());

        // And the system is usable afterwards.
        rig.store.fail_on_update = 0;
        CHECK(rig.service->transfer(100001, 100002, 2500, "op") == Error::Ok);
        CHECK(rig.balanceOf(100001) == 7500);
        CHECK(rig.balanceOf(100002) == 4500);
        CHECK(rig.reconciles());
    }
}

TEST_CASE("TC-TRF-02: self-transfer and transfer to a closed account get distinct refusals") {
    Rig rig("trf02");
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 10000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings, 10000,
                                        AccountState::Closed));
    rig.store.createAccount(makeAccount(100003, AccountType::Savings, 10000,
                                        AccountState::Locked));

    CHECK(rig.service->transfer(100001, 100001, 100, "op") == Error::SameAccount);
    CHECK(rig.service->transfer(100001, 100002, 100, "op") == Error::AccountClosed);
    CHECK(rig.service->transfer(100002, 100001, 100, "op") == Error::AccountClosed);
    CHECK(rig.service->transfer(100001, 100003, 100, "op") == Error::AccountLocked);
    CHECK(rig.service->transfer(100001, 999999, 100, "op") == Error::AccountNotFound);
    CHECK(rig.service->transfer(999999, 100001, 100, "op") == Error::AccountNotFound);
    CHECK(rig.service->transfer(100001, 100003, 0, "op") == Error::InvalidInput);
    CHECK(rig.service->transfer(100001, 100003, 100, "") == Error::InvalidInput);

    CHECK(rig.balanceOf(100001) == 10000);
    CHECK(rig.balanceOf(100002) == 10000);
    CHECK(rig.entries().empty());  // refused before any balance or journal write
}

TEST_CASE("TC-TRF-02: a transfer cannot overdraw the source or overflow the target") {
    bms::TransactionLimits limits;
    limits.savings_minimum_balance = 500;
    Rig rig("trf02b", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 1000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings,
                                        std::numeric_limits<Paise>::max() - 10));

    CHECK(rig.service->transfer(100001, 100002, 501, "op") == Error::BelowMinimumBalance);
    CHECK(rig.service->transfer(100001, 100002, 2000, "op") == Error::InvalidInput);
    CHECK(rig.service->transfer(100001, 100002, 500, "op") == Error::InvalidInput);  // target overflow
    CHECK(rig.balanceOf(100001) == 1000);
}

TEST_CASE("TC-TRF-03: per-transaction ceiling") {
    bms::TransactionLimits limits;
    limits.transfer_per_txn_ceiling = 5000;
    Rig rig("trf03a", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 100000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings, 0));

    CHECK(rig.service->transfer(100001, 100002, 5001, "op") == Error::TransferCeilingExceeded);
    CHECK(rig.service->transfer(100001, 100002, 5000, "op") == Error::Ok);
    CHECK(rig.balanceOf(100002) == 5000);
}

TEST_CASE("TC-TRF-03: daily ceiling is refused even when each transfer is within the per-transaction limit") {
    bms::TransactionLimits limits;
    limits.transfer_per_txn_ceiling = 5000;
    limits.transfer_daily_ceiling = 12000;
    Rig rig("trf03b", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 100000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings, 0));

    CHECK(rig.service->transfer(100001, 100002, 5000, "op") == Error::Ok);
    CHECK(rig.service->transfer(100001, 100002, 5000, "op") == Error::Ok);
    // Running total is 10000; another 5000 is within the per-transfer limit
    // but crosses the 12000 daily ceiling.
    CHECK(rig.service->transfer(100001, 100002, 5000, "op") == Error::DailyLimitExceeded);
    CHECK(rig.service->transfer(100001, 100002, 2000, "op") == Error::Ok);  // exactly 12000
    CHECK(rig.balanceOf(100001) == 88000);

    // Next bank day the total starts again.
    rig.today = kDay2;
    CHECK(rig.service->transfer(100001, 100002, 5000, "op") == Error::Ok);
}

TEST_CASE("TC-TRF-03: transfers and withdrawals keep separate daily totals") {
    bms::TransactionLimits limits;
    limits.daily_withdrawal_limit = 3000;
    limits.transfer_daily_ceiling = 3000;
    Rig rig("trf03c", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 100000));
    rig.store.createAccount(makeAccount(100002, AccountType::Savings, 0));

    CHECK(rig.service->withdraw(100001, 3000, "op") == Error::Ok);
    CHECK(rig.service->transfer(100001, 100002, 3000, "op") == Error::Ok);
    CHECK(rig.service->withdraw(100001, 1, "op") == Error::DailyLimitExceeded);
    CHECK(rig.service->transfer(100001, 100002, 1, "op") == Error::DailyLimitExceeded);
}

// ------------------------------------------------- error and edge branches
// These close the gaps the Part-3 branch-coverage run found in
// transaction_service.cpp (store read errors, journal failures, overflow).

TEST_CASE("Deposit and withdrawal on an unknown account return AccountNotFound") {
    Rig rig("edge_unknown");
    CHECK(rig.service->deposit(424242, 100, "op") == Error::AccountNotFound);
    CHECK(rig.service->withdraw(424242, 100, "op") == Error::AccountNotFound);
    CHECK(rig.entries().empty());
}

TEST_CASE("Deposit that would overflow the balance is refused") {
    Rig rig("edge_overflow");
    rig.store.createAccount(makeAccount(
        100001, AccountType::Savings, std::numeric_limits<Paise>::max() - 50));
    CHECK(rig.service->deposit(100001, 100, "op") == Error::InvalidInput);
    CHECK(rig.balanceOf(100001) == std::numeric_limits<Paise>::max() - 50);
}

TEST_CASE("TC-WDR-02: a Savings account already under the minimum cannot withdraw") {
    bms::TransactionLimits limits;
    limits.savings_minimum_balance = 50000;
    Rig rig("edge_under_min", limits);
    rig.store.createAccount(makeAccount(100001, AccountType::Savings, 30000));
    CHECK(rig.service->withdraw(100001, 1, "op") == Error::BelowMinimumBalance);
    CHECK(rig.balanceOf(100001) == 30000);
}

TEST_CASE("A huge Current balance with an overdraft does not overflow the check") {
    Rig rig("edge_big_current");
    AccountRecord cur = makeAccount(
        200001, AccountType::Current, std::numeric_limits<Paise>::max() - 5);
    cur.overdraft_limit = 1000;
    rig.store.createAccount(cur);
    CHECK(rig.service->withdraw(200001, 500, "op") == Error::Ok);
    CHECK(rig.balanceOf(200001) == std::numeric_limits<Paise>::max() - 505);
}

TEST_CASE("A stored negative overdraft limit is refused rather than trusted") {
    Rig rig("edge_neg_od");
    AccountRecord cur = makeAccount(200001, AccountType::Current, 5000);
    cur.overdraft_limit = -1;
    rig.store.createAccount(cur);
    rig.store.createAccount(makeAccount(200002, AccountType::Current, 0));
    CHECK(rig.service->withdraw(200001, 100, "op") == Error::InvalidInput);
    CHECK(rig.service->transfer(200001, 200002, 100, "op") == Error::InvalidInput);
    CHECK(rig.balanceOf(200001) == 5000);
}

TEST_CASE("TC-REL-01 (partial): a journal that cannot be written stops the money moving") {
    // The journal sits in a folder that does not exist, so every append fails.
    const std::string bad_journal = "no_such_dir_for_test/journal.tmp";
    const std::string bad_audit = "no_such_dir_for_test/audit.tmp";
    FlakyStore store;
    store.inner.createAccount(makeAccount(100001, AccountType::Savings, 10000));
    store.inner.createAccount(makeAccount(100002, AccountType::Savings, 10000));
    bms::ledger::Ledger ledger(bad_journal, bad_audit);
    bms::TransactionService service(store, ledger, 1000000);

    CHECK(service.deposit(100001, 500, "op") != Error::Ok);
    CHECK(service.withdraw(100001, 500, "op") != Error::Ok);
    CHECK(service.transfer(100001, 100002, 500, "op") != Error::Ok);

    AccountRecord a{};
    REQUIRE(store.readAccount(100001, a) == Error::Ok);
    CHECK(a.balance == 10000);  // journal-first: no journal, no balance change
    REQUIRE(store.readAccount(100002, a) == Error::Ok);
    CHECK(a.balance == 10000);
}

TEST_CASE("A deposit whose balance write fails reports the error") {
    Rig rig("edge_deposit_io");
    rig.store.inner.createAccount(makeAccount(100001, AccountType::Savings, 1000));
    rig.store.fail_on_update = 1;
    CHECK(rig.service->deposit(100001, 100, "op") == Error::Io);
    rig.store.fail_on_update = 0;
    rig.store.update_calls = 0;
    // The journal is ahead of the stored balance - exactly what
    // reconciliation (BMS-F-061) exists to catch.
    CHECK_FALSE(rig.reconciles());
}

TEST_CASE("The bank date is a plausible YYYYMMDD") {
    const std::uint32_t today = bms::bankDateToday();
    CHECK(today > 20250101);
    CHECK(today < 22000101);
    CHECK(today / 100 % 100 >= 1);
    CHECK(today / 100 % 100 <= 12);
    CHECK(today % 100 >= 1);
    CHECK(today % 100 <= 31);
}
