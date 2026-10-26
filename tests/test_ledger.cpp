#include <doctest/doctest.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "ledger/ledger.h"
#include "persistence/append_file.h"

using namespace bms;
using namespace bms::ledger;

namespace {

// Each test owns a directory under the platform's temporary directory.
struct TempFiles {
    std::filesystem::path directory;
    std::string journal;
    std::string audit;

    TempFiles() {
        std::error_code error;
        const auto root = std::filesystem::temp_directory_path(error);
        REQUIRE_FALSE(error);
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        bool created = false;
        for (unsigned int attempt = 0; attempt < 100; ++attempt) {
            directory = root / ("bms-ledger-" + std::to_string(nonce) + "-" +
                                std::to_string(attempt));
            created = std::filesystem::create_directory(directory, error);
            REQUIRE_FALSE(error);
            if (created) {
                break;
            }
        }
        REQUIRE(created);
        journal = (directory / "journal.log").string();
        audit = (directory / "audit.log").string();
    }

    ~TempFiles() {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }
};

std::int64_t fixedTime() { return 1793030400; }

std::string bytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.is_open());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Only corruption fixtures overwrite files; production writes always append.
void writeFixture(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    REQUIRE(file.is_open());
    file << text;
    file.close();
    REQUIRE_FALSE(file.fail());
}

void checkRecord(const JournalRecord& actual, const JournalRecord& expected) {
    CHECK(actual.seq == expected.seq);
    CHECK(actual.unix_ts == expected.unix_ts);
    CHECK(actual.type == expected.type);
    CHECK(actual.account == expected.account);
    CHECK(actual.amount == expected.amount);
    CHECK(actual.balance_after == expected.balance_after);
    CHECK(actual.operator_id == expected.operator_id);
}

void checkRecord(const AuditRecord& actual, const AuditRecord& expected) {
    CHECK(actual.seq == expected.seq);
    CHECK(actual.unix_ts == expected.unix_ts);
    CHECK(actual.operator_id == expected.operator_id);
    CHECK(actual.action == expected.action);
    CHECK(actual.target == expected.target);
    CHECK(actual.details == expected.details);
}

}  // namespace

TEST_CASE("TC-LED-01 TC-SEC-06: journal and audit appends preserve every earlier byte") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit, fixedTime);
    std::uint64_t seq = 0;
    REQUIRE(ledger.appendTransaction(TxnType::Open, 42, 100, 100, "teller", seq) == Error::Ok);
    REQUIRE(ledger.appendAudit("teller", AuditAction::AccountOpen, "42", "opened", seq) ==
            Error::Ok);
    const std::string old_journal = bytes(files.journal);
    const std::string old_audit = bytes(files.audit);
    REQUIRE(ledger.appendTransaction(TxnType::Deposit, 42, 50, 150, "teller", seq) == Error::Ok);
    REQUIRE(ledger.appendAudit("manager", AuditAction::ReportRun, "daily", "done", seq) ==
            Error::Ok);
    const std::string new_journal = bytes(files.journal);
    const std::string new_audit = bytes(files.audit);
    CHECK(new_journal.size() > old_journal.size());
    CHECK(new_journal.substr(0, old_journal.size()) == old_journal);
    CHECK(new_audit.size() > old_audit.size());
    CHECK(new_audit.substr(0, old_audit.size()) == old_audit);
}

TEST_CASE("TC-LED-01: independent journal and audit sequences continue after restart") {
    TempFiles files;
    std::uint64_t seq = 0;
    {
        Ledger ledger(files.journal, files.audit, fixedTime);
        REQUIRE(ledger.appendTransaction(TxnType::Open, 42, 0, 0, "teller", seq) == Error::Ok);
        CHECK(seq == 1);
        REQUIRE(ledger.appendTransaction(TxnType::Deposit, 42, 100, 100, "teller", seq) ==
                Error::Ok);
        CHECK(seq == 2);
        REQUIRE(ledger.appendAudit("manager", AuditAction::ReportRun, "daily", "", seq) ==
                Error::Ok);
        CHECK(seq == 1);
    }
    Ledger restarted(files.journal, files.audit, fixedTime);
    REQUIRE(restarted.appendTransaction(TxnType::Withdraw, 42, 10, 90, "teller", seq) ==
            Error::Ok);
    CHECK(seq == 3);
    REQUIRE(restarted.appendAudit("manager", AuditAction::ReportRun, "daily", "", seq) ==
            Error::Ok);
    CHECK(seq == 2);
}

