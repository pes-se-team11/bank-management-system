#include "ledger/ledger.h"

#include <array>
#include <charconv>
#include <ctime>
#include <limits>
#include <utility>

#include "persistence/append_file.h"

namespace bms::ledger {

namespace {

std::int64_t systemTime() { return static_cast<std::int64_t>(std::time(nullptr)); }

// BMS-NF-003: decimal integers only, checked for range and trailing bytes.
template <typename Integer>
bool parseInteger(std::string_view text, Integer& out) {
    if (text.empty()) {
        return false;
    }
    const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

template <std::size_t Count>
bool split(std::string_view line, std::array<std::string_view, Count>& fields) {
    for (std::size_t i = 0; i < Count; ++i) {
        const std::size_t separator = line.find('|');
        if (i == Count - 1) {
            if (separator != std::string_view::npos) {
                return false;
            }
            fields[i] = line;
        } else {
            if (separator == std::string_view::npos) {
                return false;
            }
            fields[i] = line.substr(0, separator);
            line.remove_prefix(separator + 1);
        }
    }
    return true;
}

// BMS-SR-005: escaping is a second defence after service input validation.
std::string escape(std::string_view text) {
    std::string out;
    for (const char c : text) {
        switch (c) {
        case '%': out += "%25"; break;
        case '|': out += "%7C"; break;
        case '\n': out += "%0A"; break;
        case '\r': out += "%0D"; break;
        default: out += c; break;
        }
    }
    return out;
}

bool unescape(std::string_view text, std::string& out) {
    std::string decoded;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '\n' || c == '\r' || c == '|') {
            return false;
        }
        if (c != '%') {
            decoded += c;
            continue;
        }
        const std::string_view code = text.substr(i, 3);
        if (code == "%25") {
            decoded += '%';
        } else if (code == "%7C") {
            decoded += '|';
        } else if (code == "%0A") {
            decoded += '\n';
        } else if (code == "%0D") {
            decoded += '\r';
        } else {
            return false;
        }
        i += 2;
    }
    out = std::move(decoded);
    return true;
}

bool valid(const JournalRecord& record) {
    if (record.seq == 0 || record.account == 0 || record.operator_id.empty() ||
        label(record.type).empty()) {
        return false;
    }
    if (record.type == TxnType::Open) {
        return record.amount >= 0 && record.amount == record.balance_after;
    }
    // BMS-F-033: debit legs may leave a Current account in overdraft.
    return record.amount > 0 &&
           (record.balance_after >= 0 || record.type == TxnType::Withdraw ||
            record.type == TxnType::TransferOut);
}

bool valid(const AuditRecord& record) {
    return record.seq != 0 && !record.operator_id.empty() && !label(record.action).empty();
}

template <typename Record, typename Parser>
Error readRecords(const std::string& path, std::vector<Record>& out, Parser parse) {
    out.clear();
    std::vector<std::string> lines;
    if (const Error e = persistence::readLines(path, lines); e != Error::Ok) {
        return e;
    }
    std::vector<Record> records;
    std::uint64_t previous = 0;
    for (const std::string& line : lines) {
        Record record;
        if (parse(line, record) != Error::Ok || record.seq <= previous) {
            return Error::CorruptRecord;
        }
        previous = record.seq;
        records.push_back(std::move(record));
    }
    out = std::move(records);
    return Error::Ok;
}

// Zero is an exhausted sequence, never a stored sequence (BMS-SR-005).
std::uint64_t successor(std::uint64_t seq) {
    return seq == std::numeric_limits<std::uint64_t>::max() ? 0 : seq + 1;
}

}  // namespace

std::string_view label(TxnType type) {
    switch (type) {
    case TxnType::Open: return "OPEN";
    case TxnType::Deposit: return "DEPOSIT";
    case TxnType::Withdraw: return "WITHDRAW";
    case TxnType::TransferOut: return "TRANSFER_OUT";
    case TxnType::TransferIn: return "TRANSFER_IN";
    }
    return {};
}

std::string_view label(AuditAction action) {
    switch (action) {
    case AuditAction::CustomerCreate: return "CUSTOMER_CREATE";
    case AuditAction::CustomerModify: return "CUSTOMER_MODIFY";
    case AuditAction::AccountOpen: return "ACCOUNT_OPEN";
    case AuditAction::AccountClose: return "ACCOUNT_CLOSE";
    case AuditAction::AccountLock: return "ACCOUNT_LOCK";
    case AuditAction::AccountUnlock: return "ACCOUNT_UNLOCK";
    case AuditAction::LoginFail: return "LOGIN_FAIL";
    case AuditAction::ReportRun: return "REPORT_RUN";
    }
    return {};
}

Error parseTxnType(std::string_view text, TxnType& out) {
    constexpr TxnType types[] = {TxnType::Open, TxnType::Deposit, TxnType::Withdraw,
                               TxnType::TransferOut, TxnType::TransferIn};
    for (const TxnType type : types) {
        if (text == label(type)) {
            out = type;
            return Error::Ok;
        }
    }
    return Error::CorruptRecord;
}

Error parseAuditAction(std::string_view text, AuditAction& out) {
    constexpr AuditAction actions[] = {
        AuditAction::CustomerCreate, AuditAction::CustomerModify,
        AuditAction::AccountOpen, AuditAction::AccountClose,
        AuditAction::AccountLock, AuditAction::AccountUnlock,
        AuditAction::LoginFail, AuditAction::ReportRun};
    for (const AuditAction action : actions) {
        if (text == label(action)) {
            out = action;
            return Error::Ok;
        }
    }
    return Error::CorruptRecord;
}

