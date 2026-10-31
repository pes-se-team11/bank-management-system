
#pragma once
// Binary on-disk record serialization and integrity definitions (SAD 9.4).
// Uses explicit little-endian byte encoding so layouts are completely portable,
// free of compiler-specific padding, and warning-free under -pedantic.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "account/account.h"
#include "auth/auth.h"
#include "core/error.h"
#include "persistence/crc32.h"

namespace bms::persistence {

// -----------------------------------------------------------------------------
// Byte encoding / decoding helpers (Little-Endian)
// -----------------------------------------------------------------------------

inline void writeUint8(std::uint8_t* dst, std::uint8_t val) {
    dst[0] = val;
}

inline std::uint8_t readUint8(const std::uint8_t* src) {
    return src[0];
}

inline void writeUint32LE(std::uint8_t* dst, std::uint32_t val) {
    dst[0] = static_cast<std::uint8_t>(val & 0xFFU);
    dst[1] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    dst[2] = static_cast<std::uint8_t>((val >> 16U) & 0xFFU);
    dst[3] = static_cast<std::uint8_t>((val >> 24U) & 0xFFU);
}

inline std::uint32_t readUint32LE(const std::uint8_t* src) {
    return static_cast<std::uint32_t>(src[0]) |
           (static_cast<std::uint32_t>(src[1]) << 8U) |
           (static_cast<std::uint32_t>(src[2]) << 16U) |
           (static_cast<std::uint32_t>(src[3]) << 24U);
}

inline void writeUint64LE(std::uint8_t* dst, std::uint64_t val) {
    for (std::size_t i = 0; i < 8; ++i) {
        dst[i] = static_cast<std::uint8_t>((val >> (i * 8U)) & 0xFFU);
    }
}

inline std::uint64_t readUint64LE(const std::uint8_t* src) {
    std::uint64_t val = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        val |= (static_cast<std::uint64_t>(src[i]) << (i * 8U));
    }
    return val;
}

inline void writeInt64LE(std::uint8_t* dst, std::int64_t val) {
    writeUint64LE(dst, static_cast<std::uint64_t>(val));
}

inline std::int64_t readInt64LE(const std::uint8_t* src) {
    return static_cast<std::int64_t>(readUint64LE(src));
}

inline void writeFixedString(
    std::uint8_t* dst,
    std::size_t max_len,
    std::string_view text
) {
    std::memset(dst, 0, max_len);
    const std::size_t copy_len =
        text.size() < max_len ? text.size() : max_len;

    if (copy_len > 0) {
        std::memcpy(dst, text.data(), copy_len);
    }
}

inline std::string readFixedString(
    const std::uint8_t* src,
    std::size_t max_len
) {
    std::size_t len = 0;

    while (len < max_len && src[len] != '\0') {
        ++len;
    }

    return std::string(reinterpret_cast<const char*>(src), len);
}

// -----------------------------------------------------------------------------
// Account Record Format: accounts.dat (66 bytes, SAD 9.4)
// Layout:
//   acct_no (4), cust_id (4), type (1), state (1), opening_balance (8),
//   balance (8), overdraft_limit (8), daily_withdrawn (8),
//   daily_transferred (8), counter_date (4), last_seq (8), crc32 (4)
// -----------------------------------------------------------------------------

constexpr std::size_t kAccountRecordSize = 66;

