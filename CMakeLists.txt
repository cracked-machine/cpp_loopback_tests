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
    add_subdirectory(src/HelloBoost)

elseif(SUBPROJECT STREQUAL "HelloAFXDP")
    message(STATUS "Configuring HelloAFXDP project")
    add_subdirectory(src/HelloAFXDP)

elseif(SUBPROJECT STREQUAL "HelloDPDK")
    message(STATUS "Configuring HelloDPDK project")
    add_subdirectory(src/HelloDPDK)

endif()

