include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE
)

set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()
endif()

FetchContent_MakeAvailable(doctest)

# Bank Management System unit tests
add_executable(bms_tests
    test_transaction.cpp
    test_main.cpp
    test_money.cpp
    test_validation.cpp
    test_crypto.cpp
    test_account.cpp
    test_auth.cpp
    test_ledger.cpp
    test_cli.cpp
    test_persistence.cpp
)

target_compile_definitions(
    bms_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS
)

target_link_libraries(bms_tests PRIVATE
    bms_cli
    doctest::doctest
)

add_test(NAME unit COMMAND bms_tests)