TEST_CASE("TC-LED-01: every journal type round trips including OPEN and both transfer legs") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit, fixedTime);
    const std::vector<JournalRecord> expected = {
        {1, fixedTime(), TxnType::Open, 42, 100, 100, "teller"},
        {2, fixedTime(), TxnType::Deposit, 42, 50, 150, "teller"},
        {3, fixedTime(), TxnType::Withdraw, 42, 200, -50, "teller"},
        {4, fixedTime(), TxnType::TransferOut, 42, 10, -60, "teller"},
        {5, fixedTime(), TxnType::TransferIn, 43, 10, 10, "teller"}};
    for (const auto& record : expected) {
        CAPTURE(label(record.type));
        std::string line;
        REQUIRE(serialize(record, line) == Error::Ok);
        JournalRecord parsed;
        REQUIRE(parseJournal(line, parsed) == Error::Ok);
        checkRecord(parsed, record);
        std::uint64_t seq = 0;
        REQUIRE(ledger.appendTransaction(record.type, record.account, record.amount,
                                        record.balance_after, record.operator_id, seq) == Error::Ok);
        CHECK(seq == record.seq);
    }
    std::vector<JournalRecord> stored;
    REQUIRE(ledger.readJournal(stored) == Error::Ok);
    REQUIRE(stored.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        checkRecord(stored[i], expected[i]);
    }
    CHECK(stored[4].seq == stored[3].seq + 1);
}

TEST_CASE("TC-LED-01: invalid transactions leave files and output sequence unchanged") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit, fixedTime);
    const std::vector<JournalRecord> invalid = {
        {1, 0, TxnType::Deposit, 42, 0, 10, "teller"},
        {1, 0, TxnType::Deposit, 42, -1, 10, "teller"},
        {1, 0, TxnType::Withdraw, 42, 0, -10, "teller"},
        {1, 0, TxnType::TransferOut, 42, -1, -10, "teller"},
        {1, 0, TxnType::TransferIn, 42, 0, 10, "teller"},
        {1, 0, TxnType::Deposit, 0, 1, 10, "teller"},
        {1, 0, TxnType::Deposit, 42, 1, 10, ""},
        {1, 0, TxnType::Open, 42, -1, -1, "teller"},
        {1, 0, TxnType::Open, 42, 1, 2, "teller"},
        {1, 0, TxnType::Deposit, 42, 1, -1, "teller"},
        {1, 0, TxnType::TransferIn, 42, 1, -1, "teller"},
        {1, 0, static_cast<TxnType>(99), 42, 1, 10, "teller"}};
    for (const auto& record : invalid) {
        std::uint64_t seq = 99;
        CHECK(ledger.appendTransaction(record.type, record.account, record.amount,
                                       record.balance_after, record.operator_id, seq) ==
              Error::InvalidInput);
        CHECK(seq == 99);
        std::string line = "unchanged";
        CHECK(serialize(record, line) == Error::InvalidInput);
        CHECK(line == "unchanged");
    }
    CHECK_FALSE(std::filesystem::exists(files.journal));
    std::uint64_t seq = 99;
    REQUIRE(ledger.appendTransaction(TxnType::Open, 42, 0, 0, "teller", seq) == Error::Ok);
    CHECK(seq == 1);
}

TEST_CASE("TC-LED-04: privileged actions capture operator action target details and timestamp") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit, fixedTime);
    const AuditAction actions[] = {
        AuditAction::CustomerCreate, AuditAction::CustomerModify, AuditAction::AccountOpen,
        AuditAction::AccountClose, AuditAction::AccountLock, AuditAction::AccountUnlock,
        AuditAction::LoginFail, AuditAction::ReportRun};
    std::uint64_t expected_seq = 1;
    for (const AuditAction action : actions) {
        AuditRecord expected{expected_seq, fixedTime(), "manager", action, "42", "old -> new"};
        std::string line;
        REQUIRE(serialize(expected, line) == Error::Ok);
        AuditRecord parsed;
        REQUIRE(parseAudit(line, parsed) == Error::Ok);
        checkRecord(parsed, expected);
        std::uint64_t seq = 0;
        REQUIRE(ledger.appendAudit(expected.operator_id, action, expected.target,
                                  expected.details, seq) == Error::Ok);
        CHECK(seq == expected_seq);
        ++expected_seq;
    }
    std::vector<AuditRecord> stored;
    REQUIRE(ledger.readAudit(stored) == Error::Ok);
    REQUIRE(stored.size() == 8);
    for (std::size_t i = 0; i < stored.size(); ++i) {
        checkRecord(stored[i], AuditRecord{static_cast<std::uint64_t>(i) + 1, fixedTime(),
                                          "manager", actions[i], "42", "old -> new"});
    }
}

