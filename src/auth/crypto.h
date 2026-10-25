#pragma once
// Hashing used to store credentials (story 7.3, BMS-SR-001).
// SHA-256 (FIPS 180-4), HMAC-SHA256 (RFC 2104) and PBKDF2-HMAC-SHA256
// (RFC 8018), written against the standard library because the SRS rules
// out third-party libraries. Tests check each against published vectors.
#include <array>
#include <cstddef>
#include <cstdint>

namespace bms::crypto {

using Digest = std::array<std::uint8_t, 32>;

Digest sha256(const std::uint8_t* data, std::size_t len);

Digest hmacSha256(const std::uint8_t* key, std::size_t key_len,
                  const std::uint8_t* msg, std::size_t msg_len);

// Derives out_len bytes from a secret and a salt. More iterations make each
// guess slower for an attacker who has stolen the credential file.
void pbkdf2Sha256(const std::uint8_t* secret, std::size_t secret_len,
                  const std::uint8_t* salt, std::size_t salt_len,
                  std::uint32_t iterations, std::uint8_t* out, std::size_t out_len);

// Compares without stopping at the first difference, so the time taken
// does not reveal how much of a guessed hash was right.
bool constantTimeEqual(const std::uint8_t* a, const std::uint8_t* b, std::size_t len);

}  // namespace bms::crypto
