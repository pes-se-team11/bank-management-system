#include "auth/auth.h"

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

Error AuthService::enrol(const Principal& who, Role role, std::string& secret) {
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

    if (!known || !crypto::constantTimeEqual(attempt.data(), stored.hash.data(),
                                             attempt.size())) {
        return Error::BadCredentials;
    }
    out.operator_id = who.id;
    out.role = stored.role;
    return Error::Ok;
}

}  // namespace bms
