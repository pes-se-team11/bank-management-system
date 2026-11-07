#pragma once
// ReportModule (Person 3): story 6.3, the end-of-day report (BMS-F-062).
// It reads the day's journal entries and totals them by type, plus the
// closing balance of every account the journal knows. Manager-only, checked
// here rather than in the menu (BMS-F-012, BMS-SR-007).
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "auth/auth.h"
#include "core/error.h"
#include "ledger/ledger.h"

namespace bms {

// Totals for one journal type over the report day, in paise. Transfer legs
// stay separate; a net figure would hide a mismatch between the two legs.
struct EodTotals {
    Paise deposits = 0;
    Paise withdrawals = 0;
    Paise transfer_out = 0;
    Paise transfer_in = 0;
};

// One line of the closing-balance list: the last balance_after the journal
// holds for that account, whatever its state.
struct ClosingBalance {
    AccountNo account = 0;
    Paise balance = 0;
};

struct EodReport {
    std::int64_t journal_day = 0;  // day index of the report day (see JournalDay)
    std::string day_label;         // "07-11-2026", the report day in IST
    EodTotals totals;
    std::vector<ClosingBalance> balances;  // ascending by account number
};

// The IST (UTC+05:30) calendar-day index of a journal timestamp, matching
// the timestamps the CLI shows. Pure arithmetic on the seconds.
std::int64_t journalDay(std::int64_t unix_ts);

class ReportService {
public:
    using Clock = std::function<std::int64_t()>;
    ReportService(ledger::Ledger& journal, Clock clock = {});

    // BMS-F-062. Computes the report, then records exactly one REPORT_RUN
    // audit entry for the manager who ran it (BMS-F-063); an audit failure
    // refuses the report, so it can never run without its record.
    // Tellers and customers are refused (BMS-F-012).
    Error endOfDay(const Session& by, EodReport& out);

private:
    ledger::Ledger& journal_;
    Clock clock_;
};

}  // namespace bms
