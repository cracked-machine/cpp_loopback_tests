cmake_minimum_required(VERSION 3.10)
set(TARGET "HelloPoco")
project(${TARGET})

set(CMAKE_CXX_STANDARD 17)

find_package(Poco REQUIRED Foundation)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE Poco::Foundation)
