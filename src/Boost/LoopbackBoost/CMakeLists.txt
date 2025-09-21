# g++ -std=c++17 main.cpp -o LoopbackBoost \
#     -lboost_program_options -lboost_thread -lboost_system -lpthread -lpcap

cmake_minimum_required(VERSION 3.10)
set(TARGET "LoopbackBoost")

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED program_options thread system)

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(${TARGET} PRIVATE 
    Boost::system
    Boost::thread
    Boost::program_options
    pthread
    pcap
)