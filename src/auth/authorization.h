#pragma once
// AuthModule (Person 1): story 2.3, role-based authorisation (BMS-F-012).
// Every privileged service function calls authorize() itself, immediately
// before it acts (BMS-SR-007). The menu only hides options a role cannot
// use; it is a convenience, not the security boundary.
#include "auth/auth.h"
#include "core/error.h"

namespace bms {

enum class Operation {
    // Tellers only.
    CreateCustomer,
    ModifyCustomer,
    OpenAccount,  // includes setting the new account's PIN
    // Managers only.
    CloseAccount,
    UnlockAccount,
    AddStaff,
    EndOfDayReport,
    ViewAuditLog,
};

// Ok when the session's role may perform the operation; NotAuthorised
// otherwise, and always for a session with no operator ID.
Error authorize(const Session& session, Operation operation);

}  // namespace bms
