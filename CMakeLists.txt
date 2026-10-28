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
    src/persistence/append_file.cpp
    # Add service implementation files here as they are created.
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

add_executable(bms
    src/cli/main.cpp
    # Future CLI implementation files belong here.
)

target_link_libraries(bms PRIVATE bms_core)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
