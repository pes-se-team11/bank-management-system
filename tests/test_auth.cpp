#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include "auth/auth.h"
#include "fake_stores.h"
#include "ledger/ledger.h"
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

// Stores, an audit log that starts empty, and the service under test.
// Members are built in this order, so the files are cleared before the
// ledger reads them.
struct AuthFixture {
    struct Files {
        explicit Files(const std::string& tag)
            : journal("test_auth_" + tag + "_journal.tmp"),
              audit("test_auth_" + tag + "_audit.tmp") {
            clear();
        }
        ~Files() { clear(); }
        void clear() const {
            std::remove(journal.c_str());
            std::remove(audit.c_str());
        }
        std::string journal;
        std::string audit;
    };

    explicit AuthFixture(const std::string& tag)
        : files(tag),
          ledger(files.journal, files.audit),
          auth(store, accounts, ledger, kTestIterations) {}

    Files files;
    test::MemoryCredentialStore store;
    test::MemoryAccountStore accounts;
    ledger::Ledger ledger;
    AuthService auth;
};

// One wrong-PIN attempt; the PIN string is consumed by login.
Error tryPin(AuthService& auth, const std::string& account_no, std::string pin) {
    Session s;
    return auth.login(customer(account_no), pin, s);
}

}  // namespace

TEST_CASE("TC-AUT-01: correct credentials open a session with the stored role") {
    AuthFixture fx("session");
    AuthService& auth = fx.auth;

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
    AuthFixture fx("same");
    AuthService& auth = fx.auth;
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
    AuthFixture fx("hash");
    test::MemoryCredentialStore& store = fx.store;
    AuthService& auth = fx.auth;

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
    AuthFixture fx("wipe");
    AuthService& auth = fx.auth;

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
    AuthFixture fx("enrol");
    test::MemoryCredentialStore& store = fx.store;
    AuthService& auth = fx.auth;

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

TEST_CASE("TC-AUT-02: the third wrong PIN locks the account and records it") {
    AuthFixture fx("lock");
    CustomerId cust = 0;
    REQUIRE(fx.accounts.nextCustomerId(cust) == Error::Ok);
    AccountRecord account;
    account.acct_no = 100001;
    account.cust_id = cust;
    REQUIRE(fx.accounts.createAccount(account) == Error::Ok);
    std::string pin = "4821";
    REQUIRE(fx.auth.enrol(customer("100001"), Role::Customer, pin) == Error::Ok);

    CHECK(tryPin(fx.auth, "100001", "1111") == Error::BadCredentials);
    CHECK(tryPin(fx.auth, "100001", "2222") == Error::BadCredentials);
    CHECK(tryPin(fx.auth, "100001", "3333") == Error::AccountLocked);

    // AC-BMS-F-011: the fourth attempt is refused as LOCKED even with the
    // right PIN.
    CHECK(tryPin(fx.auth, "100001", "4821") == Error::AccountLocked);

    AccountRecord stored;
    REQUIRE(fx.accounts.readAccount(100001, stored) == Error::Ok);
    CHECK(stored.state == AccountState::Locked);

    // Exactly one audit event, naming the account; the ledger adds the time.
    std::vector<ledger::AuditRecord> audit;
    REQUIRE(fx.ledger.readAudit(audit) == Error::Ok);
    REQUIRE(audit.size() == 1);
    CHECK(audit[0].action == ledger::AuditAction::AccountLock);
    CHECK(audit[0].target == "100001");
    CHECK(audit[0].unix_ts > 0);
}

TEST_CASE("TC-AUT-02: a right PIN resets the count, so only failures in a row lock") {
    AuthFixture fx("reset");
    std::string pin = "4821";
    REQUIRE(fx.auth.enrol(customer("100001"), Role::Customer, pin) == Error::Ok);

    CHECK(tryPin(fx.auth, "100001", "1111") == Error::BadCredentials);
    CHECK(tryPin(fx.auth, "100001", "2222") == Error::BadCredentials);
    CHECK(tryPin(fx.auth, "100001", "4821") == Error::Ok);

    Credential stored;
    REQUIRE(fx.store.readCredential(customer("100001"), stored) == Error::Ok);
    CHECK(stored.failed_attempts == 0);

    CHECK(tryPin(fx.auth, "100001", "1111") == Error::BadCredentials);
    CHECK(tryPin(fx.auth, "100001", "2222") == Error::BadCredentials);
    CHECK(tryPin(fx.auth, "100001", "4821") == Error::Ok);  // still not locked

    std::vector<ledger::AuditRecord> audit;
    REQUIRE(fx.ledger.readAudit(audit) == Error::Ok);
    CHECK(audit.empty());
}

TEST_CASE("TC-AUT-02: lockout touches only the account that was guessed at") {
    AuthFixture fx("isolated");
    std::string pin_a = "4821";
    std::string pin_b = "5932";
    REQUIRE(fx.auth.enrol(customer("100001"), Role::Customer, pin_a) == Error::Ok);
    REQUIRE(fx.auth.enrol(customer("100002"), Role::Customer, pin_b) == Error::Ok);

    for (const char* wrong : {"1111", "2222", "3333"}) {
        tryPin(fx.auth, "100001", wrong);
    }
    CHECK(tryPin(fx.auth, "100001", "4821") == Error::AccountLocked);
    CHECK(tryPin(fx.auth, "100002", "5932") == Error::Ok);

    // Guessing at an ID that does not exist locks nothing and says nothing.
    for (const char* wrong : {"1111", "2222", "3333", "4444"}) {
        CHECK(tryPin(fx.auth, "999999", wrong) == Error::BadCredentials);
    }
}

TEST_CASE("TC-AUT-02: staff passwords are never locked") {
    AuthFixture fx("staff");
    std::string password = "manager-pass1";
    REQUIRE(fx.auth.enrol(staff("M001"), Role::Manager, password) == Error::Ok);

    Session s;
    for (int i = 0; i < 5; ++i) {
        std::string wrong = "wrong-pass1";
        CHECK(fx.auth.login(staff("M001"), wrong, s) == Error::BadCredentials);
    }
    std::string right = "manager-pass1";
    CHECK(fx.auth.login(staff("M001"), right, s) == Error::Ok);
    CHECK(s.role == Role::Manager);
}
