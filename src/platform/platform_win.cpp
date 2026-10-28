// Windows implementation of the platform layer (BMS-NF-004).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>

#include <iostream>
#include <limits>

#include "platform/platform.h"

namespace bms::platform {

bool secureRandom(std::uint8_t* buf, std::size_t len) {
    // BCryptGenRandom takes a 32-bit length; salts are far smaller.
    if (len > std::numeric_limits<ULONG>::max()) {
        return false;
    }
    const NTSTATUS status = BCryptGenRandom(
        nullptr, buf, static_cast<ULONG>(len), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return BCRYPT_SUCCESS(status);
}

bool readLineNoEcho(std::string& out) {
    const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD saved = 0;
    const bool console =
        in != INVALID_HANDLE_VALUE && GetConsoleMode(in, &saved) != 0;
    if (console) {
        SetConsoleMode(in, saved & ~static_cast<DWORD>(ENABLE_ECHO_INPUT));
    }
    const bool ok = static_cast<bool>(std::getline(std::cin, out));
    if (console) {
        SetConsoleMode(in, saved);
        std::cout << '\n';  // the user's Enter key was not echoed either
    }
    return ok;
}

void secureWipe(void* data, std::size_t len) {
    if (data != nullptr && len != 0) {
        SecureZeroMemory(data, len);
    }
}

}  // namespace bms::platform
