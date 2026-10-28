#include "auth/crypto.h"

#include <algorithm>
#include <vector>

namespace bms::crypto {

namespace {

// First 32 bits of the fractional parts of the cube roots of the first 64
// primes (FIPS 180-4, section 4.2.2).
constexpr std::array<std::uint32_t, 64> kRound = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::uint32_t rotr(std::uint32_t x, unsigned n) { return (x >> n) | (x << (32U - n)); }

class Sha256 {
public:
    void update(const std::uint8_t* data, std::size_t len) {
        for (std::size_t i = 0; i < len; ++i) {
            block_[used_++] = data[i];
            if (used_ == block_.size()) {
                compress();
                used_ = 0;
            }
        }
        total_bytes_ += len;
    }

    Digest finish() {
        const std::uint64_t total_bits = total_bytes_ * 8U;
        const std::uint8_t one_bit = 0x80;
        const std::uint8_t zero = 0x00;
        update(&one_bit, 1);
        while (used_ != 56) {  // leave 8 bytes in the block for the length
            update(&zero, 1);
        }
        std::uint8_t length[8];
        for (std::size_t i = 0; i < 8; ++i) {
            length[i] = static_cast<std::uint8_t>(total_bits >> (56U - 8U * i));
        }
        update(length, 8);

        Digest out{};
        for (std::size_t i = 0; i < 8; ++i) {
            out[4 * i] = static_cast<std::uint8_t>(state_[i] >> 24);
            out[4 * i + 1] = static_cast<std::uint8_t>(state_[i] >> 16);
            out[4 * i + 2] = static_cast<std::uint8_t>(state_[i] >> 8);
            out[4 * i + 3] = static_cast<std::uint8_t>(state_[i]);
        }
        return out;
    }

private:
    void compress() {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(block_[4 * i]) << 24) |
                   (static_cast<std::uint32_t>(block_[4 * i + 1]) << 16) |
                   (static_cast<std::uint32_t>(block_[4 * i + 2]) << 8) |
                   static_cast<std::uint32_t>(block_[4 * i + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 =
                rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 =
                rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t sum1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t choose = (e & f) ^ (~e & g);
            const std::uint32_t t1 = h + sum1 + choose + kRound[i] + w[i];
            const std::uint32_t sum0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    // Initial hash value (FIPS 180-4, section 5.3.3).
    std::array<std::uint32_t, 8> state_ = {0x6a09e667, 0xbb67ae85, 0x3c6ef372,
                                           0xa54ff53a, 0x510e527f, 0x9b05688c,
                                           0x1f83d9ab, 0x5be0cd19};
    std::array<std::uint8_t, 64> block_{};
    std::size_t used_ = 0;
    std::uint64_t total_bytes_ = 0;
};

}  // namespace

Digest sha256(const std::uint8_t* data, std::size_t len) {
    Sha256 h;
    h.update(data, len);
    return h.finish();
}

Digest hmacSha256(const std::uint8_t* key, std::size_t key_len,
                  const std::uint8_t* msg, std::size_t msg_len) {
    // Keys longer than one block are hashed first; shorter ones are padded.
    std::array<std::uint8_t, 64> padded_key{};
    if (key_len > padded_key.size()) {
        const Digest hashed = sha256(key, key_len);
        std::copy(hashed.begin(), hashed.end(), padded_key.begin());
    } else {
        std::copy(key, key + key_len, padded_key.begin());
    }

    std::array<std::uint8_t, 64> inner_pad{};
    std::array<std::uint8_t, 64> outer_pad{};
    for (std::size_t i = 0; i < padded_key.size(); ++i) {
        inner_pad[i] = static_cast<std::uint8_t>(padded_key[i] ^ 0x36);
        outer_pad[i] = static_cast<std::uint8_t>(padded_key[i] ^ 0x5c);
    }

    Sha256 inner;
    inner.update(inner_pad.data(), inner_pad.size());
    inner.update(msg, msg_len);
    const Digest inner_hash = inner.finish();

    Sha256 outer;
    outer.update(outer_pad.data(), outer_pad.size());
    outer.update(inner_hash.data(), inner_hash.size());
    return outer.finish();
}

void pbkdf2Sha256(const std::uint8_t* secret, std::size_t secret_len,
                  const std::uint8_t* salt, std::size_t salt_len,
                  std::uint32_t iterations, std::uint8_t* out, std::size_t out_len) {
    // Each output block i is U1 ^ U2 ^ ... ^ Uc, where U1 = HMAC(secret,
    // salt || i as 4 big-endian bytes) and each later U = HMAC(secret, previous U).
    std::vector<std::uint8_t> salted(salt, salt + salt_len);
    salted.resize(salt_len + 4);

    std::uint32_t block_index = 1;
    std::size_t written = 0;
    while (written < out_len) {
        salted[salt_len] = static_cast<std::uint8_t>(block_index >> 24);
        salted[salt_len + 1] = static_cast<std::uint8_t>(block_index >> 16);
        salted[salt_len + 2] = static_cast<std::uint8_t>(block_index >> 8);
        salted[salt_len + 3] = static_cast<std::uint8_t>(block_index);

        Digest u = hmacSha256(secret, secret_len, salted.data(), salted.size());
        Digest block = u;
        for (std::uint32_t round = 1; round < iterations; ++round) {
            u = hmacSha256(secret, secret_len, u.data(), u.size());
            for (std::size_t b = 0; b < block.size(); ++b) {
                block[b] = static_cast<std::uint8_t>(block[b] ^ u[b]);
            }
        }

        const std::size_t take = std::min(block.size(), out_len - written);
        std::copy_n(block.begin(), take, out + written);
        written += take;
        ++block_index;
    }
}

bool constantTimeEqual(const std::uint8_t* a, const std::uint8_t* b, std::size_t len) {
    std::uint8_t difference = 0;
    for (std::size_t i = 0; i < len; ++i) {
        difference = static_cast<std::uint8_t>(difference | (a[i] ^ b[i]));
    }
    return difference == 0;
}

}  // namespace bms::crypto
