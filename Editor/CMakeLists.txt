# Editor application
project(Editor)

# Collect source files
file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(Editor ${EDITOR_SOURCES})

# Link against Engine library and ImGui
# SDL2::SDL2main must come first on Windows — it provides the WinMain entry
# point that SDL2 requires when it redefines main as SDL_main via macro.
target_link_libraries(Editor PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    Engine ImGui
)

# Include directories
target_include_directories(Editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
