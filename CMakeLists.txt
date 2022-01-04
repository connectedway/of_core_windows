cmake_minimum_required(VERSION 3.21.1)
project(of_core_linux VERSION 1.0.1 DESCRIPTION "OpenFiles Platform for Linux")

include(configs/default)
configure_file(include/ofc_windows/config.h.in ofc_windows/config.h @ONLY)
include_directories(
        ${of_core_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${of_core_windows_SOURCE_DIR}/include
)

set(SRCS
        src/console_windows.c
        src/env_windows.c
        src/event_windows.c
        src/lock_windows.c
        src/net_windows.c
        src/process_windows.c
        src/socket_windows.c
        src/thread_windows.c
        src/time_windows.c
        src/waitset_windows.c
        )

add_library(of_core_windows STATIC ${SRCS})
target_link_libraries(of_core_windows PUBLIC pthread of_core_static)

set_target_properties(of_core_windows PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(of_core_windows PROPERTIES SOVERSION 1)

