
#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "account/account.h"
#include "auth/auth.h"
#include "persistence/crc32.h"
#include "persistence/file_account_store.h"
#include "persistence/file_credential_store.h"
#include "persistence/record_types.h"

using namespace bms;
using namespace bms::persistence;

namespace {

struct TempDir {
    std::string path;

    explicit TempDir(const std::string& name)
        : path("test_dir_persist_" + name) {
        clean();
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }

    ~TempDir() {
        clean();
    }

    void clean() const {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

Customer sampleCustomer(CustomerId id = 1) {
    Customer c;
    c.id = id;
    c.name = "Priya Sharma";
    c.address = "45 Residency Road, Bengaluru";
    c.contact = "+919876543210";
    c.gov_id_ref = "ABCDE1234F";
    return c;
}

AccountRecord sampleAccount(
    AccountNo acct_no = 100001,
    CustomerId cust_id = 1
) {
    AccountRecord a;
    a.acct_no = acct_no;
    a.cust_id = cust_id;
    a.type = AccountType::Savings;
    a.state = AccountState::Active;
    a.opening_balance = 500000;
    a.balance = 500000;
    a.overdraft_limit = 0;
    a.daily_withdrawn = 0;
    a.daily_transferred = 0;
    a.counter_date = 20261031;
    a.last_seq = 1;
    return a;
}

void refreshAccountCrc(
    std::array<std::uint8_t, kAccountRecordSize>& buf
) {
    const std::uint32_t crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kAccountRecordSize - 4
    ));

    writeUint32LE(buf.data() + kAccountRecordSize - 4, crc);
}

void refreshCredentialCrc(
    std::array<std::uint8_t, kCredentialRecordSize>& buf
) {
    const std::uint32_t crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCredentialRecordSize - 4
    ));

    writeUint32LE(buf.data() + kCredentialRecordSize - 4, crc);
}

}  // namespace

TEST_CASE("CRC32 matches the standard reference value") {
    CHECK(crc32("123456789") == 0xCBF43926U);
}

TEST_CASE("CRC32 of empty data is zero") {
    CHECK(crc32("") == 0U);
}

TEST_CASE("Customer persistence across store reopen") {
    TempDir dir("cust_reopen");

    {
        FileAccountStore store(dir.path);
        const Customer c = sampleCustomer(1);
        REQUIRE(store.writeCustomer(c) == Error::Ok);

        Customer read_back;
        REQUIRE(store.readCustomer(1, read_back) == Error::Ok);
        CHECK(read_back.id == 1);
        CHECK(read_back.name == "Priya Sharma");
        CHECK(read_back.address == "45 Residency Road, Bengaluru");
        CHECK(read_back.contact == "+919876543210");
        CHECK(read_back.gov_id_ref == "ABCDE1234F");
    }

    {
        FileAccountStore store(dir.path);
        Customer read_back;
        REQUIRE(store.readCustomer(1, read_back) == Error::Ok);
        CHECK(read_back.id == 1);
        CHECK(read_back.name == "Priya Sharma");
        CHECK(read_back.address == "45 Residency Road, Bengaluru");
        CHECK(read_back.contact == "+919876543210");
        CHECK(read_back.gov_id_ref == "ABCDE1234F");
    }
}

TEST_CASE("Account persistence and in-place updates survive store reopen") {
    TempDir dir("acct_reopen");

    {
        FileAccountStore store(dir.path);
        const AccountRecord a = sampleAccount(100001, 1);
        REQUIRE(store.createAccount(a) == Error::Ok);

        AccountRecord modified = a;
        modified.balance = 750000;
        modified.daily_withdrawn = 250000;
        modified.last_seq = 2;
        REQUIRE(store.updateAccount(modified) == Error::Ok);
    }

    {
        FileAccountStore store(dir.path);
        AccountRecord read_back;
        REQUIRE(store.readAccount(100001, read_back) == Error::Ok);
        CHECK(read_back.acct_no == 100001);
        CHECK(read_back.cust_id == 1);
        CHECK(read_back.type == AccountType::Savings);
        CHECK(read_back.state == AccountState::Active);
        CHECK(read_back.opening_balance == 500000);
        CHECK(read_back.balance == 750000);
        CHECK(read_back.daily_withdrawn == 250000);
        CHECK(read_back.last_seq == 2);
    }
}

