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
};
}  // namespace bms
