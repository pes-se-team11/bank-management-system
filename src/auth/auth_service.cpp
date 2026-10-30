#include "auth/auth.h"

#include "auth/authorization.h"
#include "auth/crypto.h"
#include "platform/platform.h"
#include "validation/validation.h"

namespace bms {

namespace {

const std::uint8_t* bytes(const std::string& s) {
    return reinterpret_cast<const std::uint8_t*>(s.data());
}

// Removes the secret from memory as soon as it has been used (BMS-F-014).
void wipe(std::string& secret) {
    platform::secureWipe(secret.data(), secret.size());
    secret.clear();
}

bool roleMatchesKind(PrincipalKind kind, Role role) {
    return kind == PrincipalKind::Customer ? role == Role::Customer
                                           : role != Role::Customer;
}

}  // namespace

Error AuthService::enrol(const Session& by, const Principal& who, Role role,
                         std::string& secret) {
    const Operation operation =
        who.kind == PrincipalKind::Customer ? Operation::OpenAccount : Operation::AddStaff;
    if (const Error e = authorize(by, operation); e != Error::Ok) {
        wipe(secret);
        return e;
    }
    return writeSecret(who, role, secret);
}

Error AuthService::enrolFirstManager(const std::string& staff_id, std::string& password) {
    bool staff_exists = false;
    if (const Error e = store_.anyStaff(staff_exists); e != Error::Ok) {
        wipe(password);
        return e;
    }
    if (staff_exists) {
        wipe(password);
        return Error::NotAuthorised;
    }
    return writeSecret(Principal{PrincipalKind::Staff, staff_id}, Role::Manager, password);
}

Error AuthService::writeSecret(const Principal& who, Role role, std::string& secret) {
    const Error secret_ok = who.kind == PrincipalKind::Customer
                                ? validation::validatePin(secret)
                                : validation::validatePassword(secret);
    if (who.id.empty() || !roleMatchesKind(who.kind, role) || secret_ok != Error::Ok) {
        wipe(secret);
        return Error::InvalidInput;
    }

    Credential credential;
    credential.principal = who;
    credential.role = role;
    credential.iterations = iterations_;
    if (!platform::secureRandom(credential.salt.data(), credential.salt.size())) {
        wipe(secret);
        return Error::Io;
    }
    crypto::pbkdf2Sha256(bytes(secret), secret.size(), credential.salt.data(),
                         credential.salt.size(), credential.iterations,
                         credential.hash.data(), credential.hash.size());
    wipe(secret);
    return store_.writeCredential(credential);
}

Error AuthService::login(const Principal& who, std::string& secret, Session& out) {
    Credential stored;
    const Error found = store_.readCredential(who, stored);
    if (found != Error::Ok && found != Error::CredentialNotFound) {
        wipe(secret);
        return found;  // a storage failure, not a wrong guess
    }

    // Hash even when the ID is unknown, so both failures cost the same time.
    const bool known = found == Error::Ok;
    std::array<std::uint8_t, 32> attempt{};
    crypto::pbkdf2Sha256(bytes(secret), secret.size(), stored.salt.data(),
                         stored.salt.size(), known ? stored.iterations : iterations_,
                         attempt.data(), attempt.size());
    wipe(secret);

    if (!known) {
        return Error::BadCredentials;
    }
    // A locked account refuses even the right PIN until a manager unlocks it.
    const bool customer = who.kind == PrincipalKind::Customer;
    if (customer && stored.failed_attempts >= kMaxFailedAttempts) {
        return Error::AccountLocked;
    }

    if (crypto::constantTimeEqual(attempt.data(), stored.hash.data(), attempt.size())) {
        // Only consecutive failures count, so a right PIN starts again at zero.
        if (stored.failed_attempts != 0) {
            stored.failed_attempts = 0;
            if (const Error e = store_.writeCredential(stored); e != Error::Ok) {
                return e;
            }
        }
        out.operator_id = who.id;
        out.role = stored.role;
        return Error::Ok;
    }

    if (!customer) {
        return Error::BadCredentials;
    }
    // The count is saved before anything else, so it is never lost even if
    // locking the account record or writing the audit entry fails.
    ++stored.failed_attempts;
    if (const Error e = store_.writeCredential(stored); e != Error::Ok) {
        return e;
    }
    if (stored.failed_attempts < kMaxFailedAttempts) {
        return Error::BadCredentials;
    }
    return lockAccount(who.id);
}

Error AuthService::lockAccount(const std::string& account_id) {
    // Mark the account LOCKED too, so staff see it and no transaction can
    // run on it while it is locked.
    std::uint32_t number = 0;
    if (validation::parseId(account_id, number) == Error::Ok) {
        AccountRecord record;
        const Error found = accounts_.readAccount(number, record);
        if (found == Error::Ok) {
            record.state = AccountState::Locked;
            if (const Error e = accounts_.updateAccount(record); e != Error::Ok) {
                return e;
            }
        } else if (found != Error::AccountNotFound) {
            return found;
        }
    }

    // The audit entry names the account, and the ledger adds the time.
    std::uint64_t seq = 0;
    const Error logged =
        audit_.appendAudit("system", ledger::AuditAction::AccountLock, account_id,
                           std::to_string(kMaxFailedAttempts) + " wrong PINs in a row", seq);
    return logged != Error::Ok ? logged : Error::AccountLocked;
}

}  // namespace bms
