#include <doctest/doctest.h>

#include <string>

#include "validation/validation.h"

using bms::Error;
namespace v = bms::validation;

TEST_CASE("validation: free text") {
    CHECK(v::validateText("Asha Rao", v::kMaxName) == Error::Ok);
    CHECK(v::validateText("", v::kMaxName) == Error::InvalidInput);
    CHECK(v::validateText("   ", v::kMaxName) == Error::InvalidInput);
    CHECK(v::validateText(std::string(v::kMaxName + 1, 'a'), v::kMaxName) ==
          Error::InvalidInput);
    // '|' separates fields in the journal and audit files.
    CHECK(v::validateText("Asha|Rao", v::kMaxName) == Error::InvalidInput);
    CHECK(v::validateText("Asha\nRao", v::kMaxName) == Error::InvalidInput);
}

TEST_CASE("validation: contact number and ID reference") {
    CHECK(v::validateContact("9876543210") == Error::Ok);
    CHECK(v::validateContact("+919876543210") == Error::Ok);
    CHECK(v::validateContact("98765") == Error::InvalidInput);
    CHECK(v::validateContact("98765-43210") == Error::InvalidInput);
    CHECK(v::validateGovIdRef("ABCDE1234F") == Error::Ok);
    CHECK(v::validateGovIdRef("AB C") == Error::InvalidInput);
}

TEST_CASE("validation: PIN and password rules") {
    CHECK(v::validatePin("4821") == Error::Ok);
    CHECK(v::validatePin("482") == Error::InvalidInput);
    CHECK(v::validatePin("4821000") == Error::InvalidInput);
    CHECK(v::validatePin("48a1") == Error::InvalidInput);
    CHECK(v::validatePassword("teller-pass1") == Error::Ok);
    CHECK(v::validatePassword("short1") == Error::InvalidInput);
    CHECK(v::validatePassword("has space1") == Error::InvalidInput);
}

TEST_CASE("validation: account numbers") {
    std::uint32_t id = 0;
    CHECK(v::parseId("100001", id) == Error::Ok);
    CHECK(id == 100001U);
    CHECK(v::parseId("4294967295", id) == Error::Ok);
    CHECK(v::parseId("4294967296", id) == Error::InvalidInput);  // above 32 bits
    CHECK(v::parseId("0", id) == Error::InvalidInput);
    CHECK(v::parseId("12a", id) == Error::InvalidInput);
    CHECK(v::parseId("", id) == Error::InvalidInput);
}

TEST_CASE("validation: rupee amounts become paise") {
    bms::Paise p = -1;
    CHECK(v::parseAmount("1250", p) == Error::Ok);
    CHECK(p == 125000);
    CHECK(v::parseAmount("1250.5", p) == Error::Ok);
    CHECK(p == 125050);
    CHECK(v::parseAmount("1250.05", p) == Error::Ok);
    CHECK(p == 125005);
    CHECK(v::parseAmount("0", p) == Error::Ok);
    CHECK(p == 0);
    CHECK(v::parseAmount("999999999999999.99", p) == Error::Ok);  // largest allowed
    CHECK(p == 99999999999999999);

    for (const char* bad : {"", "abc", "12a", "-5", "+5", "1,250", "12.", ".5",
                            "1.234", "1 000", "1e5", "1000000000000000"}) {
        CAPTURE(bad);
        CHECK(v::parseAmount(bad, p) == Error::InvalidInput);
    }
}
