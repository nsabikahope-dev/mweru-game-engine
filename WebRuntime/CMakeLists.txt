cmake_minimum_required(VERSION 3.20)
project(KirkianaWebRuntime C CXX)

# ---------------------------------------------------------------------------
# This project must be built with Emscripten:
#
#   cd WebRuntime
#   mkdir build && cd build
#   emcmake cmake ..
#   emmake make -j4
#
# Output: build/KirkianaWebRuntime.{html,js,wasm,data}
# Deploy the .js and .wasm files to calendar-store/frontend/public/wasm/
# ---------------------------------------------------------------------------

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR
        "KirkianaWebRuntime must be built with Emscripten.\n"
        "Run:  emcmake cmake <path-to-WebRuntime-dir>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Path to engine root (parent directory)
set(ENGINE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ---------------------------------------------------------------------------
# Engine source files (compiled directly — bypasses Engine/CMakeLists.txt
# which has native-only dependencies like SDL2 find_package, sndfile, etc.)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES "${ENGINE_ROOT}/Engine/src/*.cpp")

# Exclude glad.c — WebGL2 functions are directly available in Emscripten
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/glad/.*")

# ---------------------------------------------------------------------------
# Vendor: Lua 5.4 (C code, compiles with Emscripten)
# ---------------------------------------------------------------------------
set(LUA_DIR "${ENGINE_ROOT}/vendor/lua-5.4.6")
file(GLOB LUA_SOURCES "${LUA_DIR}/src/*.c")
# Exclude standalone interpreter (has its own main)
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*(lua|luac)\\.c$")

# ---------------------------------------------------------------------------
# Vendor: Box2D (C++, compiles with Emscripten)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE BOX2D_SOURCES "${ENGINE_ROOT}/vendor/box2d/src/*.cpp")

# ---------------------------------------------------------------------------
# WebRuntime source
# ---------------------------------------------------------------------------
set(WEBRUNTIME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WebRuntimeApp.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_executable(KirkianaWebRuntime
    ${ENGINE_SOURCES}
    ${WEBRUNTIME_SOURCES}
    ${LUA_SOURCES}
    ${BOX2D_SOURCES}
)

# ---------------------------------------------------------------------------
# Include paths
# Note: our glad/glad.h redirect comes FIRST to override vendor/glad
# ---------------------------------------------------------------------------
target_include_directories(KirkianaWebRuntime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include          # glad redirect + any web-specific headers
    ${ENGINE_ROOT}/Engine/include
    ${ENGINE_ROOT}/vendor/glad/include           # still needed for KHR/khrplatform.h
    ${ENGINE_ROOT}/vendor/glm
    ${ENGINE_ROOT}/vendor/stb
    ${ENGINE_ROOT}/vendor/entt/src
    ${ENGINE_ROOT}/vendor/box2d/include
    ${ENGINE_ROOT}/vendor/box2d/src              # Box2D internal headers
    ${ENGINE_ROOT}/vendor                        # for <json.hpp>
    ${LUA_DIR}/src
    ${ENGINE_ROOT}/vendor/sol2/include
)

# Suppress warnings in vendor C code
set_source_files_properties(${LUA_SOURCES} PROPERTIES COMPILE_FLAGS "-w")
set_source_files_properties(${BOX2D_SOURCES} PROPERTIES COMPILE_FLAGS "-w")

# ---------------------------------------------------------------------------
# Emscripten compile flags
# ---------------------------------------------------------------------------
target_compile_options(KirkianaWebRuntime PRIVATE
    "SHELL:-s USE_SDL=2"
    "SHELL:-s USE_WEBGL2=1"
    "SHELL:-s FULL_ES3=1"
)

# ---------------------------------------------------------------------------
# Emscripten link flags
# ---------------------------------------------------------------------------
set(EM_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")

target_link_options(KirkianaWebRuntime PRIVATE
    "SHELL:-s USE_SDL=2"
    "SHELL:-s USE_WEBGL2=1"
    "SHELL:-s FULL_ES3=1"
    "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s MAX_WEBGL_VERSION=2"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s INITIAL_MEMORY=268435456"     # 256 MB
    "SHELL:-s WASM=1"
    "SHELL:-s FETCH=1"                      # emscripten_fetch API for scene loading
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    "SHELL:-s EXPORTED_FUNCTIONS=['_main']"
    "SHELL:--shell-file ${EM_SHELL}"
    # Preload shared engine assets (fonts, default textures)
    # Game-specific scene files are fetched at runtime via HTTP
    "SHELL:--preload-file ${ENGINE_ROOT}/assets@/assets"
)

set_target_properties(KirkianaWebRuntime PROPERTIES
    SUFFIX ".html"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out
)