TEST_CASE("Persistent monotonic customer and account ID generation") {
    TempDir dir("counters");

    {
        FileAccountStore store(dir.path);
        CustomerId c1 = 0;
        CustomerId c2 = 0;
        AccountNo a1 = 0;
        AccountNo a2 = 0;

        REQUIRE(store.nextCustomerId(c1) == Error::Ok);
        REQUIRE(store.nextCustomerId(c2) == Error::Ok);
        CHECK(c1 == 1);
        CHECK(c2 == 2);

        REQUIRE(store.nextAccountNo(a1) == Error::Ok);
        REQUIRE(store.nextAccountNo(a2) == Error::Ok);
        CHECK(a1 == 100001);
        CHECK(a2 == 100002);
    }

    {
        FileAccountStore store(dir.path);
        CustomerId c3 = 0;
        AccountNo a3 = 0;

        REQUIRE(store.nextCustomerId(c3) == Error::Ok);
        REQUIRE(store.nextAccountNo(a3) == Error::Ok);
        CHECK(c3 == 3);
        CHECK(a3 == 100003);
    }
}

TEST_CASE("List accounts by customer") {
    TempDir dir("list_accts");

    {
        FileAccountStore store(dir.path);
        REQUIRE(store.createAccount(sampleAccount(100001, 1)) == Error::Ok);
        REQUIRE(store.createAccount(sampleAccount(100002, 1)) == Error::Ok);
        REQUIRE(store.createAccount(sampleAccount(100003, 2)) == Error::Ok);
    }

    {
        FileAccountStore store(dir.path);
        std::vector<AccountRecord> cust1_accts;
        std::vector<AccountRecord> cust2_accts;
        std::vector<AccountRecord> cust3_accts;

        REQUIRE(store.listAccounts(1, cust1_accts) == Error::Ok);
        CHECK(cust1_accts.size() == 2);
        CHECK(cust1_accts[0].acct_no == 100001);
        CHECK(cust1_accts[1].acct_no == 100002);

        REQUIRE(store.listAccounts(2, cust2_accts) == Error::Ok);
        CHECK(cust2_accts.size() == 1);
        CHECK(cust2_accts[0].acct_no == 100003);

        REQUIRE(store.listAccounts(99, cust3_accts) == Error::Ok);
        CHECK(cust3_accts.empty());
    }
}

TEST_CASE("Unknown customer and account handling") {
    TempDir dir("unknowns");
    FileAccountStore store(dir.path);

    Customer c;
    CHECK(store.readCustomer(404, c) == Error::CustomerNotFound);

    AccountRecord a;
    CHECK(store.readAccount(404, a) == Error::AccountNotFound);

    AccountRecord missing_update;
    missing_update.acct_no = 999999;
    CHECK(store.updateAccount(missing_update) == Error::AccountNotFound);
}

TEST_CASE("Validation of customer and account input") {
    TempDir dir("validation");
    FileAccountStore store(dir.path);

    Customer invalid_cust;
    invalid_cust.id = 0;
    CHECK(store.writeCustomer(invalid_cust) == Error::InvalidInput);

    Customer long_name = sampleCustomer(1);
    long_name.name = std::string(61, 'A');
    CHECK(store.writeCustomer(long_name) == Error::InvalidInput);

    AccountRecord invalid_acct;
    invalid_acct.acct_no = 0;
    CHECK(store.createAccount(invalid_acct) == Error::InvalidInput);

    AccountRecord valid_acct = sampleAccount(100001, 1);
    REQUIRE(store.createAccount(valid_acct) == Error::Ok);
    CHECK(store.createAccount(valid_acct) == Error::InvalidInput);
}

