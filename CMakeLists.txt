cmake_minimum_required(VERSION 3.20)
project(laya VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wnon-virtual-dtor)
endif()

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSESW REQUIRED IMPORTED_TARGET ncursesw)

find_package(Threads REQUIRED)

# Include directories
include_directories(include external)

# Components target
add_library(laya_core
    ##
)

target_include_directories(laya_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(laya_core PUBLIC
    PkgConfig::NCURSESW
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m
)

# Executable
add_executable(laya src/main.cpp)
target_link_libraries(laya PRIVATE laya_core)