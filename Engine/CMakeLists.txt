# Engine library
project(Engine)

# Collect all source files
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# Add GLAD source directly to avoid C compiler issues
set(GLAD_SOURCE "${CMAKE_SOURCE_DIR}/vendor/glad/src/glad.c")

# Create Engine static library
add_library(Engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${GLAD_SOURCE})

# Include directories
target_include_directories(Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/vendor/glad/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Find audio libraries
find_package(OpenAL REQUIRED)
find_library(SNDFILE_LIB sndfile REQUIRED)

# Link libraries
target_link_libraries(Engine
    PUBLIC
        SDL2::SDL2
        glm
        stb_image
        EnTT
        box2d
        nlohmann_json
        lua
        sol2
        OpenAL::OpenAL
        ${SNDFILE_LIB}
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(Engine PUBLIC ENGINE_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(Engine PUBLIC ENGINE_PLATFORM_LINUX)
elseif(APPLE)
    target_compile_definitions(Engine PUBLIC ENGINE_PLATFORM_MACOS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(Engine PRIVATE /W4)
else()
    target_compile_options(Engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