inline Error serializeAccount(
    const AccountRecord& record,
    std::array<std::uint8_t, kAccountRecordSize>& buf
) {
    // Validate account type and state before serialization.
    if (
        (record.type != AccountType::Savings &&
         record.type != AccountType::Current) ||
        (record.state != AccountState::Active &&
         record.state != AccountState::Locked &&
         record.state != AccountState::Closed)
    ) {
        return Error::InvalidInput;
    }

    std::uint8_t* p = buf.data();

    writeUint32LE(p, record.acct_no);
    p += 4;

    writeUint32LE(p, record.cust_id);
    p += 4;

    // Account type: Savings = 0, Current = 1
    writeUint8(
        p,
        static_cast<std::uint8_t>(
            record.type == AccountType::Savings ? 0 : 1
        )
    );
    p += 1;

    // Account state: Active = 0, Locked = 1, Closed = 2
    writeUint8(
        p,
        static_cast<std::uint8_t>(
            record.state == AccountState::Active ? 0 :
            record.state == AccountState::Locked ? 1 : 2
        )
    );
    p += 1;

    writeInt64LE(p, record.opening_balance);
    p += 8;

    writeInt64LE(p, record.balance);
    p += 8;

    writeInt64LE(p, record.overdraft_limit);
    p += 8;

    writeInt64LE(p, record.daily_withdrawn);
    p += 8;

    writeInt64LE(p, record.daily_transferred);
    p += 8;

    writeUint32LE(p, record.counter_date);
    p += 4;

    writeUint64LE(p, record.last_seq);
    p += 8;

    // Calculate and write CRC32.
    const std::uint32_t crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kAccountRecordSize - 4
    ));

    writeUint32LE(p, crc);

    return Error::Ok;
}

inline Error deserializeAccount(
    const std::array<std::uint8_t, kAccountRecordSize>& buf,
    AccountRecord& out
) {
    const std::uint32_t expected_crc =
        readUint32LE(buf.data() + kAccountRecordSize - 4);

    const std::uint32_t actual_crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kAccountRecordSize - 4
    ));

    if (expected_crc != actual_crc) {
        return Error::CorruptRecord;
    }

    const std::uint8_t* p = buf.data();

    out.acct_no = readUint32LE(p); p += 4;
    out.cust_id = readUint32LE(p); p += 4;

    const std::uint8_t type_val = readUint8(p); p += 1;
    const std::uint8_t state_val = readUint8(p); p += 1;

    if (type_val > 1 || state_val > 2) {
        return Error::CorruptRecord;
    }

    out.type = (type_val == 0)
        ? AccountType::Savings
        : AccountType::Current;

    out.state = (state_val == 0)
        ? AccountState::Active
        : (state_val == 1)
            ? AccountState::Locked
            : AccountState::Closed;

    out.opening_balance = readInt64LE(p); p += 8;
    out.balance = readInt64LE(p); p += 8;
    out.overdraft_limit = readInt64LE(p); p += 8;
    out.daily_withdrawn = readInt64LE(p); p += 8;
    out.daily_transferred = readInt64LE(p); p += 8;
    out.counter_date = readUint32LE(p); p += 4;
    out.last_seq = readUint64LE(p);

    return Error::Ok;
}

// -----------------------------------------------------------------------------
// Customer Record Format: customers.dat (224 bytes, SAD 9.4, DS-ACC-01)
// Layout:
//   id (4), name (60), address (120), contact (16), gov_id_ref (20), crc32 (4)
// -----------------------------------------------------------------------------

constexpr std::size_t kCustomerRecordSize = 224;

inline void serializeCustomer(
    const Customer& record,
    std::array<std::uint8_t, kCustomerRecordSize>& buf
) {
    std::uint8_t* p = buf.data();

    writeUint32LE(p, record.id); p += 4;
    writeFixedString(p, 60, record.name); p += 60;
    writeFixedString(p, 120, record.address); p += 120;
    writeFixedString(p, 16, record.contact); p += 16;
    writeFixedString(p, 20, record.gov_id_ref); p += 20;

    const std::uint32_t crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCustomerRecordSize - 4
    ));

    writeUint32LE(p, crc);
}

inline Error deserializeCustomer(
    const std::array<std::uint8_t, kCustomerRecordSize>& buf,
    Customer& out
) {
    const std::uint32_t expected_crc =
        readUint32LE(buf.data() + kCustomerRecordSize - 4);

    const std::uint32_t actual_crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCustomerRecordSize - 4
    ));

    if (expected_crc != actual_crc) {
        return Error::CorruptRecord;
    }

    const std::uint8_t* p = buf.data();

    out.id = readUint32LE(p); p += 4;
    out.name = readFixedString(p, 60); p += 60;
    out.address = readFixedString(p, 120); p += 120;
    out.contact = readFixedString(p, 16); p += 16;
    out.gov_id_ref = readFixedString(p, 20);

    return Error::Ok;
}

