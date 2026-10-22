// doctest entry point. Add a test_<module>.cpp per module and list it in
// tests/CMakeLists.txt. Name cases after the Test Plan ids (TC-DEP-01 ...).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstring>

#include "core/version.h"

TEST_CASE("scaffold: library links and version is set") {
    CHECK(std::strlen(bms::kVersion) > 0);
}
