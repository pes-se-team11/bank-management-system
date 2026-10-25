#pragma once
// AuthModule (Person 1): stories 2.1 (sign-in) and 7.3 (credential hashing).
#include <array>
#include <cstdint>
#include <string>

#include "core/error.h"

namespace bms {

enum class Role { Customer, Teller, Manager };

// Customers sign in with an account number, staff with a staff ID, so the
// two kinds of login ID live in separate namespaces.
enum class PrincipalKind { Customer, Staff };

struct Principal {
    PrincipalKind kind = PrincipalKind::Customer;
    std::string id;  // account number for customers, staff ID for staff
};

// Created at sign-in and passed explicitly to every service call; there are
// no global variables (SAD 9.1).
struct Session {
    std::string operator_id;
    Role role = Role::Customer;
};

// What is stored for each login ID: never the secret itself, only a salted
// hash of it (BMS-SR-001). The PIN belongs to the account, which is what
// lets story 2.2 lock one account after three wrong PINs (BMS-F-011).
struct Credential {
    Principal principal;
    Role role = Role::Customer;
    std::array<std::uint8_t, 16> salt{};
    std::array<std::uint8_t, 32> hash{};
    std::uint32_t iterations = 0;       // stored, so the cost can be raised later
    std::uint32_t failed_attempts = 0;  // used by story 2.2 (BMS-F-011)
};

// What AuthModule needs from storage; PersistenceModule implements it over
// the customer and staff records (SAD 9.3).
class CredentialStore {
public:
    virtual ~CredentialStore() = default;
    virtual Error readCredential(const Principal& who, Credential& out) = 0;  // CredentialNotFound
    virtual Error writeCredential(const Credential& credential) = 0;
};

class AuthService {
public:
    // PBKDF2 rounds per hash. Tests pass a small number to stay fast.
    static constexpr std::uint32_t kDefaultIterations = 100000;

    explicit AuthService(CredentialStore& store,
                         std::uint32_t iterations = kDefaultIterations)
        : store_(store), iterations_(iterations) {}

    // Sets or replaces the secret for a login ID: a 4-6 digit PIN for a
    // customer, an 8-64 character password for staff. A fresh random salt
    // is used every time. The caller's copy of the secret is wiped.
    Error enrol(const Principal& who, Role role, std::string& secret);

    // BMS-F-010. Ok fills in the session. An unknown ID and a wrong secret
    // both return BadCredentials, after the same amount of work, so the
    // answer never reveals which one was wrong. The secret is always wiped.
    Error login(const Principal& who, std::string& secret, Session& out);

private:
    CredentialStore& store_;
    std::uint32_t iterations_;
};

}  // namespace bms
