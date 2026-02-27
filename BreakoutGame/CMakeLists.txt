# Breakout Game - Simple demo game
project(BreakoutGame)

# Collect source files
file(GLOB BREAKOUT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(BreakoutGame ${BREAKOUT_SOURCES})

# Link against Engine library
target_link_libraries(BreakoutGame PRIVATE Engine)
