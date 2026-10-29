#pragma once
// Story 6.1: append-only journal and audit capture (BMS-F-060, BMS-F-063).
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "account/account.h"
#include "core/error.h"
#include "core/money.h"

namespace bms::ledger {

enum class TxnType { Open, Deposit, Withdraw, TransferOut, TransferIn };
enum class AuditAction {
    CustomerCreate, CustomerModify, AccountOpen, AccountClose,
    AccountLock, AccountUnlock, LoginFail, ReportRun
};

std::string_view label(TxnType type);
std::string_view label(AuditAction action);
Error parseTxnType(std::string_view text, TxnType& out);
Error parseAuditAction(std::string_view text, AuditAction& out);

struct JournalRecord {
    std::uint64_t seq = 0;
    std::int64_t unix_ts = 0;
    TxnType type = TxnType::Open;
    AccountNo account = 0;
    Paise amount = 0;
    Paise balance_after = 0;
    std::string operator_id;
};

struct AuditRecord {
    std::uint64_t seq = 0;
    std::int64_t unix_ts = 0;
    std::string operator_id;
    AuditAction action = AuditAction::CustomerCreate;
    std::string target;
    std::string details;
};

// Serialization returns InvalidInput; malformed stored lines return
// CorruptRecord. Lines exclude the final newline, which readLines checks.
Error serialize(const JournalRecord& record, std::string& out);
Error serialize(const AuditRecord& record, std::string& out);
Error parseJournal(std::string_view line, JournalRecord& out);
Error parseAudit(std::string_view line, AuditRecord& out);

// One writer owns each pair of paths (BMS-NF-006). Construction recovers
// independent sequences; recovery errors are returned by the next append.
// There is deliberately no update/delete API (BMS-SR-005).
class Ledger {
public:
    using Clock = std::function<std::int64_t()>;
    Ledger(const std::string& journal_path, const std::string& audit_path,
           Clock clock = {});

    Error appendTransaction(TxnType type, AccountNo account, Paise amount,
                            Paise balance_after, const std::string& operator_id,
                            std::uint64_t& out_seq);
    Error appendAudit(const std::string& operator_id, AuditAction action,
                      const std::string& target, const std::string& details,
                      std::uint64_t& out_seq);
    Error readJournal(std::vector<JournalRecord>& out) const;
    Error readAudit(std::vector<AuditRecord>& out) const;
    // BMS-F-041: up to ten matching records in descending journal sequence.
    // No matches returns Ok with an empty result; errors also clear out.
    Error miniStatement(AccountNo account, std::vector<JournalRecord>& out) const;

private:
    std::string journal_path_;
    std::string audit_path_;
    Clock clock_;
    std::uint64_t journal_seq_ = 1;
    std::uint64_t audit_seq_ = 1;
    Error journal_status_ = Error::Ok;
    Error audit_status_ = Error::Ok;
};

}  // namespace bms::ledger