// -----------------------------------------------------------------------------
// Credential Record Format: credentials.dat (94 bytes, BMS-SR-001)
// Layout:
//   kind (1), id (32), role (1), salt (16), hash (32), iterations (4),
//   failed_attempts (4), crc32 (4)
// -----------------------------------------------------------------------------

constexpr std::size_t kCredentialRecordSize = 94;

inline void serializeCredential(
    const Credential& record,
    std::array<std::uint8_t, kCredentialRecordSize>& buf
) {
    std::uint8_t* p = buf.data();

    writeUint8(
        p,
        static_cast<std::uint8_t>(
            record.principal.kind == PrincipalKind::Customer ? 0 : 1
        )
    );
    p += 1;

    writeFixedString(p, 32, record.principal.id); p += 32;

    writeUint8(
        p,
        static_cast<std::uint8_t>(
            record.role == Role::Customer ? 0 :
            record.role == Role::Teller ? 1 : 2
        )
    );
    p += 1;

    std::memcpy(p, record.salt.data(), 16); p += 16;
    std::memcpy(p, record.hash.data(), 32); p += 32;

    writeUint32LE(p, record.iterations); p += 4;
    writeUint32LE(p, record.failed_attempts); p += 4;

    const std::uint32_t crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCredentialRecordSize - 4
    ));

    writeUint32LE(p, crc);
}

inline Error deserializeCredential(
    const std::array<std::uint8_t, kCredentialRecordSize>& buf,
    Credential& out
) {
    const std::uint32_t expected_crc =
        readUint32LE(buf.data() + kCredentialRecordSize - 4);

    const std::uint32_t actual_crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCredentialRecordSize - 4
    ));

    if (expected_crc != actual_crc) {
        return Error::CorruptRecord;
    }

    const std::uint8_t* p = buf.data();

    const std::uint8_t kind_val = readUint8(p); p += 1;
    const std::uint8_t role_val = buf[33];

    if (kind_val > 1 || role_val > 2) {
        return Error::CorruptRecord;
    }

    out.principal.kind = (kind_val == 0)
        ? PrincipalKind::Customer
        : PrincipalKind::Staff;

    out.principal.id = readFixedString(p, 32); p += 32;

    p += 1;

    out.role = (role_val == 0)
        ? Role::Customer
        : (role_val == 1)
            ? Role::Teller
            : Role::Manager;

    std::memcpy(out.salt.data(), p, 16); p += 16;
    std::memcpy(out.hash.data(), p, 32); p += 32;

    out.iterations = readUint32LE(p); p += 4;
    out.failed_attempts = readUint32LE(p);

    return Error::Ok;
}

// -----------------------------------------------------------------------------
// Counters Record Format: counters.dat (12 bytes)
// Layout:
//   next_customer_id (4), next_account_no (4), crc32 (4)
// -----------------------------------------------------------------------------

constexpr std::size_t kCountersRecordSize = 12;

inline void serializeCounters(
    CustomerId next_customer,
    AccountNo next_account,
    std::array<std::uint8_t, kCountersRecordSize>& buf
) {
    std::uint8_t* p = buf.data();

    writeUint32LE(p, next_customer); p += 4;
    writeUint32LE(p, next_account); p += 4;

    const std::uint32_t crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCountersRecordSize - 4
    ));

    writeUint32LE(p, crc);
}

inline Error deserializeCounters(
    const std::array<std::uint8_t, kCountersRecordSize>& buf,
    CustomerId& next_customer,
    AccountNo& next_account
) {
    const std::uint32_t expected_crc =
        readUint32LE(buf.data() + kCountersRecordSize - 4);

    const std::uint32_t actual_crc = crc32(std::string_view(
        reinterpret_cast<const char*>(buf.data()),
        kCountersRecordSize - 4
    ));

    if (expected_crc != actual_crc) {
        return Error::CorruptRecord;
    }

    const std::uint8_t* p = buf.data();

    next_customer = readUint32LE(p); p += 4;
    next_account = readUint32LE(p);

    return Error::Ok;
}

}  // namespace bms::persistence
