#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <string>

#include "auth/auth.h"
#include "fake_stores.h"
#include "platform/platform.h"

using namespace bms;

namespace {

// Low iteration count keeps the tests fast; production uses the default.
constexpr std::uint32_t kTestIterations = 1000;

Principal customer(const std::string& account_no) {
    return Principal{PrincipalKind::Customer, account_no};
}

Principal staff(const std::string& staff_id) {
    return Principal{PrincipalKind::Staff, staff_id};
}

}  // namespace

TEST_CASE("TC-AUT-01: correct credentials open a session with the stored role") {
    test::MemoryCredentialStore store;
    AuthService auth(store, kTestIterations);

    std::string pin = "4821";
    REQUIRE(auth.enrol(customer("100001"), Role::Customer, pin) == Error::Ok);
    std::string password = "teller-pass1";
    REQUIRE(auth.enrol(staff("T007"), Role::Teller, password) == Error::Ok);

    Session s;
    std::string attempt = "4821";
    REQUIRE(auth.login(customer("100001"), attempt, s) == Error::Ok);
    CHECK(s.operator_id == "100001");
    CHECK(s.role == Role::Customer);

    attempt = "teller-pass1";
    REQUIRE(auth.login(staff("T007"), attempt, s) == Error::Ok);
    CHECK(s.operator_id == "T007");
    CHECK(s.role == Role::Teller);
}

TEST_CASE("TC-AUT-01: a wrong PIN and an unknown ID give the same refusal") {
    test::MemoryCredentialStore store;
    AuthService auth(store, kTestIterations);
    std::string pin = "4821";
    REQUIRE(auth.enrol(customer("100001"), Role::Customer, pin) == Error::Ok);

    Session s;
    std::string wrong_pin = "1111";
    std::string any_pin = "4821";
    const Error wrong = auth.login(customer("100001"), wrong_pin, s);
    const Error unknown = auth.login(customer("999999"), any_pin, s);
    CHECK(wrong == Error::BadCredentials);
    CHECK(unknown == Error::BadCredentials);  // nothing says which part was wrong

    // A staff ID is not a valid customer login, even with the right secret.
    std::string password = "teller-pass1";
    REQUIRE(auth.enrol(staff("T007"), Role::Teller, password) == Error::Ok);
    password = "teller-pass1";
    CHECK(auth.login(customer("T007"), password, s) == Error::BadCredentials);
}

TEST_CASE("TC-SEC-01: only a salted hash is stored, never the PIN") {
    test::MemoryCredentialStore store;
    AuthService auth(store, kTestIterations);

    std::string pin_a = "4821";
    std::string pin_b = "4821";  // same PIN on two accounts
    REQUIRE(auth.enrol(customer("100001"), Role::Customer, pin_a) == Error::Ok);
    REQUIRE(auth.enrol(customer("100002"), Role::Customer, pin_b) == Error::Ok);

    Credential a;
    Credential b;
    REQUIRE(store.readCredential(customer("100001"), a) == Error::Ok);
    REQUIRE(store.readCredential(customer("100002"), b) == Error::Ok);

    // Each credential gets its own random salt, so equal PINs hash differently.
    CHECK(a.salt != b.salt);
    CHECK(a.hash != b.hash);
    CHECK(a.iterations == kTestIterations);

    // The PIN's digits do not appear anywhere in what is stored.
    const std::string needle = "4821";
    const auto contains = [&](const auto& field) {
        return std::search(field.begin(), field.end(), needle.begin(), needle.end()) !=
               field.end();
    };
    CHECK_FALSE(contains(a.hash));
    CHECK_FALSE(contains(a.salt));
}

TEST_CASE("TC-SEC-02: the caller's copy of the secret is wiped after use") {
    test::MemoryCredentialStore store;
    AuthService auth(store, kTestIterations);

    std::string pin = "4821";
    REQUIRE(auth.enrol(customer("100001"), Role::Customer, pin) == Error::Ok);
    CHECK(pin.empty());

    Session s;
    std::string attempt = "4821";
    REQUIRE(auth.login(customer("100001"), attempt, s) == Error::Ok);
    CHECK(attempt.empty());

    std::string wrong = "9999";
    CHECK(auth.login(customer("100001"), wrong, s) == Error::BadCredentials);
    CHECK(wrong.empty());  // wiped on failure too

    char buffer[] = "4821";
    platform::secureWipe(buffer, sizeof buffer);
    for (const char c : buffer) {
        CHECK(c == '\0');
    }
}

TEST_CASE("auth: enrol refuses weak secrets and mismatched roles") {
    test::MemoryCredentialStore store;
    AuthService auth(store, kTestIterations);

    std::string short_pin = "12";
    CHECK(auth.enrol(customer("100001"), Role::Customer, short_pin) == Error::InvalidInput);
    std::string short_password = "abc";
    CHECK(auth.enrol(staff("T007"), Role::Teller, short_password) == Error::InvalidInput);

    // A customer login cannot be given a staff role, and the reverse.
    std::string pin = "4821";
    CHECK(auth.enrol(customer("100001"), Role::Manager, pin) == Error::InvalidInput);
    std::string password = "manager-pass1";
    CHECK(auth.enrol(staff("M001"), Role::Customer, password) == Error::InvalidInput);

    CHECK(store.credentials.empty());
}

TEST_CASE("auth: the OS random source produces fresh bytes") {
    std::array<std::uint8_t, 16> first{};
    std::array<std::uint8_t, 16> second{};
    REQUIRE(platform::secureRandom(first.data(), first.size()));
    REQUIRE(platform::secureRandom(second.data(), second.size()));
    CHECK(first != second);
}
