cmake_minimum_required(VERSION 3.28)
# project(LoopbackExamples LANGUAGES CXX) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message("PROJECT NAME: ${SUBPROJECT}")
if(SUBPROJECT STREQUAL "HelloPoco")
    message(STATUS "Configuring HelloPoco project")
    add_subdirectory(src/HelloPoco)
elseif(SUBPROJECT STREQUAL "HelloBoost")
    message(STATUS "Configuring HelloBoost project")
    add_subdirectory(src/HelloBoost)
endif()

