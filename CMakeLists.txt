cmake_minimum_required(VERSION 3.28)
project(LoopbackExamples LANGUAGES CXX) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message("PROJECT NAME: ${SUBPROJECT}")
if(SUBPROJECT STREQUAL "HelloPOCO")
    message(STATUS "Configuring HelloPOCO project")
    add_subdirectory(src/POCO/HelloPOCO)

elseif(SUBPROJECT STREQUAL "LoopbackPOCO")
    message(STATUS "Configuring LoopbackPOCO project")
    add_subdirectory(src/POCO/LoopbackPOCO)

elseif(SUBPROJECT STREQUAL "HelloBoost")
    message(STATUS "Configuring HelloBoost project")
    add_subdirectory(src/Boost/HelloBoost)

elseif(SUBPROJECT STREQUAL "LoopbackBoost")
    message(STATUS "Configuring LoopbackBoost project")
    add_subdirectory(src/Boost/LoopbackBoost)

elseif(SUBPROJECT STREQUAL "HelloAFXDP")
    message(STATUS "Configuring HelloAFXDP project")
    add_subdirectory(src/AFXDP/HelloAFXDP)

elseif(SUBPROJECT STREQUAL "LoopbackAFXDP")
    message(STATUS "Configuring LoopbackAFXDP project")
    add_subdirectory(src/AFXDP/LoopbackAFXDP)

elseif(SUBPROJECT STREQUAL "LoopbackAFXDP-RHEL9_6")
    message(STATUS "Configuring LoopbackAFXDP-RHEL9_6 project")
    add_subdirectory(src/AFXDP/LoopbackAFXDP-RHEL9_6)

elseif(SUBPROJECT STREQUAL "HelloDPDK")
    message(STATUS "Configuring HelloDPDK project")
    add_subdirectory(src/DPDK/HelloDPDK)

elseif(SUBPROJECT STREQUAL "LoopbackDPDK")
    message(STATUS "Configuring LoopbackDPDK project")
    add_subdirectory(src/DPDK/LoopbackDPDK)

elseif(SUBPROJECT STREQUAL "LoopbackDPDK-RHEL9_6")
    message(STATUS "Configuring LoopbackDPDK-RHEL9_6 project")
    add_subdirectory(src/DPDK/LoopbackDPDK-RHEL9_6)
endif()

