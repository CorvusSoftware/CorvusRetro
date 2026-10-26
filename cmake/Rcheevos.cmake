# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
set(_rcheevos "${CMAKE_CURRENT_LIST_DIR}/../deps/rcheevos")
file(GLOB _rcheevos_runtime CONFIGURE_DEPENDS "${_rcheevos}/src/rcheevos/*.c")
file(GLOB _rcheevos_api CONFIGURE_DEPENDS "${_rcheevos}/src/rapi/*.c")
add_library(rcheevos STATIC
    "${_rcheevos}/src/rc_client.c" "${_rcheevos}/src/rc_compat.c"
    "${_rcheevos}/src/rc_util.c" "${_rcheevos}/src/rc_version.c"
    "${_rcheevos}/src/rhash/hash.c" "${_rcheevos}/src/rhash/hash_rom.c"
    "${_rcheevos}/src/rhash/md5.c" ${_rcheevos_runtime} ${_rcheevos_api})
target_include_directories(rcheevos PUBLIC "${_rcheevos}/include")
target_compile_definitions(rcheevos PRIVATE RC_DISABLE_LUA RC_CLIENT_SUPPORTS_HASH
    RC_HASH_NO_DISC RC_HASH_NO_ZIP RC_HASH_NO_ENCRYPTED)
set_target_properties(rcheevos PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 11)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(rcheevos PUBLIC Threads::Threads m)
endif()
add_library(corvus_cheevos STATIC "${CMAKE_CURRENT_LIST_DIR}/../cheevos/cheevos.c")
target_include_directories(corvus_cheevos PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../cheevos")
target_link_libraries(corvus_cheevos PUBLIC rcheevos)
target_compile_options(corvus_cheevos PRIVATE -Wall -Wextra -Werror)
set_target_properties(corvus_cheevos PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 11)