Error serialize(const JournalRecord& record, std::string& out) {
    if (!valid(record)) {
        return Error::InvalidInput;
    }
    out = "J|1|" + std::to_string(record.seq) + "|" + std::to_string(record.unix_ts) +
          "|" + std::string(label(record.type)) + "|" + std::to_string(record.account) +
          "|" + std::to_string(record.amount) + "|" + std::to_string(record.balance_after) +
          "|" + escape(record.operator_id);
    return Error::Ok;
}

Error serialize(const AuditRecord& record, std::string& out) {
    if (!valid(record)) {
        return Error::InvalidInput;
    }
    out = "A|1|" + std::to_string(record.seq) + "|" + std::to_string(record.unix_ts) +
          "|" + escape(record.operator_id) + "|" + std::string(label(record.action)) +
          "|" + escape(record.target) + "|" + escape(record.details);
    return Error::Ok;
}

Error parseJournal(std::string_view line, JournalRecord& out) {
    std::array<std::string_view, 9> fields{};
    JournalRecord record;
    if (!split(line, fields) || fields[0] != "J" || fields[1] != "1" ||
        !parseInteger(fields[2], record.seq) || !parseInteger(fields[3], record.unix_ts) ||
        parseTxnType(fields[4], record.type) != Error::Ok ||
        !parseInteger(fields[5], record.account) || !parseInteger(fields[6], record.amount) ||
        !parseInteger(fields[7], record.balance_after) ||
        !unescape(fields[8], record.operator_id) || !valid(record)) {
        return Error::CorruptRecord;
    }
    out = std::move(record);
    return Error::Ok;
}

Error parseAudit(std::string_view line, AuditRecord& out) {
    std::array<std::string_view, 8> fields{};
    AuditRecord record;
    if (!split(line, fields) || fields[0] != "A" || fields[1] != "1" ||
        !parseInteger(fields[2], record.seq) || !parseInteger(fields[3], record.unix_ts) ||
        !unescape(fields[4], record.operator_id) ||
        parseAuditAction(fields[5], record.action) != Error::Ok ||
        !unescape(fields[6], record.target) || !unescape(fields[7], record.details) ||
        !valid(record)) {
        return Error::CorruptRecord;
    }
    out = std::move(record);
    return Error::Ok;
}

Ledger::Ledger(const std::string& journal_path, const std::string& audit_path, Clock clock)
    : journal_path_(journal_path), audit_path_(audit_path),
      clock_(clock ? std::move(clock) : Clock{systemTime}) {
    std::vector<JournalRecord> journal;
    journal_status_ = readJournal(journal);
    if (journal_status_ == Error::Ok && !journal.empty()) {
        journal_seq_ = successor(journal.back().seq);
    }
    std::vector<AuditRecord> audit;
    audit_status_ = readAudit(audit);
    if (audit_status_ == Error::Ok && !audit.empty()) {
        audit_seq_ = successor(audit.back().seq);
    }
}

Error Ledger::appendTransaction(TxnType type, AccountNo account, Paise amount,
                                Paise balance_after, const std::string& operator_id,
                                std::uint64_t& out_seq) {
    JournalRecord record{1, 0, type, account, amount, balance_after, operator_id};
    // Validate even when the sequence is exhausted, before consulting storage.
    if (!valid(record)) {
        return Error::InvalidInput;
    }
    if (journal_status_ != Error::Ok) {
        return journal_status_;
    }
    if (journal_seq_ == 0) {
        return Error::Io;
    }
    record.seq = journal_seq_;
    record.unix_ts = clock_();
    std::string line;
    if (const Error e = serialize(record, line); e != Error::Ok) {
        return e;
    }
    journal_status_ = persistence::appendLine(journal_path_, line);
    if (journal_status_ != Error::Ok) {
        return journal_status_;  // a failed write may have left a partial line
    }
    out_seq = journal_seq_;
    journal_seq_ = successor(journal_seq_);
    return Error::Ok;
}

Error Ledger::appendAudit(const std::string& operator_id, AuditAction action,
                          const std::string& target, const std::string& details,
                          std::uint64_t& out_seq) {
    AuditRecord record{1, 0, operator_id, action, target, details};
    if (!valid(record)) {
        return Error::InvalidInput;
    }
    if (audit_status_ != Error::Ok) {
        return audit_status_;
    }
    if (audit_seq_ == 0) {
        return Error::Io;
    }
    record.seq = audit_seq_;
    record.unix_ts = clock_();
    std::string line;
    if (const Error e = serialize(record, line); e != Error::Ok) {
        return e;
    }
    audit_status_ = persistence::appendLine(audit_path_, line);
    if (audit_status_ != Error::Ok) {
        return audit_status_;
    }
    out_seq = audit_seq_;
    audit_seq_ = successor(audit_seq_);
    return Error::Ok;
}

Error Ledger::readJournal(std::vector<JournalRecord>& out) const {
    return readRecords(journal_path_, out, parseJournal);
}

Error Ledger::readAudit(std::vector<AuditRecord>& out) const {
    return readRecords(audit_path_, out, parseAudit);
}

}  // namespace bms::ledger