TEST_CASE("TC-LED-04: audit refuses empty operators and unknown actions") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit, fixedTime);
    std::uint64_t seq = 99;
    CHECK(ledger.appendAudit("", AuditAction::AccountClose, "42", "", seq) == Error::InvalidInput);
    CHECK(ledger.appendAudit("manager", static_cast<AuditAction>(99), "42", "", seq) ==
          Error::InvalidInput);
    CHECK(seq == 99);
    CHECK_FALSE(std::filesystem::exists(files.audit));
    REQUIRE(ledger.appendAudit("manager", AuditAction::ReportRun, "", "", seq) == Error::Ok);
    CHECK(seq == 1);
}

TEST_CASE("TC-SEC-06: reserved free text characters escape and round trip exactly") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit, fixedTime);
    const std::string text = "op|%\n\r%7C";
    std::string line;
    const JournalRecord journal{1, fixedTime(), TxnType::Open, 42, 0, 0, text};
    REQUIRE(serialize(journal, line) == Error::Ok);
    CHECK(line == "J|1|1|1793030400|OPEN|42|0|0|op%7C%25%0A%0D%257C");
    JournalRecord parsed_journal;
    REQUIRE(parseJournal(line, parsed_journal) == Error::Ok);
    checkRecord(parsed_journal, journal);
    const AuditRecord audit{1, fixedTime(), text, AuditAction::CustomerModify, text, text};
    REQUIRE(serialize(audit, line) == Error::Ok);
    CHECK(line == "A|1|1|1793030400|op%7C%25%0A%0D%257C|CUSTOMER_MODIFY|"
                  "op%7C%25%0A%0D%257C|op%7C%25%0A%0D%257C");
    AuditRecord parsed_audit;
    REQUIRE(parseAudit(line, parsed_audit) == Error::Ok);
    checkRecord(parsed_audit, audit);
    std::uint64_t seq = 0;
    REQUIRE(ledger.appendTransaction(TxnType::Open, 42, 0, 0, text, seq) == Error::Ok);
    REQUIRE(ledger.appendAudit(text, audit.action, text, text, seq) == Error::Ok);
    std::vector<JournalRecord> journals;
    std::vector<AuditRecord> audits;
    REQUIRE(ledger.readJournal(journals) == Error::Ok);
    REQUIRE(ledger.readAudit(audits) == Error::Ok);
    REQUIRE(journals.size() == 1);
    REQUIRE(audits.size() == 1);
    checkRecord(journals.front(), journal);
    checkRecord(audits.front(), audit);
}

TEST_CASE("TC-SEC-06: strict journal parsing refuses malformed fields without changing output") {
    const std::vector<std::string> invalid = {
        "", "J|1|1", "X|1|1|0|OPEN|42|0|0|op", "J|2|1|0|OPEN|42|0|0|op",
        "J|1|1|0|OPEN|42|0|0|op|extra", "J|1|x|0|OPEN|42|0|0|op",
        "J|1|-1|0|OPEN|42|0|0|op", "J|1|0|0|OPEN|42|0|0|op",
        "J|1|18446744073709551616|0|OPEN|42|0|0|op",
        "J|1|1|9223372036854775808|OPEN|42|0|0|op",
        "J|1|1|-9223372036854775809|OPEN|42|0|0|op",
        "J|1|1|0x|OPEN|42|0|0|op", "J|1|1| 0|OPEN|42|0|0|op",
        "J|1|1|+0|OPEN|42|0|0|op", "J|1|1|0|UNKNOWN|42|0|0|op",
        "J|1|1|0|OPEN|4294967296|0|0|op", "J|1|1|0|OPEN|-1|0|0|op",
        "J|1|1|0|OPEN|0|0|0|op", "J|1|1|0|DEPOSIT|42|1x|1|op",
        "J|1|1|0|DEPOSIT|42|9223372036854775808|1|op",
        "J|1|1|0|WITHDRAW|42|1|-9223372036854775809|op",
        "J|1|1|0|OPEN|42|0|1|op", "J|1|1|0|DEPOSIT|42|0|0|op",
        "J|1|1|0|OPEN|42|0|0|", "J|1|1|0|OPEN|42|0|0|op%",
        "J|1|1|0|OPEN|42|0|0|op%7", "J|1|1|0|OPEN|42|0|0|op%GG",
        "J|1|1|0|OPEN|42|0|0|op%20", "J|1|1|0|OPEN|42|0|0|op%7c",
        "J|1|1|0|OPEN|42|0|0|op\n", "J|1|1|0|OPEN|42|0|0|op\r"};
    JournalRecord out;
    out.operator_id = "unchanged";
    for (const auto& line : invalid) {
        CAPTURE(line);
        CHECK(parseJournal(line, out) == Error::CorruptRecord);
        CHECK(out.operator_id == "unchanged");
    }
}

