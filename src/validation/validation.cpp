#include "validation/validation.h"

#include <limits>

namespace bms::validation {

namespace {

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isAlnum(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// Printable ASCII, space included.
bool isPrintable(char c) { return c >= ' ' && c <= '~'; }

bool allDigits(std::string_view text) {
    for (const char c : text) {
        if (!isDigit(c)) {
            return false;
        }
    }
    return true;
}

}  // namespace

Error validateText(std::string_view text, std::size_t max_len) {
    if (text.empty() || text.size() > max_len) {
        return Error::InvalidInput;
    }
    bool has_visible = false;
    for (const char c : text) {
        if (!isPrintable(c) || c == '|') {
            return Error::InvalidInput;
        }
        if (c != ' ') {
            has_visible = true;
        }
    }
    return has_visible ? Error::Ok : Error::InvalidInput;
}

Error validateContact(std::string_view text) {
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
    }
    if (text.size() < 7 || text.size() > kMaxContact || !allDigits(text)) {
        return Error::InvalidInput;
    }
    return Error::Ok;
}

Error validateGovIdRef(std::string_view text) {
    if (text.size() < 4 || text.size() > kMaxGovIdRef) {
        return Error::InvalidInput;
    }
    for (const char c : text) {
        if (!isAlnum(c)) {
            return Error::InvalidInput;
        }
    }
    return Error::Ok;
}

Error validatePin(std::string_view pin) {
    if (pin.size() < 4 || pin.size() > 6 || !allDigits(pin)) {
        return Error::InvalidInput;
    }
    return Error::Ok;
}

Error validatePassword(std::string_view password) {
    if (password.size() < 8 || password.size() > 64) {
        return Error::InvalidInput;
    }
    for (const char c : password) {
        if (!isPrintable(c) || c == ' ' || c == '|') {
            return Error::InvalidInput;
        }
    }
    return Error::Ok;
}

Error parseId(std::string_view text, std::uint32_t& out) {
    if (text.empty() || text.size() > 10 || !allDigits(text)) {
        return Error::InvalidInput;
    }
    std::uint64_t value = 0;  // 10 digits always fit in 64 bits
    for (const char c : text) {
        value = value * 10 + static_cast<std::uint64_t>(c - '0');
    }
    if (value == 0 || value > std::numeric_limits<std::uint32_t>::max()) {
        return Error::InvalidInput;
    }
    out = static_cast<std::uint32_t>(value);
    return Error::Ok;
}

Error parseAmount(std::string_view text, Paise& out) {
    const std::size_t dot = text.find('.');
    const std::string_view rupees = text.substr(0, dot);
    const std::string_view decimals =
        dot == std::string_view::npos ? std::string_view{} : text.substr(dot + 1);

    if (rupees.empty() || rupees.size() > 15 || !allDigits(rupees)) {
        return Error::InvalidInput;
    }
    // "12." is refused: a dot must be followed by one or two digits.
    if (dot != std::string_view::npos &&
        (decimals.empty() || decimals.size() > 2 || !allDigits(decimals))) {
        return Error::InvalidInput;
    }

    Paise value = 0;
    for (const char c : rupees) {
        value = value * 10 + (c - '0');
    }
    Paise paise = 0;
    if (!decimals.empty()) {
        paise = (decimals[0] - '0') * 10;  // "5" means 50 paise
        if (decimals.size() == 2) {
            paise += decimals[1] - '0';
        }
    }
    out = value * 100 + paise;
    return Error::Ok;
}

}  // namespace bms::validation
