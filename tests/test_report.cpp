#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "auth/auth.h"
#include "fake_stores.h"
#include "ledger/ledger.h"
#include "report/report_service.h"

using namespace bms;

namespace {

constexpr std::int64_t kIstOffset = 5 * 3600 + 30 * 60;
constexpr std::int64_t kSecondsPerDay = 86400;

constexpr std::int64_t kFixedTime = 1793030400;  // 2026-10-26 21:30 IST

const Session kTeller{"teller1", Role::Teller};
const Session kManager{"manager1", Role::Manager};

struct Files {
    explicit Files(const std::string& tag)
        : journal("test_report_" + tag + "_journal.tmp"),
          audit("test_report_" + tag + "_audit.tmp") {
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

// A movable clock, so entries can be journalled on other days through the
// ordinary append path before the report runs on its own day.
struct Bank {
    explicit Bank(const std::string& tag)
        : files(tag),
          ledger(files.journal, files.audit, [this] { return now; }),
          reports(ledger, [this] { return now; }) {}

    Files files;
    std::int64_t now = kFixedTime;
    ledger::Ledger ledger;
    ReportService reports;
};

void append(Bank& bank, ledger::TxnType type, AccountNo account, Paise amount,
            Paise balance, const char* operator_id = "teller") {
    std::uint64_t seq = 0;
    REQUIRE(bank.ledger.appendTransaction(type, account, amount, balance, operator_id, seq) ==
            Error::Ok);
}

bool contains(const std::vector<ClosingBalance>& balances, AccountNo account, Paise balance) {
    for (const ClosingBalance& line : balances) {
        if (line.account == account) {
            return line.balance == balance;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("journalDay splits timestamps on the IST midnight") {
    // An instant only half a day earlier shares the same IST day index,
    // because the day index moves on UTC+05:30, not on UTC.
    CHECK(journalDay(kFixedTime) == 20752);
    CHECK(journalDay(kFixedTime - kIstOffset) == 20752);
    // The first second of the next IST day (18:30 UTC) is a new day index.
    const std::int64_t boundary =
        static_cast<std::int64_t>(journalDay(kFixedTime)) * kSecondsPerDay - kIstOffset;
    CHECK(journalDay(boundary - 1) + 1 == journalDay(boundary));
}

TEST_CASE("TC-LED-03: totals equal the day's journal entries and close every account") {
    Bank bank("totals");

    // The day before: an opening and a deposit that must be excluded.
    bank.now -= kSecondsPerDay;  // 2026-10-25, the previous IST day
    append(bank, ledger::TxnType::Open, 100001, 50000, 50000);
    append(bank, ledger::TxnType::Deposit, 100001, 25000, 75000);
    bank.now = kFixedTime;       // the report day
    append(bank, ledger::TxnType::Deposit, 100001, 20000, 95000);
    append(bank, ledger::TxnType::Withdraw, 100001, 5000, 90000);
    append(bank, ledger::TxnType::TransferOut, 100001, 3000, 87000);
    append(bank, ledger::TxnType::TransferIn, 100002, 3000, 13000);
    append(bank, ledger::TxnType::Deposit, 100003, 500, 500);

    bank.now += 3600;  // still the same IST day at report time
    EodReport report;
    REQUIRE(bank.reports.endOfDay(kManager, report) == Error::Ok);

    CHECK(report.totals.deposits == 20500);
    CHECK(report.totals.withdrawals == 5000);
    CHECK(report.totals.transfer_out == 3000);
    CHECK(report.totals.transfer_in == 3000);
    // Every account with journal history closes, however old its last entry.
    REQUIRE(report.balances.size() == 3);
    CHECK(report.balances[0].account == 100001);
    CHECK(report.balances[0].balance == 87000);
    CHECK(report.balances[1].account == 100002);
    CHECK(report.balances[1].balance == 13000);
    CHECK(report.balances[2].account == 100003);
    CHECK(report.balances[2].balance == 500);
}

TEST_CASE("TC-LED-03: an empty day totals zero but still closes every account") {
    Bank bank("empty_day");
    append(bank, ledger::TxnType::Open, 100001, 50000, 50000);
    append(bank, ledger::TxnType::Deposit, 100001, 10000, 60000);
    bank.now += kSecondsPerDay;  // tomorrow opens a fresh day

    EodReport report;
    REQUIRE(bank.reports.endOfDay(kManager, report) == Error::Ok);
    CHECK(report.totals.deposits == 0);
    CHECK(report.totals.withdrawals == 0);
    CHECK(report.totals.transfer_out == 0);
    CHECK(report.totals.transfer_in == 0);
    REQUIRE(report.balances.size() == 1);
    CHECK(report.balances[0].account == 100001);
    CHECK(report.balances[0].balance == 60000);

    // A new day's first entry moves to the new totals.
    append(bank, ledger::TxnType::Deposit, 100002, 20, 20);
    EodReport next;
    bank.now += 60;
    REQUIRE(bank.reports.endOfDay(kManager, next) == Error::Ok);
    CHECK(next.totals.deposits == 20);
    CHECK(next.balances.size() == 2);
    CHECK(contains(next.balances, 100002, 20));
}

TEST_CASE("TC-LED-03: an empty journal produces a zero report for the day") {
    Bank bank("fresh");
    EodReport report;
    REQUIRE(bank.reports.endOfDay(kManager, report) == Error::Ok);
    CHECK(report.totals.deposits == 0);
    CHECK(report.totals.withdrawals == 0);
    CHECK(report.totals.transfer_out == 0);
    CHECK(report.totals.transfer_in == 0);
    CHECK(report.balances.empty());
    CHECK(report.day_label == "26-10-2026");
}

TEST_CASE("TC-AUT-03 TC-SEC-08: tellers and customers are refused, managers are not") {
    Bank bank("roles");
    EodReport report;
    CHECK(bank.reports.endOfDay(kTeller, report) == Error::NotAuthorised);
    CHECK(bank.reports.endOfDay(Session{"100001", Role::Customer}, report) ==
          Error::NotAuthorised);
    // A session that never came from a login has no operator and gets nothing.
    CHECK(bank.reports.endOfDay(Session{"", Role::Manager}, report) == Error::NotAuthorised);
    CHECK(report.balances.empty());
    CHECK(report.totals.deposits == 0);

    CHECK(bank.reports.endOfDay(kManager, report) == Error::Ok);
    CHECK(report.journal_day == journalDay(kFixedTime));
}

TEST_CASE("TC-LED-04: each manager report writes exactly one audit record") {
    Bank bank("audit");
    std::vector<ledger::AuditRecord> records;
    CHECK(bank.ledger.readAudit(records) == Error::Ok);
    CHECK(records.empty());

    EodReport report;
    REQUIRE(bank.reports.endOfDay(kManager, report) == Error::Ok);
    REQUIRE(bank.ledger.readAudit(records) == Error::Ok);
    REQUIRE(records.size() == 1);
    CHECK(records[0].operator_id == "manager1");
    CHECK(records[0].action == ledger::AuditAction::ReportRun);
    CHECK(records[0].target == "26-10-2026");

    // Running it again for the same day is another run, with another record.
    REQUIRE(bank.reports.endOfDay(kManager, report) == Error::Ok);
    REQUIRE(bank.ledger.readAudit(records) == Error::Ok);
    CHECK(records.size() == 2);
    CHECK(records[0].seq != records[1].seq);
}

TEST_CASE("TC-LED-03: a refused report writes no audit record") {
    Bank bank("refused_audit");
    append(bank, ledger::TxnType::Open, 100001, 50000, 50000);
    EodReport report;
    CHECK(bank.reports.endOfDay(kTeller, report) == Error::NotAuthorised);
    std::vector<ledger::AuditRecord> records;
    CHECK(bank.ledger.readAudit(records) == Error::Ok);
    CHECK(records.empty());
}

TEST_CASE("TC-LED-03: a corrupt journal is reported and the report is refused") {
    Bank bank("corrupt");
    // The fixture overwrites the file directly; production writes append.
    {
        std::ofstream file(bank.files.journal, std::ios::binary | std::ios::trunc);
        REQUIRE(file.is_open());
        file << "bad\n";
        file.close();
        REQUIRE_FALSE(file.fail());
    }
    EodReport report;
    CHECK(bank.reports.endOfDay(kManager, report) == Error::CorruptRecord);
    CHECK(report.balances.empty());
    std::vector<ledger::AuditRecord> records;
    CHECK(bank.ledger.readAudit(records) == Error::Ok);
    CHECK(records.empty());
}
