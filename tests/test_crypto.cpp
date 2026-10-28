#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "auth/crypto.h"

namespace {

const std::uint8_t* raw(const std::string& s) {
    return reinterpret_cast<const std::uint8_t*>(s.data());
}

std::string hex(const std::uint8_t* data, std::size_t len) {
    static const char digits[] = "0123456789abcdef";
    std::string out;
    for (std::size_t i = 0; i < len; ++i) {
        out += digits[data[i] >> 4];
        out += digits[data[i] & 0x0f];
    }
    return out;
}

std::string sha256Hex(const std::string& msg) {
    const bms::crypto::Digest d = bms::crypto::sha256(raw(msg), msg.size());
    return hex(d.data(), d.size());
}

}  // namespace

TEST_CASE("crypto: SHA-256 matches FIPS 180-4 test vectors") {
    CHECK(sha256Hex("") ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha256Hex("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(sha256Hex(std::string(1000000, 'a')) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST_CASE("crypto: HMAC-SHA256 matches RFC 4231 test vectors") {
    const std::string key1(20, '\x0b');
    const std::string msg1 = "Hi There";
    bms::crypto::Digest d =
        bms::crypto::hmacSha256(raw(key1), key1.size(), raw(msg1), msg1.size());
    CHECK(hex(d.data(), d.size()) ==
          "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");

    const std::string key2 = "Jefe";
    const std::string msg2 = "what do ya want for nothing?";
    d = bms::crypto::hmacSha256(raw(key2), key2.size(), raw(msg2), msg2.size());
    CHECK(hex(d.data(), d.size()) ==
          "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");

    // A key longer than one block is hashed first (RFC 4231 test case 6).
    const std::string key6(131, '\xaa');
    const std::string msg6 = "Test Using Larger Than Block-Size Key - Hash Key First";
    d = bms::crypto::hmacSha256(raw(key6), key6.size(), raw(msg6), msg6.size());
    CHECK(hex(d.data(), d.size()) ==
          "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
}

TEST_CASE("crypto: PBKDF2-HMAC-SHA256 matches published test vectors") {
    const std::string pw = "password";
    const std::string salt = "salt";
    std::vector<std::uint8_t> out(32);

    bms::crypto::pbkdf2Sha256(raw(pw), pw.size(), raw(salt), salt.size(), 1,
                              out.data(), out.size());
    CHECK(hex(out.data(), out.size()) ==
          "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");

    bms::crypto::pbkdf2Sha256(raw(pw), pw.size(), raw(salt), salt.size(), 4096,
                              out.data(), out.size());
    CHECK(hex(out.data(), out.size()) ==
          "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");

    // 40 bytes needs a second output block.
    const std::string pw2 = "passwordPASSWORDpassword";
    const std::string salt2 = "saltSALTsaltSALTsaltSALTsaltSALTsalt";
    std::vector<std::uint8_t> out40(40);
    bms::crypto::pbkdf2Sha256(raw(pw2), pw2.size(), raw(salt2), salt2.size(), 4096,
                              out40.data(), out40.size());
    CHECK(hex(out40.data(), out40.size()) ==
          "348c89dbcbd32b2f32d814b8116e84cf2b17347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9");
}

TEST_CASE("crypto: constant-time comparison") {
    const std::uint8_t a[] = {1, 2, 3, 4};
    const std::uint8_t b[] = {1, 2, 3, 4};
    const std::uint8_t c[] = {1, 2, 3, 5};
    CHECK(bms::crypto::constantTimeEqual(a, b, 4));
    CHECK_FALSE(bms::crypto::constantTimeEqual(a, c, 4));
}
