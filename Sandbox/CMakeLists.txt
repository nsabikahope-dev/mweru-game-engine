# Sandbox test application
project(Sandbox)

# Collect source files
file(GLOB SANDBOX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(Sandbox ${SANDBOX_SOURCES})

# Link against Engine library
target_link_libraries(Sandbox PRIVATE Engine)

# Copy assets to build directory
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMENT "Copying assets to build directory"
)

# Physics demo executable
add_executable(PhysicsDemo ${CMAKE_CURRENT_SOURCE_DIR}/PhysicsDemo.cpp)
target_link_libraries(PhysicsDemo PRIVATE Engine)

# Serialization demo executable
add_executable(SerializationDemo ${CMAKE_CURRENT_SOURCE_DIR}/SerializationDemo.cpp)
target_link_libraries(SerializationDemo PRIVATE Engine)
