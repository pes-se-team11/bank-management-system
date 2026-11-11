#pragma once
// The ONLY place OS-specific code may live (BMS-NF-004).
// Implementations: platform_posix.cpp (Linux) and platform_win.cpp (MinGW).
//
// Implemented here:
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

// BMS-SR-006: makes the data directory ready for use. Creates it owner-only
// when missing, makes every file the process creates afterwards owner-only
// (umask 077 on Linux), and refuses a directory that group or others can
// write to. On refusal `reason` says what to do about it. On Windows the
// per-user profile ACL is relied on and only creation is performed.
bool secureDataDirectory(const std::string& dir, std::string& reason);

// BMS-SR-006: keeps a second copy of the program from opening the same data
// directory. The lock is released when the object is destroyed or the
// process dies, so a crash never leaves a stale lock behind.
class InstanceLock {
public:
    InstanceLock() = default;
    InstanceLock(const InstanceLock&) = delete;
    InstanceLock& operator=(const InstanceLock&) = delete;
    ~InstanceLock();

    // True when this object now holds the lock; false when another
    // process holds it or the lock file cannot be opened.
    bool acquire(const std::string& path);

private:
    std::intptr_t handle_ = -1;
};

// BMS-NF-002: flushes `tmp` to disk, then atomically renames it over `dest`,
// so a crash leaves either the old complete file or the new complete one,
// never a half-written one.
bool replaceFileDurably(const std::string& tmp, const std::string& dest);

}  // namespace bms::platform
