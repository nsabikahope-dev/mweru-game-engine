cmake_minimum_required(VERSION 3.20)
project(GameEngine VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug build by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find SDL2
find_package(SDL2 REQUIRED)

# Add vendor subdirectories
add_subdirectory(vendor)

# Add Engine library
add_subdirectory(Engine)

# Add Sandbox application
add_subdirectory(Sandbox)

# Add Editor application
add_subdirectory(Editor)

# Add BreakoutGame demo
add_subdirectory(BreakoutGame)

# Runtime: standalone game player (no editor UI)
add_subdirectory(Runtime)
