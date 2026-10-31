#pragma once
// File-backed CredentialStore implementation (SAD 9.3, BMS-SR-001).
// Persists salted credential hashes to credentials.dat in a specified directory.

#include <string>
#include <cstdint>
#include <unordered_map>

#include "auth/auth.h"
#include "core/error.h"

namespace bms::persistence {

class FileCredentialStore final : public CredentialStore {
public:
    explicit FileCredentialStore(std::string data_dir);

    // Returns the storage initialization status.
    Error status() const { return init_status_; }

    Error readCredential(const Principal& who, Credential& out) override;
    Error writeCredential(const Credential& credential) override;

    // True once any staff login exists (used for initial setup in main).
    bool hasStaff() const;

private:
    static std::string makeKey(const Principal& who);
    Error scanExistingFiles();

    std::string data_dir_;
    std::string credentials_path_;
    std::unordered_map<std::string, std::uint64_t> credential_offsets_;
    Error init_status_ = Error::Ok;
};

}  // namespace bms::persistence