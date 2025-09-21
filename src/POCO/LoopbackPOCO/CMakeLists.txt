# g++ -std=c++17 loopback_poco.cpp -o loopback_poco \
#     -lpcap -lPocoFoundation -lPocoUtil -lpthread

cmake_minimum_required(VERSION 3.10)
set(TARGET "LoopbackPOCO")

set(CMAKE_CXX_STANDARD 17)

find_package(Poco REQUIRED Util Foundation)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE 
    Poco::Foundation
    Poco::Util
    pthread
    pcap
)
