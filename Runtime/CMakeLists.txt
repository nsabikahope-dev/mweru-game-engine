# Runtime application — standalone game player (no editor UI)
project(Runtime)

file(GLOB_RECURSE RUNTIME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(Runtime ${RUNTIME_SOURCES})

# SDL2::SDL2main must come first on Windows — it provides the WinMain entry
# point that SDL2 requires when it redefines main as SDL_main via macro.
target_link_libraries(Runtime PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    Engine
)

target_include_directories(Runtime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
