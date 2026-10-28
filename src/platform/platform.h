#pragma once
// The ONLY place OS-specific code may live (BMS-NF-004).
// Implementations: platform_posix.cpp (Linux) and platform_win.cpp (MinGW).
//
// Planned:
//   - durable fsync and atomic file replace      (7.2, BMS-NF-002)
//   - owner-only permissions, instance lock      (7.5, BMS-SR-006)
#include <cstddef>
#include <cstdint>
#include <string>

namespace bms::platform {

// Fills buf with len bytes from the operating system's secure random
// source. Used for credential salts (BMS-SR-001). False if the OS fails.
bool secureRandom(std::uint8_t* buf, std::size_t len);

// Reads one line from standard input without echoing it to the terminal
// (BMS-F-014). When input is piped rather than typed, it reads normally,
// so scripted tests still work.
bool readLineNoEcho(std::string& out);

// Overwrites memory in a way the compiler is not allowed to optimise away,
// so a PIN or password does not linger after use (BMS-F-014).
void secureWipe(void* data, std::size_t len);

}  // namespace bms::platform
