# Editor application
project(Editor)

# Collect source files
file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(Editor ${EDITOR_SOURCES})

# Link against Engine library and ImGui
target_link_libraries(Editor PRIVATE Engine ImGui)

# Include directories
target_include_directories(Editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
