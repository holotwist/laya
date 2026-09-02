cmake_minimum_required(VERSION 3.20)
project(Laya VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wnon-virtual-dtor)
endif()

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSESW REQUIRED IMPORTED_TARGET ncursesw)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Includes
include_directories(include external)

# Core Library Target
add_library(laya_core
    src/core/time_utils.cpp
    src/core/lrc_document.cpp
    src/crypto/sha256.cpp
    src/net/challenge_solver.cpp
    src/net/lrclib_client.cpp
    src/audio/miniaudio_impl.cpp
    src/audio/audio_player.cpp
    src/ui/app.cpp
    src/ui/editor_view.cpp
    src/ui/player_view.cpp
)

target_include_directories(laya_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(laya_core PUBLIC
    PkgConfig::NCURSESW
    CURL::libcurl
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m
)

# Executable Target
add_executable(laya src/main.cpp)
target_link_libraries(laya PRIVATE laya_core)