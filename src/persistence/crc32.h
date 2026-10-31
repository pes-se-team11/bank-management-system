#pragma once

#include <cstdint>
#include <string_view>

namespace bms::persistence {

inline std::uint32_t crc32(std::string_view data) {
    std::uint32_t crc = 0xFFFFFFFFU;

    for (unsigned char byte : data) {
        crc ^= byte;

        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc ^ 0xFFFFFFFFU;
}

}  // namespace bms::persistence
