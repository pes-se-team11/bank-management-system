
#include <doctest/doctest.h>

#include "core/money.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

using bms::Money;

TEST_CASE("Money stores integer paise correctly") {
    Money amount(10050);

    CHECK(amount.paise() == 10050);
}

TEST_CASE("Money addition works correctly") {
    Money a(10050);
    Money b(2500);

    CHECK((a + b).paise() == 12550);
}

TEST_CASE("Money subtraction works correctly") {
    Money a(10050);
    Money b(2500);

    CHECK((a - b).paise() == 7550);
}

TEST_CASE("Money handles zero correctly") {
    Money amount(500);

    CHECK((amount + Money(0)).paise() == 500);
    CHECK((amount - Money(0)).paise() == 500);
}

TEST_CASE("Money detects addition overflow") {
    Money maxAmount(
        std::numeric_limits<std::int64_t>::max()
    );

    CHECK_THROWS_AS(
        maxAmount + Money(1),
        std::overflow_error
    );
}

TEST_CASE("Money detects subtraction underflow") {
    Money minAmount(
        std::numeric_limits<std::int64_t>::min()
    );

    CHECK_THROWS_AS(
        minAmount - Money(1),
        std::overflow_error
    );
}

TEST_CASE("Money detects negative addition underflow") {
    Money minAmount(
        std::numeric_limits<std::int64_t>::min()
    );

    CHECK_THROWS_AS(
        minAmount + Money(-1),
        std::overflow_error
    );
}

TEST_CASE("Money detects subtraction overflow") {
    Money maxAmount(
        std::numeric_limits<std::int64_t>::max()
    );

    CHECK_THROWS_AS(
        maxAmount - Money(-1),
        std::overflow_error
    );
}

TEST_CASE("Money comparison works correctly") {
    Money a(100);
    Money b(200);
    Money c(100);

    CHECK(a == c);
    CHECK(a < b);
}
