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

bool secureDataDirectory(const std::string& dir, std::string& reason) {
    // Windows has no group/other write bits to inspect. New folders under the
    // user's profile inherit an owner-only ACL, so creation is all we do.
    const DWORD attrs = GetFileAttributesA(dir.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(dir.c_str(), nullptr) &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            reason = "Could not create the data folder '" + dir + "'.";
            return false;
        }
        return true;
    }
    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        reason = "'" + dir + "' exists but is not a folder.";
        return false;
    }
    return true;
}

InstanceLock::~InstanceLock() {
    if (handle_ != -1) {
        CloseHandle(reinterpret_cast<HANDLE>(handle_));
    }
}

bool InstanceLock::acquire(const std::string& path) {
    if (handle_ != -1) {
        return true;
    }
    // Share mode 0: a second open of the same file fails while we hold it.
    const HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                 nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                 nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    handle_ = reinterpret_cast<std::intptr_t>(h);
    return true;
}

bool replaceFileDurably(const std::string& tmp, const std::string& dest) {
    return MoveFileExA(tmp.c_str(), dest.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

}  // namespace bms::platform
