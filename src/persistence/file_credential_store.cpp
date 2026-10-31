
#include "persistence/file_credential_store.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <utility>

#include "persistence/record_types.h"

namespace bms::persistence {

namespace {

bool validPrincipalKind(PrincipalKind kind) {
    return kind == PrincipalKind::Customer ||
           kind == PrincipalKind::Staff;
}

bool validRole(Role role) {
    return role == Role::Customer ||
           role == Role::Teller ||
           role == Role::Manager;
}

bool roleMatchesPrincipal(PrincipalKind kind, Role role) {
    return kind == PrincipalKind::Customer
               ? role == Role::Customer
               : role == Role::Teller || role == Role::Manager;
}

bool validId(const std::string& id) {
    return !id.empty() &&
           id.size() <= 31 &&
           id.find('\0') == std::string::npos;
}

Error validateCredentialRecord(
    const std::array<std::uint8_t, kCredentialRecordSize>& buf,
    Credential& credential) {

    // Reject unknown enum values instead of interpreting them as valid ones.
    if (readUint8(buf.data()) > 1 ||
        readUint8(buf.data() + 33) > 2) {
        return Error::CorruptRecord;
    }

    const Error err = deserializeCredential(buf, credential);
    if (err != Error::Ok) {
        return err;
    }

    if (!validId(credential.principal.id)) {
        return Error::CorruptRecord;
    }

    if (!roleMatchesPrincipal(credential.principal.kind, credential.role) ||
        credential.iterations == 0) {
        return Error::CorruptRecord;
    }

    return Error::Ok;
}

}  // namespace

std::string FileCredentialStore::makeKey(const Principal& who) {
    return (who.kind == PrincipalKind::Customer ? "C:" : "S:") + who.id;
}

FileCredentialStore::FileCredentialStore(std::string data_dir)
    : data_dir_(std::move(data_dir)),
      credentials_path_(data_dir_ + "/credentials.dat") {

    std::error_code ec;
    std::filesystem::create_directories(data_dir_, ec);

    if (ec) {
        init_status_ = Error::Io;
        return;
    }

    init_status_ = scanExistingFiles();
}

Error FileCredentialStore::scanExistingFiles() {
    credential_offsets_.clear();

    std::error_code ec;
    const bool exists =
        std::filesystem::exists(credentials_path_, ec);

    if (ec) {
        return Error::Io;
    }

    if (!exists) {
        return Error::Ok;
    }

    const std::uintmax_t file_size =
        std::filesystem::file_size(credentials_path_, ec);

    if (ec) {
        return Error::Io;
    }

    // Every credential must occupy exactly one complete record.
    if (file_size % kCredentialRecordSize != 0) {
        return Error::CorruptRecord;
    }

    if (file_size >
        static_cast<std::uintmax_t>(
            std::numeric_limits<std::streamoff>::max())) {
        return Error::Io;
    }

    std::ifstream file(
        credentials_path_,
        std::ios::in | std::ios::binary
    );

    if (!file) {
        return Error::Io;
    }

    std::array<std::uint8_t, kCredentialRecordSize> buf{};

    std::uint64_t offset = 0;

    while (offset < file_size) {
        file.read(
            reinterpret_cast<char*>(buf.data()),
            static_cast<std::streamsize>(buf.size())
        );

        if (!file) {
            return Error::Io;
        }

        Credential credential;
        const Error err = validateCredentialRecord(buf, credential);

        if (err != Error::Ok) {
            return err;
        }

        const std::string key = makeKey(credential.principal);

        // Duplicate principal records are treated as corruption.
        if (credential_offsets_.find(key) != credential_offsets_.end()) {
            return Error::CorruptRecord;
        }

        credential_offsets_.emplace(key, offset);
        offset += kCredentialRecordSize;
    }

    if (file.bad()) {
        return Error::Io;
    }

    return Error::Ok;
}

Error FileCredentialStore::readCredential(
    const Principal& who,
    Credential& out) {

    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    if (!validPrincipalKind(who.kind) || !validId(who.id)) {
        return Error::InvalidInput;
    }

    const auto it = credential_offsets_.find(makeKey(who));

    if (it == credential_offsets_.end()) {
        return Error::CredentialNotFound;
    }

    std::ifstream file(
        credentials_path_,
        std::ios::in | std::ios::binary
    );

    if (!file) {
        return Error::Io;
    }

    file.seekg(static_cast<std::streamoff>(it->second));

    if (!file) {
        return Error::Io;
    }

    std::array<std::uint8_t, kCredentialRecordSize> buf{};

    file.read(
        reinterpret_cast<char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    if (!file) {
        return Error::Io;
    }

    Credential credential;
    const Error err = validateCredentialRecord(buf, credential);

    if (err != Error::Ok) {
        return err;
    }

    if (credential.principal.kind != who.kind ||
        credential.principal.id != who.id) {
        return Error::CorruptRecord;
    }

    out = credential;
    return Error::Ok;
}

Error FileCredentialStore::writeCredential(
    const Credential& credential) {

    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    if (!validPrincipalKind(credential.principal.kind) ||
        !validRole(credential.role) ||
        !roleMatchesPrincipal(credential.principal.kind, credential.role) ||
        !validId(credential.principal.id) ||
        credential.iterations == 0) {
        return Error::InvalidInput;
    }

    std::array<std::uint8_t, kCredentialRecordSize> buf{};
    serializeCredential(credential, buf);

    const std::string key = makeKey(credential.principal);
    const auto it = credential_offsets_.find(key);

    if (it != credential_offsets_.end()) {
        std::fstream file(
            credentials_path_,
            std::ios::in | std::ios::out | std::ios::binary
        );

        if (!file) {
            return Error::Io;
        }

        file.seekp(static_cast<std::streamoff>(it->second));

        if (!file) {
            return Error::Io;
        }

        file.write(
            reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size())
        );

        file.flush();

        if (!file) {
            init_status_ = Error::Io;
            return Error::Io;
        }

        return Error::Ok;
    }

    std::ofstream file(
        credentials_path_,
        std::ios::out | std::ios::binary | std::ios::app
    );

    if (!file) {
        return Error::Io;
    }

    const std::streampos end_position = file.tellp();

    if (end_position == std::streampos(-1)) {
        init_status_ = Error::Io;
        return Error::Io;
    }

    const std::streamoff stream_offset =
        static_cast<std::streamoff>(end_position);

    if (stream_offset < 0) {
        init_status_ = Error::Io;
        return Error::Io;
    }

    const std::uint64_t offset =
        static_cast<std::uint64_t>(stream_offset);

    file.write(
        reinterpret_cast<const char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    file.flush();

    if (!file) {
        // Prevent further writes after an incomplete append.
        init_status_ = Error::Io;
        return Error::Io;
    }

    credential_offsets_.emplace(key, offset);

    return Error::Ok;
}

bool FileCredentialStore::hasStaff() const {
    for (const auto& entry : credential_offsets_) {
        if (entry.first.rfind("S:", 0) == 0) {
            return true;
        }
    }

    return false;
}

}  // namespace bms::persistence
