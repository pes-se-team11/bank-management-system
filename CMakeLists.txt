
cmake_minimum_required(VERSION 3.20)
project(bms VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bms_options INTERFACE)

if(MSVC)
    target_compile_options(bms_options INTERFACE /W4 /WX)
else()
    target_compile_options(
        bms_options INTERFACE
        -Wall -Wextra -Werror -pedantic
    )
endif()

add_library(bms_core STATIC
    src/core/version.cpp
    src/validation/validation.cpp
    src/account/account_service.cpp
    src/auth/crypto.cpp
    src/auth/auth_service.cpp
    src/ledger/ledger.cpp
    src/transaction/transaction_service.cpp
    src/persistence/append_file.cpp
    src/persistence/file_account_store.cpp
    src/persistence/file_credential_store.cpp
)

target_include_directories(
    bms_core PUBLIC "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(bms_core PUBLIC bms_options)

if(WIN32)
    target_sources(bms_core PRIVATE src/platform/platform_win.cpp)
    # BCryptGenRandom, the OS random source for credential salts.
    target_link_libraries(bms_core PUBLIC bcrypt)
else()
    target_sources(bms_core PRIVATE src/platform/platform_posix.cpp)
endif()

# Console menu, kept separate from main() so tests can drive it.
add_library(bms_cli STATIC
    src/cli/menu.cpp
)
target_link_libraries(bms_cli PUBLIC bms_core)

add_executable(bms
    src/cli/main.cpp
)

target_link_libraries(bms PRIVATE bms_cli)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
