# Third-party dependencies

# GLAD - OpenGL loader (built directly into Engine to avoid C compiler issues)
# add_subdirectory(glad)

# GLM - Mathematics library (header-only)
add_subdirectory(glm)

# stb_image - Image loading (header-only wrapper)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# EnTT - Entity Component System (header-only)
add_library(EnTT INTERFACE)
target_include_directories(EnTT INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt/src)

# Box2D - 2D physics engine
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
add_subdirectory(box2d)

# nlohmann/json - JSON parsing (header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# ImGui - Immediate mode GUI (with docking)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(ImGui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

# Lua - Scripting language (built from source so it works on all platforms)
set(LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6)
file(GLOB LUA_SOURCES ${LUA_DIR}/src/*.c)
# Exclude the Lua standalone interpreter and compiler (they have their own main())
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*(lua|luac)\\.c$")
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${LUA_DIR}/src)
# Suppress warnings from Lua source
target_compile_options(lua PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-w>
    $<$<C_COMPILER_ID:MSVC>:/w>
)

# sol2 - C++ Lua binding (header-only)
add_library(sol2 INTERFACE)
target_include_directories(sol2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sol2/include)
target_link_libraries(sol2 INTERFACE lua)
