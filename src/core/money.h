#pragma once
// Story 7.1 (BMS-NF-003, BMS-SR-003) - owner: Vidit.
// Money is int64_t paise, never float or double. Every add/subtract is
// overflow-checked before it happens and refuses rather than wraps.
#include <cstdint>

namespace bms {
// TODO(7.1): Money type and checked arithmetic.
}  // namespace bms
