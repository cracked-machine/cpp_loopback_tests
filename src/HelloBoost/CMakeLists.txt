cmake_minimum_required(VERSION 3.10)
set(TARGET "HelloBoost")

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED)

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIRS})
