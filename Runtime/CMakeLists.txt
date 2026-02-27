# Runtime application — standalone game player (no editor UI)
project(Runtime)

file(GLOB_RECURSE RUNTIME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(Runtime ${RUNTIME_SOURCES})

target_link_libraries(Runtime PRIVATE Engine)

target_include_directories(Runtime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