TEST_CASE("Credential persistence and customer/staff namespace separation") {
    TempDir dir("credentials");

    {
        FileCredentialStore store(dir.path);
        CHECK_FALSE(store.hasStaff());

        Credential cust_cred;
        cust_cred.principal = {PrincipalKind::Customer, "100001"};
        cust_cred.role = Role::Customer;
        cust_cred.salt = {
            1, 2, 3, 4, 5, 6, 7, 8,
            9, 10, 11, 12, 13, 14, 15, 16
        };
        cust_cred.hash.fill(0xAA);
        cust_cred.iterations = 100000;
        cust_cred.failed_attempts = 1;
        REQUIRE(store.writeCredential(cust_cred) == Error::Ok);

        Credential staff_cred;
        staff_cred.principal = {PrincipalKind::Staff, "100001"};
        staff_cred.role = Role::Teller;
        staff_cred.salt = {
            16, 15, 14, 13, 12, 11, 10, 9,
            8, 7, 6, 5, 4, 3, 2, 1
        };
        staff_cred.hash.fill(0xBB);
        staff_cred.iterations = 120000;
        staff_cred.failed_attempts = 0;
        REQUIRE(store.writeCredential(staff_cred) == Error::Ok);

        CHECK(store.hasStaff());
    }

    {
        FileCredentialStore store(dir.path);
        CHECK(store.hasStaff());

        Credential read_cust;
        REQUIRE(store.readCredential(
            {PrincipalKind::Customer, "100001"}, read_cust
        ) == Error::Ok);

        CHECK(read_cust.principal.kind == PrincipalKind::Customer);
        CHECK(read_cust.principal.id == "100001");
        CHECK(read_cust.role == Role::Customer);
        CHECK(read_cust.salt[0] == 1);
        CHECK(read_cust.hash[0] == 0xAA);
        CHECK(read_cust.iterations == 100000);
        CHECK(read_cust.failed_attempts == 1);

        Credential read_staff;
        REQUIRE(store.readCredential(
            {PrincipalKind::Staff, "100001"}, read_staff
        ) == Error::Ok);

        CHECK(read_staff.principal.kind == PrincipalKind::Staff);
        CHECK(read_staff.principal.id == "100001");
        CHECK(read_staff.role == Role::Teller);
        CHECK(read_staff.salt[0] == 16);
        CHECK(read_staff.hash[0] == 0xBB);
        CHECK(read_staff.iterations == 120000);
        CHECK(read_staff.failed_attempts == 0);

        Credential unknown;
        CHECK(store.readCredential(
            {PrincipalKind::Customer, "999999"}, unknown
        ) == Error::CredentialNotFound);

        CHECK(store.readCredential(
            {PrincipalKind::Customer, ""}, unknown
        ) == Error::InvalidInput);
    }
}

TEST_CASE("Account deserialization rejects invalid enum values") {
    AccountRecord account = sampleAccount();
    std::array<std::uint8_t, kAccountRecordSize> buf{};

   REQUIRE(serializeAccount(account, buf) == Error::Ok);

    buf[8] = 2;
    refreshAccountCrc(buf);

    AccountRecord result;
    CHECK(deserializeAccount(buf, result) == Error::CorruptRecord);

    REQUIRE(serializeAccount(account, buf) == Error::Ok);
    buf[9] = 3;
    refreshAccountCrc(buf);

    CHECK(deserializeAccount(buf, result) == Error::CorruptRecord);
}

TEST_CASE("Credential deserialization rejects invalid enum values") {
    Credential credential;
    credential.principal = {PrincipalKind::Customer, "100001"};
    credential.role = Role::Customer;
    credential.salt.fill(1);
    credential.hash.fill(2);
    credential.iterations = 1000;
    credential.failed_attempts = 0;

    std::array<std::uint8_t, kCredentialRecordSize> buf{};
    serializeCredential(credential, buf);

    Credential result;

    buf[0] = 2;
    refreshCredentialCrc(buf);

    CHECK(deserializeCredential(buf, result) == Error::CorruptRecord);

    serializeCredential(credential, buf);
    buf[33] = 3;
    refreshCredentialCrc(buf);

    CHECK(deserializeCredential(buf, result) == Error::CorruptRecord);
}

TEST_CASE("CRC32 validation detects corrupted account records") {
    TempDir dir("corrupt_acct");

    {
        FileAccountStore store(dir.path);
        REQUIRE(store.createAccount(sampleAccount(100001, 1)) == Error::Ok);
    }

    const std::string acct_path = dir.path + "/accounts.dat";

    {
        std::fstream file(
            acct_path,
            std::ios::in | std::ios::out | std::ios::binary
        );
        REQUIRE(file.is_open());
        file.seekp(10);
        char corrupt_byte = 0x7F;
        file.write(&corrupt_byte, 1);
        file.flush();
    }

    {
        FileAccountStore store(dir.path);
        AccountRecord rec;
        CHECK(store.readAccount(100001, rec) == Error::CorruptRecord);
    }
}

