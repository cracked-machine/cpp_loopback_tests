cmake_minimum_required(VERSION 3.10)
set(TARGET "LoopbackDPDK")

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PRIVATE ${DPDK_INCLUDE_DIRS})
target_link_libraries(${TARGET} PRIVATE ${DPDK_LIBRARIES})

# DPDK’s optimized rte_memcpy using SSSE3 instructions.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")