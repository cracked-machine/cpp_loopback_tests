cmake_minimum_required(VERSION 3.10)
set(TARGET "HelloDPDK")

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PRIVATE ${DPDK_INCLUDE_DIRS})
target_link_libraries(${TARGET} PRIVATE ${DPDK_LIBRARIES})