TEST_CASE("CRC32 validation detects corrupted customer records") {
    TempDir dir("corrupt_cust");

    {
        FileAccountStore store(dir.path);
        REQUIRE(store.writeCustomer(sampleCustomer(1)) == Error::Ok);
    }

    const std::string cust_path = dir.path + "/customers.dat";

    {
        std::fstream file(
            cust_path,
            std::ios::in | std::ios::out | std::ios::binary
        );
        REQUIRE(file.is_open());
        file.seekp(15);
        char corrupt_byte = 0x5A;
        file.write(&corrupt_byte, 1);
        file.flush();
    }

    {
        FileAccountStore store(dir.path);
        Customer c;
        CHECK(store.readCustomer(1, c) == Error::CorruptRecord);
    }
}

TEST_CASE("CRC32 validation detects corrupted credential records") {
    TempDir dir("corrupt_cred");

    {
        FileCredentialStore store(dir.path);
        Credential cred;
        cred.principal = {PrincipalKind::Customer, "100001"};
        cred.role = Role::Customer;
        cred.salt.fill(1);
        cred.hash.fill(2);
        cred.iterations = 1000;
        cred.failed_attempts = 0;
        REQUIRE(store.writeCredential(cred) == Error::Ok);
    }

    const std::string cred_path = dir.path + "/credentials.dat";

    {
        std::fstream file(
            cred_path,
            std::ios::in | std::ios::out | std::ios::binary
        );
        REQUIRE(file.is_open());
        file.seekp(20);
        char corrupt_byte = 0x11;
        file.write(&corrupt_byte, 1);
        file.flush();
    }

    {
        FileCredentialStore store(dir.path);
        Credential c;
        CHECK(store.readCredential(
            {PrincipalKind::Customer, "100001"}, c
        ) == Error::CorruptRecord);
    }
}

TEST_CASE("Credential store rejects incomplete trailing records") {
    TempDir dir("partial_cred");

    {
        FileCredentialStore store(dir.path);

        Credential cred;
        cred.principal = {PrincipalKind::Customer, "100001"};
        cred.role = Role::Customer;
        cred.salt.fill(1);
        cred.hash.fill(2);
        cred.iterations = 1000;
        cred.failed_attempts = 0;

        REQUIRE(store.writeCredential(cred) == Error::Ok);
    }

    const std::string path = dir.path + "/credentials.dat";

    {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        REQUIRE(file.is_open());

        const char extra_byte = 0x42;
        file.write(&extra_byte, 1);
        REQUIRE(file.good());
    }

    FileCredentialStore reopened(dir.path);
    CHECK(reopened.status() == Error::CorruptRecord);
}

TEST_CASE("Credential store rejects duplicate principal records") {
    TempDir dir("duplicate_cred");

    {
        FileCredentialStore store(dir.path);

        Credential cred;
        cred.principal = {PrincipalKind::Customer, "100001"};
        cred.role = Role::Customer;
        cred.salt.fill(1);
        cred.hash.fill(2);
        cred.iterations = 1000;
        cred.failed_attempts = 0;

        REQUIRE(store.writeCredential(cred) == Error::Ok);
    }

    const std::string path = dir.path + "/credentials.dat";

    {
        std::ifstream input(path, std::ios::binary);
        REQUIRE(input.is_open());

        std::vector<char> record(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>()
        );

        REQUIRE(record.size() == kCredentialRecordSize);

        std::ofstream output(path, std::ios::binary | std::ios::app);
        REQUIRE(output.is_open());

        output.write(
            record.data(),
            static_cast<std::streamsize>(record.size())
        );
        REQUIRE(output.good());
    }

    FileCredentialStore reopened(dir.path);
    CHECK(reopened.status() == Error::CorruptRecord);
}

