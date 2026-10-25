
#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace bms {

class Money {
private:
    std::int64_t paise_;

public:
    explicit Money(std::int64_t paise = 0) : paise_(paise) {}

    std::int64_t paise() const noexcept {
        return paise_;
    }

    Money operator+(const Money& other) const {
        const auto max = std::numeric_limits<std::int64_t>::max();
        const auto min = std::numeric_limits<std::int64_t>::min();

        if ((other.paise_ > 0 && paise_ > max - other.paise_) ||
            (other.paise_ < 0 && paise_ < min - other.paise_)) {
            throw std::overflow_error("Money addition overflow");
        }

        return Money(paise_ + other.paise_);
    }

    Money operator-(const Money& other) const {
        const auto max = std::numeric_limits<std::int64_t>::max();
        const auto min = std::numeric_limits<std::int64_t>::min();

        if ((other.paise_ < 0 && paise_ > max + other.paise_) ||
            (other.paise_ > 0 && paise_ < min + other.paise_)) {
            throw std::overflow_error("Money subtraction overflow");
        }

        return Money(paise_ - other.paise_);
    }

    bool operator==(const Money& other) const noexcept {
        return paise_ == other.paise_;
    }

    bool operator<(const Money& other) const noexcept {
        return paise_ < other.paise_;
    }
};

}  // namespace bms
