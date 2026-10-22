#pragma once
// The ONLY place OS-specific code may live (BMS-NF-004).
// Implementations: platform_posix.cpp (Linux) and platform_win.cpp (MinGW).
//
// Planned:
//   - no-echo credential entry and buffer wipe   (7.3, BMS-F-014)
//   - cryptographically secure random bytes      (7.3, BMS-SR-001)
//   - durable fsync and atomic file replace      (7.2, BMS-NF-002)
//   - owner-only permissions, instance lock      (7.5, BMS-SR-006)

namespace bms::platform {
}  // namespace bms::platform
