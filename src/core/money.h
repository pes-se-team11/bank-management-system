#pragma once
// Story 7.1 (BMS-NF-003, BMS-SR-003) - owner: Vidit.
// Money is int64_t paise, never float or double. Every add/subtract is
// overflow-checked before it happens and refuses rather than wraps.
#include <cstdint>

namespace bms {
// The money type itself (SAD 9.1). Defined early because account opening
// (story 1.2) and amount parsing need it before 7.1 lands.
using Paise = std::int64_t;

// TODO(7.1): checked arithmetic (checkedAdd / checkedSub).
}  // namespace bms
