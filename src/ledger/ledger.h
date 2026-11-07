#pragma once
// Story 6.1: append-only journal and audit capture (BMS-F-060, BMS-F-063).
// Story 6.2: reconciliation of balances against the journal (BMS-F-061).
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

// BMS-F-061: one row per account that fails reconciliation.
struct ReconciliationRow {
    AccountNo account = 0;
    Paise expected = 0;  // opening balance plus its signed journal entries
    Paise stored = 0;    // the balance supplied through StoredBalance
};

// Supplies the stored balance of an account named in the journal; a
// non-Ok result marks that account as failing. The store implementation
// does the reading, so reconciliation stays free of persistence details
// (BMS-NF-006).
using StoredBalance = std::function<Error(AccountNo, Paise&)>;

// BMS-F-061 / TC-LED-02: verify that every account balance equals its
// opening balance plus the sum of its journal entries, so tampering with
// the data files is named rather than absorbed. `journal` is normally
// readJournal's result.
//
// An OPEN record starts an account's history; DEPOSIT and TRANSFER_IN
// credit and WITHDRAW and TRANSFER_OUT debit the running balance, which
// must equal every record's balance_after, ending at the stored balance.
// An account fails - with exactly one row - when it has no OPEN start, an
// OPEN record repeats, the replayed chain diverges from a balance_after,
// the running balance is not representable, or the stored balance is
// missing or different. Unhealthy accounts stop replaying at the break;
// `expected` is the running balance when replay stopped and `stored` is
// zero when no stored balance was reported.
//
// Rows are ordered by account number. The check always reports what does
// not reconcile instead of assuming it, so it returns Ok with an empty
// `out` on clean data and Ok naming the offending accounts otherwise.
// Structural journal errors remain readJournal's business; the records here
// must have come from it.
Error reconcile(const std::vector<JournalRecord>& journal,
                const StoredBalance& stored_balance,
                std::vector<ReconciliationRow>& out);

}  // namespace bms::ledger
