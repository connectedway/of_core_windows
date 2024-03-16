cmake_minimum_required(VERSION 3.20.0)
project(of_core_windows VERSION 1.0.1 DESCRIPTION "OpenFiles Platform for Linux")

include(configs/default)
configure_file(include/ofc_windows/config.h.in ofc_windows/config.h @ONLY)
include_directories(
        ${of_core_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${of_core_windows_SOURCE_DIR}/include
)

set(SRCS
        src/backtrace_windows.c
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

add_library(of_core_windows OBJECT ${SRCS})
set_property(TARGET of_core_windows PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(of_core_windows PUBLIC wsock32 ws2_32)


