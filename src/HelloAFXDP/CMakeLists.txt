cmake_minimum_required(VERSION 3.10)
set(TARGET "HelloAFXDP")

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBPF REQUIRED libbpf)

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PRIVATE ${LIBBPF_INCLUDE_DIRS})
target_link_libraries(${TARGET} PRIVATE ${LIBBPF_LIBRARIES})
