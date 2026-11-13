cmake_minimum_required(VERSION 3.24)
project(breakout LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch raylib at configure time (no system install needed).
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(breakout
    src/main.cpp
)

target_link_libraries(breakout PRIVATE raylib)
target_compile_options(breakout PRIVATE -Wall -Wextra -Wpedantic)

if (APPLE)
    target_link_libraries(breakout PRIVATE
        "-framework Cocoa" "-framework IOKit" "-framework OpenGL" "-framework CoreVideo")
endif()