TEST_CASE("TC-SEC-06: strict audit parsing refuses malformed fields without changing output") {
    const std::vector<std::string> invalid = {
        "", "A|1|1", "X|1|1|0|op|REPORT_RUN|42|done",
        "A|2|1|0|op|REPORT_RUN|42|done", "A|1|1|0|op|REPORT_RUN|42|done|extra",
        "A|1|0|0|op|REPORT_RUN|42|done", "A|1|-1|0|op|REPORT_RUN|42|done",
        "A|1|18446744073709551616|0|op|REPORT_RUN|42|done",
        "A|1|1|9223372036854775808|op|REPORT_RUN|42|done",
        "A|1|1|0x|op|REPORT_RUN|42|done", "A|1|1|0||REPORT_RUN|42|done",
        "A|1|1|0|op|UNKNOWN|42|done", "A|1|1|0|op%|REPORT_RUN|42|done",
        "A|1|1|0|op|REPORT_RUN|42%7|done", "A|1|1|0|op|REPORT_RUN|42|done%ZZ",
        "A|1|1|0|op|REPORT_RUN|42|done\n", "A|1|1|0|op|REPORT_RUN|42|done\r"};
    AuditRecord out;
    out.operator_id = "unchanged";
    for (const auto& line : invalid) {
        CAPTURE(line);
        CHECK(parseAudit(line, out) == Error::CorruptRecord);
        CHECK(out.operator_id == "unchanged");
    }
}

TEST_CASE("TC-SEC-06: corrupt or unterminated files refuse reads and further appends") {
    TempFiles files;
    const std::vector<std::string> bad_journals = {
        "bad\n", "J|1|1|0|OPEN|42|0|0|op", "J|1|1|0|OPEN|42|0|0|op\nJ|1|2",
        "J|1|1|0|OPEN|42|0|0|op\n\n",
        "J|1|1|0|OPEN|42|0|0|op\nJ|1|1|0|OPEN|42|0|0|op\n",
        "J|1|2|0|OPEN|42|0|0|op\nJ|1|1|0|OPEN|42|0|0|op\n"};
    for (const auto& content : bad_journals) {
        writeFixture(files.journal, content);
        Ledger ledger(files.journal, files.audit, fixedTime);
        std::vector<JournalRecord> records(1);
        CHECK(ledger.readJournal(records) == Error::CorruptRecord);
        CHECK(records.empty());
        std::uint64_t seq = 99;
        CHECK(ledger.appendTransaction(TxnType::Deposit, 42, 1, 1, "op", seq) ==
              Error::CorruptRecord);
        CHECK(seq == 99);
        CHECK(bytes(files.journal) == content);
    }
    const std::vector<std::string> bad_audits = {
        "bad\n", "A|1|1|0|op|REPORT_RUN|42|done",
        "A|1|1|0|op|REPORT_RUN|42|done\nA|1|2",
        "A|1|1|0|op|REPORT_RUN|42|done\nA|1|1|0|op|REPORT_RUN|42|done\n"};
    for (const auto& content : bad_audits) {
        writeFixture(files.audit, content);
        Ledger ledger(files.journal, files.audit, fixedTime);
        std::vector<AuditRecord> records(1);
        CHECK(ledger.readAudit(records) == Error::CorruptRecord);
        CHECK(records.empty());
        std::uint64_t seq = 99;
        CHECK(ledger.appendAudit("op", AuditAction::ReportRun, "42", "done", seq) ==
              Error::CorruptRecord);
        CHECK(seq == 99);
        CHECK(bytes(files.audit) == content);
    }
}