TEST_CASE("End-to-End Persistence: Account creation, restart, and plaintext security check") {
    TempDir dir("e2e_persistence");

    const std::string secret_pin = "4826";
    const std::string staff_pass = "ManagerSecurePass123";

    {
        FileAccountStore account_store(dir.path);
        FileCredentialStore credential_store(dir.path);
        AuthService auth(credential_store, 1000);

        Principal mgr_principal{PrincipalKind::Staff, "manager1"};
        std::string pass_copy = staff_pass;
        REQUIRE(auth.enrol(
            mgr_principal, Role::Manager, pass_copy
        ) == Error::Ok);

        CustomerId cust_id = 0;
        REQUIRE(account_store.nextCustomerId(cust_id) == Error::Ok);

        Customer c;
        c.id = cust_id;
        c.name = "Vikram Aditya";
        c.address = "101 Brigade Road, Bengaluru";
        c.contact = "+919123456789";
        c.gov_id_ref = "ABCDE9876Z";
        REQUIRE(account_store.writeCustomer(c) == Error::Ok);

        AccountNo acct_no = 0;
        REQUIRE(account_store.nextAccountNo(acct_no) == Error::Ok);

        AccountRecord a;
        a.acct_no = acct_no;
        a.cust_id = cust_id;
        a.type = AccountType::Savings;
        a.state = AccountState::Active;
        a.opening_balance = 250000;
        a.balance = 250000;
        a.overdraft_limit = 0;
        a.daily_withdrawn = 0;
        a.daily_transferred = 0;
        a.counter_date = 20261031;
        a.last_seq = 1;
        REQUIRE(account_store.createAccount(a) == Error::Ok);

        Principal cust_principal{
            PrincipalKind::Customer,
            std::to_string(acct_no)
        };

        std::string pin_copy = secret_pin;
        REQUIRE(auth.enrol(
            cust_principal, Role::Customer, pin_copy
        ) == Error::Ok);
    }

    {
        auto fileContainsString = [](
            const std::string& filepath,
            const std::string& target
        ) -> bool {
            std::ifstream file(filepath, std::ios::binary);
            if (!file) {
                return false;
            }

            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );

            return content.find(target) != std::string::npos;
        };

        const std::string cred_file = dir.path + "/credentials.dat";
        const std::string acct_file = dir.path + "/accounts.dat";
        const std::string cust_file = dir.path + "/customers.dat";
        const std::string count_file = dir.path + "/counters.dat";

        CHECK_FALSE(fileContainsString(cred_file, secret_pin));
        CHECK_FALSE(fileContainsString(cred_file, staff_pass));
        CHECK_FALSE(fileContainsString(acct_file, secret_pin));
        CHECK_FALSE(fileContainsString(cust_file, secret_pin));
        CHECK_FALSE(fileContainsString(count_file, secret_pin));
    }

    {
        FileAccountStore account_store(dir.path);
        FileCredentialStore credential_store(dir.path);
        AuthService auth(credential_store, 1000);

        CHECK(credential_store.hasStaff());

        Session mgr_session;
        std::string pass_attempt = staff_pass;

        REQUIRE(auth.login(
            {PrincipalKind::Staff, "manager1"},
            pass_attempt,
            mgr_session
        ) == Error::Ok);

        CHECK(mgr_session.operator_id == "manager1");
        CHECK(mgr_session.role == Role::Manager);

        Customer loaded_cust;
        REQUIRE(account_store.readCustomer(1, loaded_cust) == Error::Ok);
        CHECK(loaded_cust.name == "Vikram Aditya");
        CHECK(loaded_cust.contact == "+919123456789");

        AccountRecord loaded_acct;
        REQUIRE(account_store.readAccount(100001, loaded_acct) == Error::Ok);
        CHECK(loaded_acct.cust_id == 1);
        CHECK(loaded_acct.balance == 250000);
        CHECK(loaded_acct.state == AccountState::Active);

        Session cust_session;
        std::string pin_attempt = secret_pin;

        REQUIRE(auth.login(
            {PrincipalKind::Customer, "100001"},
            pin_attempt,
            cust_session
        ) == Error::Ok);

        CHECK(cust_session.operator_id == "100001");
        CHECK(cust_session.role == Role::Customer);

        std::string wrong_pin = "9999";

        CHECK(auth.login(
            {PrincipalKind::Customer, "100001"},
            wrong_pin,
            cust_session
        ) == Error::BadCredentials);
    }
}

TEST_CASE("Credential store rejects invalid principal-role combinations") {
    TempDir dir("invalid_credential_roles");
    FileCredentialStore store(dir.path);

    Credential customer;
    customer.principal = {PrincipalKind::Customer, "100001"};
    customer.role = Role::Teller;
    customer.iterations = 1000;

    CHECK(store.writeCredential(customer) == Error::InvalidInput);

    Credential staff;
    staff.principal = {PrincipalKind::Staff, "STAFF01"};
    staff.role = Role::Customer;
    staff.iterations = 1000;

    CHECK(store.writeCredential(staff) == Error::InvalidInput);
}

TEST_CASE("Credential store rejects zero PBKDF2 iterations") {
    TempDir dir("zero_iterations");
    FileCredentialStore store(dir.path);

    Credential credential;
    credential.principal = {PrincipalKind::Customer, "100001"};
    credential.role = Role::Customer;
    credential.iterations = 0;

    CHECK(store.writeCredential(credential) == Error::InvalidInput);
}

