#pragma once
// Append-only file boundary (BMS-SR-005). Story 7.2 adds fsync and the
// write-ahead protocol (BMS-NF-002) without changing these signatures.
#include <string>
#include <vector>

#include "core/error.h"

namespace bms::persistence {

Error appendLine(const std::string& path, const std::string& line);
// Missing files are empty. A final line without its newline is corrupt.
Error readLines(const std::string& path, std::vector<std::string>& out);

}  // namespace bms::persistence