TEST_CASE("TC-LED-01: missing files are empty and an unwritable path reports Io") {
    TempFiles files;
    Ledger empty(files.journal, files.audit, fixedTime);
    std::vector<JournalRecord> journals(1);
    std::vector<AuditRecord> audits(1);
    std::vector<std::string> lines{"stale"};
    CHECK(empty.readJournal(journals) == Error::Ok);
    CHECK(journals.empty());
    CHECK(empty.readAudit(audits) == Error::Ok);
    CHECK(audits.empty());
    CHECK(persistence::readLines(files.journal, lines) == Error::Ok);
    CHECK(lines.empty());
    const std::string missing_parent = (files.directory / "absent" / "journal").string();
    Ledger unwritable(missing_parent, files.audit, fixedTime);
    std::uint64_t seq = 99;
    CHECK(unwritable.appendTransaction(TxnType::Open, 42, 0, 0, "op", seq) == Error::Io);
    CHECK(seq == 99);
    // An append failure is latched: retry cannot follow a possibly partial write.
    CHECK(unwritable.appendTransaction(TxnType::Open, 42, 0, 0, "op", seq) == Error::Io);
    CHECK(persistence::readLines(files.directory.string(), lines) == Error::Io);
    Ledger bad_audit(files.journal, files.directory.string(), fixedTime);
    CHECK(bad_audit.appendAudit("op", AuditAction::ReportRun, "", "", seq) == Error::Io);
    CHECK(seq == 99);
}

TEST_CASE("TC-SEC-03: integer limits round trip and exhausted sequences never wrap") {
    TempFiles files;
    const auto max_seq = std::numeric_limits<std::uint64_t>::max();
    const auto max_amount = std::numeric_limits<Paise>::max();
    const auto min_amount = std::numeric_limits<Paise>::min();
    const JournalRecord original{max_seq, std::numeric_limits<std::int64_t>::min(),
                                 TxnType::Withdraw, std::numeric_limits<AccountNo>::max(),
                                 max_amount, min_amount, "op"};
    std::string line;
    REQUIRE(serialize(original, line) == Error::Ok);
    JournalRecord parsed;
    REQUIRE(parseJournal(line, parsed) == Error::Ok);
    checkRecord(parsed, original);
    REQUIRE(persistence::appendLine(files.journal, line) == Error::Ok);
    const AuditRecord audit{max_seq, std::numeric_limits<std::int64_t>::max(), "op",
                            AuditAction::ReportRun, "", ""};
    REQUIRE(serialize(audit, line) == Error::Ok);
    AuditRecord parsed_audit;
    REQUIRE(parseAudit(line, parsed_audit) == Error::Ok);
    checkRecord(parsed_audit, audit);
    REQUIRE(persistence::appendLine(files.audit, line) == Error::Ok);
    Ledger exhausted(files.journal, files.audit, fixedTime);
    std::uint64_t seq = 99;
    const auto journal_before = bytes(files.journal);
    const auto audit_before = bytes(files.audit);
    CHECK(exhausted.appendTransaction(TxnType::Deposit, 42, 1, 1, "op", seq) == Error::Io);
    CHECK(exhausted.appendAudit("op", AuditAction::ReportRun, "", "", seq) == Error::Io);
    CHECK(seq == 99);
    CHECK(bytes(files.journal) == journal_before);
    CHECK(bytes(files.audit) == audit_before);
}

TEST_CASE("TC-LED-04: the default clock records current Unix time") {
    TempFiles files;
    Ledger ledger(files.journal, files.audit);
    const auto before = static_cast<std::int64_t>(std::time(nullptr));
    std::uint64_t seq = 0;
    REQUIRE(ledger.appendAudit("manager", AuditAction::AccountUnlock, "42", "unlocked", seq) ==
            Error::Ok);
    const auto after = static_cast<std::int64_t>(std::time(nullptr));
    std::vector<AuditRecord> records;
    REQUIRE(ledger.readAudit(records) == Error::Ok);
    REQUIRE(records.size() == 1);
    CHECK(records.front().unix_ts >= before);
    CHECK(records.front().unix_ts <= after);
}
