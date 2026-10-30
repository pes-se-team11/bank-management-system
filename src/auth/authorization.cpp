#include "auth/authorization.h"

namespace bms {

namespace {

// The one place that says which role owns which operation (BMS-F-012).
// Roles are kept separate: a Manager does not inherit Teller operations.
Role requiredRole(Operation operation) {
    switch (operation) {
    case Operation::CreateCustomer:
    case Operation::ModifyCustomer:
    case Operation::OpenAccount:
        return Role::Teller;
    case Operation::CloseAccount:
    case Operation::UnlockAccount:
    case Operation::AddStaff:
    case Operation::EndOfDayReport:
    case Operation::ViewAuditLog:
        return Role::Manager;
    }
    return Role::Manager;  // unreachable; keeps every compiler happy
}

}  // namespace

Error authorize(const Session& session, Operation operation) {
    if (session.operator_id.empty() || session.role != requiredRole(operation)) {
        return Error::NotAuthorised;
    }
    return Error::Ok;
}

}  // namespace bms
