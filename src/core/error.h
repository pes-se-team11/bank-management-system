#pragma once
// Shared result codes so every module reports failures the same way
// (BMS-NF-005). Extend as modules land; never renumber existing values.

namespace bms {
enum class Error {
    Ok = 0,

    // Account, authentication and validation (Person 1).
    InvalidInput,        // malformed text or out-of-range value (BMS-SR-004)
    CustomerNotFound,    // no customer with that ID (BMS-F-005)
    AccountNotFound,     // no account with that number (BMS-F-005)
    CredentialNotFound,  // the store has no credential for that login ID
    BadCredentials,      // wrong ID *or* wrong secret - never says which (BMS-F-010)
    Io,                  // a store or file operation failed
    CorruptRecord,       // a journal or audit line failed to parse (BMS-SR-005)
    AccountLocked,       // three wrong PINs in a row; a manager must unlock it (BMS-F-011)
    NotAuthorised,       // the session's role may not do this (BMS-F-012)
    AccountNotLocked,    // unlock asked for an account that is not locked (BMS-F-013)
    AccountClosed,       // the account is CLOSED and accepts nothing more (BMS-F-004)
    BalanceNotZero,      // only a zero-balance account can be closed (BMS-F-004)

    // Transaction and platform (Person 2). Appended, never renumbered.
    BelowMinimumBalance,       // Savings debit would breach the minimum (BMS-F-032)
    DailyLimitExceeded,        // today's running total would pass the limit (BMS-F-030, BMS-F-052)
    TransferCeilingExceeded,   // one transfer is over the per-transaction ceiling (BMS-F-052)
    SameAccount,               // transfer source and destination are the same (BMS-F-051)
    InstanceLocked,            // another copy of the program holds the data directory (BMS-SR-006)
    InsecureDataDirectory,     // data directory is group- or world-writable (BMS-SR-006)
};
}  // namespace bms
