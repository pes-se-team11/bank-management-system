include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE
)

set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

# Compatibility for this dependency's older CMake minimum.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()
endif()

FetchContent_MakeAvailable(doctest)

add_executable(bms_tests
    test_main.cpp
    # Add test_money.cpp, test_auth.cpp, etc. when created.
)

target_link_libraries(bms_tests PRIVATE
    bms_core
    doctest::doctest
)

add_test(NAME unit COMMAND bms_tests)
