// Story 7.5 / BMS-SR-006 (TC-SEC-07) and the counters-file crash fix.
#include <doctest/doctest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "account/account.h"
#include "persistence/file_account_store.h"
#include "persistence/record_types.h"
#include "platform/platform.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace {

struct Dir {
    std::string path;
    explicit Dir(const std::string& name) : path("test_dir_platform_" + name) {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    ~Dir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

}  // namespace

TEST_CASE("TC-SEC-07: a second instance cannot take the lock, and a crash frees it") {
    Dir dir("lock");
    std::error_code ec;
    fs::create_directories(dir.path, ec);
    const std::string lock_path = dir.path + "/.lock";

    bms::platform::InstanceLock first;
    REQUIRE(first.acquire(lock_path));
    CHECK(first.acquire(lock_path));  // asking again while held is harmless
    {
        bms::platform::InstanceLock second;
        CHECK_FALSE(second.acquire(lock_path));
    }
    {
        // Releasing (destruction is what process exit does) frees it.
        bms::platform::InstanceLock holder;
        REQUIRE(holder.acquire(dir.path + "/.other"));
    }
    bms::platform::InstanceLock again;
    CHECK(again.acquire(dir.path + "/.other"));
}

#ifndef _WIN32
TEST_CASE("TC-SEC-07: data folder is created owner-only and files inherit owner-only") {
    Dir dir("perm_new");
    std::string reason;
    REQUIRE(bms::platform::secureDataDirectory(dir.path, reason));

    struct stat info{};
    REQUIRE(::stat(dir.path.c_str(), &info) == 0);
    CHECK((info.st_mode & 0777) == 0700);

    {
        std::ofstream f(dir.path + "/accounts.dat");
        f << "x";
    }
    REQUIRE(::stat((dir.path + "/accounts.dat").c_str(), &info) == 0);
    CHECK((info.st_mode & 0777) == 0600);
}

TEST_CASE("TC-SEC-07: a group- or world-writable data folder is refused with a clear message") {
    Dir dir("perm_loose");
    std::error_code ec;
    fs::create_directories(dir.path, ec);

    REQUIRE(::chmod(dir.path.c_str(), 0770) == 0);
    std::string reason;
    CHECK_FALSE(bms::platform::secureDataDirectory(dir.path, reason));
    CHECK(reason.find("chmod 700") != std::string::npos);

    REQUIRE(::chmod(dir.path.c_str(), 0707) == 0);
    reason.clear();
    CHECK_FALSE(bms::platform::secureDataDirectory(dir.path, reason));
    CHECK_FALSE(reason.empty());

    REQUIRE(::chmod(dir.path.c_str(), 0700) == 0);
    reason.clear();
    CHECK(bms::platform::secureDataDirectory(dir.path, reason));
}
#endif

TEST_CASE("Storage fix: a torn counters.dat is rebuilt from the data files, not fatal") {
    Dir dir("counters");
    bms::AccountNo last_account = 0;
    {
        bms::persistence::FileAccountStore store(dir.path);
        REQUIRE(store.status() == bms::Error::Ok);
        bms::CustomerId cid = 0;
        REQUIRE(store.nextCustomerId(cid) == bms::Error::Ok);
        bms::Customer c{cid, "Asha Rao", "12 MG Road", "9876543210", "ABCDE1234F"};
        REQUIRE(store.writeCustomer(c) == bms::Error::Ok);
        REQUIRE(store.nextAccountNo(last_account) == bms::Error::Ok);
        bms::AccountRecord a{};
        a.acct_no = last_account;
        a.cust_id = cid;
        a.balance = a.opening_balance = 5000;
        REQUIRE(store.createAccount(a) == bms::Error::Ok);
    }

    // Simulate the crash the old in-place truncate allowed: a half-written file.
    {
        std::ofstream torn(dir.path + "/counters.dat",
                           std::ios::binary | std::ios::trunc);
        torn << "abc";
    }

    bms::persistence::FileAccountStore reopened(dir.path);
    REQUIRE(reopened.status() == bms::Error::Ok);  // used to be CorruptRecord
    bms::AccountNo next = 0;
    REQUIRE(reopened.nextAccountNo(next) == bms::Error::Ok);
    CHECK(next == last_account + 1);  // an issued number is never reused
    bms::AccountRecord back{};
    CHECK(reopened.readAccount(last_account, back) == bms::Error::Ok);

    // An empty file, as left by a crash right after truncation, recovers too.
    {
        std::ofstream empty(dir.path + "/counters.dat",
                            std::ios::binary | std::ios::trunc);
    }
    bms::persistence::FileAccountStore third(dir.path);
    REQUIRE(third.status() == bms::Error::Ok);
    bms::AccountNo after = 0;
    REQUIRE(third.nextAccountNo(after) == bms::Error::Ok);
    CHECK(after > last_account);
}

TEST_CASE("Storage fix: saving counters swaps in a complete file and leaves no temp file") {
    Dir dir("counters_tmp");
    bms::persistence::FileAccountStore store(dir.path);
    REQUIRE(store.status() == bms::Error::Ok);
    bms::CustomerId cid = 0;
    REQUIRE(store.nextCustomerId(cid) == bms::Error::Ok);
    CHECK(fs::exists(dir.path + "/counters.dat"));
    CHECK_FALSE(fs::exists(dir.path + "/counters.dat.tmp"));
    CHECK(fs::file_size(dir.path + "/counters.dat") == bms::persistence::kCountersRecordSize);
}

TEST_CASE("Storage fix: a counters file with a bad checksum is rebuilt too") {
    Dir dir("counters_crc");
    {
        bms::persistence::FileAccountStore store(dir.path);
        REQUIRE(store.status() == bms::Error::Ok);
    }
    {
        // Right length, wrong content: the CRC cannot match.
        std::ofstream bad(dir.path + "/counters.dat",
                          std::ios::binary | std::ios::trunc);
        bad << std::string(bms::persistence::kCountersRecordSize, '\x7f');
    }
    bms::persistence::FileAccountStore reopened(dir.path);
    CHECK(reopened.status() == bms::Error::Ok);
    CHECK(fs::file_size(dir.path + "/counters.dat") ==
          bms::persistence::kCountersRecordSize);
}

TEST_CASE("A customers.dat that is not a whole number of records is still refused") {
    Dir dir("short_customers");
    std::error_code ec;
    fs::create_directories(dir.path, ec);
    {
        std::ofstream f(dir.path + "/customers.dat", std::ios::binary);
        f << "12345";
    }
    bms::persistence::FileAccountStore store(dir.path);
    CHECK(store.status() == bms::Error::CorruptRecord);  // real damage is not hidden
}

TEST_CASE("A data folder that cannot be created reports Io") {
    Dir dir("blocked");
    std::error_code ec;
    fs::create_directories(dir.path, ec);
    {
        std::ofstream f(dir.path + "/afile");
        f << "x";
    }
    bms::persistence::FileAccountStore store(dir.path + "/afile/inside");
    CHECK(store.status() == bms::Error::Io);
}

#ifndef _WIN32
TEST_CASE("secureDataDirectory refuses a path that is a file or cannot be created") {
    Dir dir("badpath");
    std::error_code ec;
    fs::create_directories(dir.path, ec);
    {
        std::ofstream f(dir.path + "/afile");
        f << "x";
    }
    std::string reason;
    CHECK_FALSE(bms::platform::secureDataDirectory(dir.path + "/afile", reason));
    CHECK(reason.find("not a folder") != std::string::npos);
    reason.clear();
    CHECK_FALSE(bms::platform::secureDataDirectory(dir.path + "/afile/sub", reason));
    CHECK_FALSE(reason.empty());
}
#endif

TEST_CASE("BMS-F-014: readLineNoEcho reads normally when input is piped") {
    std::istringstream fake("Secret123\nnext\n");
    std::streambuf* saved = std::cin.rdbuf(fake.rdbuf());
    std::string line;
    const bool ok = bms::platform::readLineNoEcho(line);
    std::string second;
    const bool ok2 = bms::platform::readLineNoEcho(second);
    std::string third;
    const bool ok3 = bms::platform::readLineNoEcho(third);  // end of input
    std::cin.rdbuf(saved);
    CHECK(ok);
    CHECK(line == "Secret123");
    CHECK(ok2);
    CHECK(second == "next");
    CHECK_FALSE(ok3);
}

TEST_CASE("replaceFileDurably swaps the file in and fails cleanly when the source is missing") {
    Dir dir("replace");
    std::error_code ec;
    fs::create_directories(dir.path, ec);
    {
        std::ofstream f(dir.path + "/new.tmp");
        f << "new";
    }
    {
        std::ofstream f(dir.path + "/final.dat");
        f << "old";
    }
    CHECK(bms::platform::replaceFileDurably(dir.path + "/new.tmp", dir.path + "/final.dat"));
    CHECK_FALSE(fs::exists(dir.path + "/new.tmp"));
    std::ifstream in(dir.path + "/final.dat");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(content == "new");
    CHECK_FALSE(bms::platform::replaceFileDurably(dir.path + "/gone.tmp", dir.path + "/final.dat"));
}

TEST_CASE("Locked and Closed account states survive a restart") {
    Dir dir("states");
    {
        bms::persistence::FileAccountStore store(dir.path);
        REQUIRE(store.status() == bms::Error::Ok);
        bms::AccountRecord a{};
        a.cust_id = 1;
        a.balance = a.opening_balance = 100;
        a.acct_no = 100001;
        a.state = bms::AccountState::Locked;
        REQUIRE(store.createAccount(a) == bms::Error::Ok);
        a.acct_no = 100002;
        a.state = bms::AccountState::Closed;
        a.type = bms::AccountType::Current;
        a.overdraft_limit = 500;
        a.daily_withdrawn = 7;
        a.daily_transferred = 9;
        a.counter_date = 20261107;
        a.last_seq = 42;
        REQUIRE(store.createAccount(a) == bms::Error::Ok);
    }
    bms::persistence::FileAccountStore reopened(dir.path);
    REQUIRE(reopened.status() == bms::Error::Ok);
    bms::AccountRecord back{};
    REQUIRE(reopened.readAccount(100001, back) == bms::Error::Ok);
    CHECK(back.state == bms::AccountState::Locked);
    REQUIRE(reopened.readAccount(100002, back) == bms::Error::Ok);
    CHECK(back.state == bms::AccountState::Closed);
    CHECK(back.type == bms::AccountType::Current);
    // The daily counters Sprint 3 relies on must round-trip through the file.
    CHECK(back.daily_withdrawn == 7);
    CHECK(back.daily_transferred == 9);
    CHECK(back.counter_date == 20261107);
    CHECK(back.last_seq == 42);
}
