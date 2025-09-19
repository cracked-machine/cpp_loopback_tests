cmake_minimum_required(VERSION 3.28)
project(ProceduralMazeProject LANGUAGES CXX) 

include(FetchContent) 
FetchContent_Declare(SPDLog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG 6fa36017cfd5731d617e1a934f0e5ea9c4445b13    # v1.15.3
    GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)

FetchContent_Declare(EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG d4014c74dc3793aba95ae354d6e23a026c2796db    # v3.15.0
    EXCLUDE_FROM_ALL SYSTEM
)

FetchContent_MakeAvailable(
    SPDLog
    EnTT
)

# ThirdParty Lib warning Supression:
# 1. spdlog-src/include/spdlog/logger.h:317:10: error: 
# variable 'args' might be clobbered by 'longjmp' or 'vfork' 
target_compile_options(spdlog INTERFACE -Wno-clobbered)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory( ${CMAKE_SOURCE_DIR}/src )

