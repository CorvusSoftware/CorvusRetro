# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
cmake_minimum_required(VERSION 3.22.1)
project(CorvusRetro LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CORVUS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(BUILD_TESTING OFF CACHE BOOL "Tests are built separately" FORCE)
set(CORVUSCORE_BUILD_LIBRETRO OFF CACHE BOOL "Use the native C API" FORCE)
set(CORVUSCORE_INSTALL OFF CACHE BOOL "Embedded dependency" FORCE)
add_subdirectory("${CORVUS_ROOT}/third_party/CorvusCore" corvuscore)
# Debug APKs still need optimized emulation; keep debug symbols and assertions.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    foreach(core_target corvuscore_common gbc_hardware gb_corvus_core gbc_corvus_core gba_corvus_core corvuscore_c_api)
        get_target_property(core_type ${core_target} TYPE)
        if(NOT core_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${core_target} PRIVATE -O2)
        endif()
    endforeach()
endif()
add_library(corvusretro SHARED
    ../jni/bridge.c
    core_bridge.cpp
    "${CORVUS_ROOT}/core/src/corvus_core.c")
target_include_directories(corvusretro PRIVATE "${CORVUS_ROOT}/core/include")
target_link_libraries(corvusretro PRIVATE corvuscore_c_api)
target_compile_options(corvusretro PRIVATE -Wall -Wextra -Werror)
if(ANDROID)
    target_link_options(corvusretro PRIVATE "-Wl,-z,max-page-size=16384")
else()
    find_package(JNI REQUIRED)
    target_include_directories(corvusretro PRIVATE ${JNI_INCLUDE_DIRS})
endif()
