#include "report/report_service.h"

#include <ctime>
#include <map>

#include "auth/authorization.h"

namespace bms {

namespace {

constexpr std::int64_t kIstOffset = 5 * 3600 + 30 * 60;
constexpr std::int64_t kSecondsPerDay = 86400;

std::int64_t systemTime() { return static_cast<std::int64_t>(std::time(nullptr)); }

// Days since 1970-01-01 to a calendar date (H. Hinnant's civil_from_days),
// the same arithmetic the CLI's formatTimestamp uses, so the day label on
// the report matches the timestamps printed under it.
std::string dayLabel(std::int64_t days) {
    const std::int64_t z = days + 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const std::int64_t doe = z - era * 146097;
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    const std::int64_t day = doy - (153 * mp + 2) / 5 + 1;
    const std::int64_t month = mp < 10 ? mp + 3 : mp - 9;
    const std::int64_t year = yoe + era * 400 + (month <= 2 ? 1 : 0);

    const auto two = [](std::int64_t value) {
        return (value < 10 ? "0" : "") + std::to_string(value);
    };
    return two(day) + '-' + two(month) + '-' + std::to_string(year);
}

}  // namespace

std::int64_t journalDay(std::int64_t unix_ts) {
    const std::int64_t local = unix_ts + kIstOffset;
    return local >= 0 ? local / kSecondsPerDay
                      : -((-local + kSecondsPerDay - 1) / kSecondsPerDay);
}

ReportService::ReportService(ledger::Ledger& journal, Clock clock)
    : journal_(journal), clock_(clock ? std::move(clock) : Clock{systemTime}) {}

Error ReportService::endOfDay(const Session& by, EodReport& out) {
    out = EodReport{};
    if (const Error e = authorize(by, Operation::EndOfDayReport); e != Error::Ok) {
        return e;
    }

    std::vector<ledger::JournalRecord> entries;
    if (const Error e = journal_.readJournal(entries); e != Error::Ok) {
        return e;
    }

    const std::int64_t day = journalDay(clock_());
    out.journal_day = day;
    out.day_label = dayLabel(day);

    std::map<AccountNo, Paise> balances;  // last balance_after per account
    EodTotals totals;
    for (const ledger::JournalRecord& entry : entries) {
        balances[entry.account] = entry.balance_after;
        if (journalDay(entry.unix_ts) != day) {
            continue;  // earlier days change no balance list or total
        }
        switch (entry.type) {
        case ledger::TxnType::Deposit: totals.deposits += entry.amount; break;
        case ledger::TxnType::Withdraw: totals.withdrawals += entry.amount; break;
        case ledger::TxnType::TransferOut: totals.transfer_out += entry.amount; break;
        case ledger::TxnType::TransferIn: totals.transfer_in += entry.amount; break;
        case ledger::TxnType::Open: break;
        }
    }
    out.totals = totals;
    out.balances.reserve(balances.size());
    for (const auto& entry : balances) {
        out.balances.push_back({entry.first, entry.second});
    }

    // Audit before returning Ok, so a report can never run without its
    // record (BMS-F-063), mirroring the unlock flow in AuthService.
    std::uint64_t seq = 0;
    std::string details = std::to_string(out.balances.size()) + " accounts closed off";
    switch (journal_.appendAudit(by.operator_id, ledger::AuditAction::ReportRun, out.day_label,
                                 details, seq)) {
    case Error::Ok:
        return Error::Ok;
    case Error::InvalidInput:
        return Error::InvalidInput;
    default:
        return Error::Io;
    }
}

}  // namespace bms
