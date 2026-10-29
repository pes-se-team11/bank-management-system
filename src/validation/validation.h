#pragma once
// ValidationModule (Person 1). Checks operator input for type and range
// before any other module uses it (BMS-SR-004). Every function only checks
// or parses; none of them changes state.
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "core/error.h"
#include "core/money.h"

namespace bms::validation {

// Longest values the fixed-width customer record has to hold.
constexpr std::size_t kMaxName = 60;
constexpr std::size_t kMaxAddress = 120;
constexpr std::size_t kMaxContact = 15;
constexpr std::size_t kMaxGovIdRef = 20;
constexpr std::size_t kMaxStaffId = 16;

// Free text: 1..max printable ASCII characters, not all spaces, and no '|'
// (the journal and audit files use '|' as their field separator, so it is
// refused rather than escaped).
Error validateText(std::string_view text, std::size_t max_len);

// 7..15 digits, optionally with one leading '+'.
Error validateContact(std::string_view text);

// 4..20 letters or digits, e.g. a PAN or Aadhaar reference.
Error validateGovIdRef(std::string_view text);

// Staff login ID: 3..16 letters or digits, e.g. "teller1".
Error validateStaffId(std::string_view text);

// Customer PIN: 4..6 digits.
Error validatePin(std::string_view pin);

// Staff password: 8..64 printable characters, no spaces and no '|'.
Error validatePassword(std::string_view password);

// Positive whole number that fits in 32 bits, e.g. an account number.
Error parseId(std::string_view text, std::uint32_t& out);

// Rupee amount such as "1250" or "1250.50" into paise (125050). Digits and
// at most one '.' with one or two decimals; no sign, commas or spaces; at
// most 15 rupee digits, so the result can never overflow int64_t.
// Zero is accepted here - whether zero is allowed is a business rule.
Error parseAmount(std::string_view text, Paise& out);

}  // namespace bms::validation
