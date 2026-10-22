#pragma once
// Shared result codes so every module reports failures the same way
// (BMS-NF-005). Extend as modules land; never renumber existing values.

namespace bms {
enum class Error {
    Ok = 0,
};
}  // namespace bms
