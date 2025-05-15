cmake_minimum_required(VERSION 3.16)
project(main LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

include(FetchContent)

# ---------------------------------------
# Fetch GLAD
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# ---------------------------------------
# Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.2
)
FetchContent_MakeAvailable(imgui)

# ---------------------------------------
# Fetch stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# ---------------------------------------
# Fetch nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------
# Fetch glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# ---------------------------------------
# Fetch SDL2
FetchContent_Declare(
    sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.26.5
)
set(SDL_TEST      OFF CACHE BOOL "" FORCE)
set(SDL_TESTS     OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sdl2)

# Grab SDL’s public include directories *now* – we’ll attach them later
get_target_property(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)

# ---------------------------------------
# 1) Define the executable BEFORE adding sources
add_executable(main)

# 2) Gather only your own .cpp files
file(GLOB_RECURSE SRC_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
target_sources(main PRIVATE ${SRC_SOURCES})

# 3) Mark SDL’s headers as SYSTEM so their warnings stay silent
target_include_directories(main SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})

# 4) Your strict warnings for *your* code only
target_compile_options(main PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -Wshadow -Wnon-virtual-dtor
    -Wold-style-cast -Wcast-align
    -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion
    -Wstrict-overflow=5 -Wformat=2
    -Woverloaded-virtual -Wundef 
    -Wfloat-equal -Wzero-as-null-pointer-constant
    -Wmissing-declarations -Wimplicit-fallthrough
    -Wunreachable-code -ftrapv 
)


# ---------------------------------------
# Copy assets
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
        COMMENT "Copying assets into build directory"
    )
else()
    message(WARNING "No assets directory found at ${CMAKE_SOURCE_DIR}/assets – skipping copy_assets target.")
    add_custom_target(copy_assets ALL
        COMMENT "No assets to copy"
    )
endif()
add_dependencies(main copy_assets)

# ---------------------------------------
# Include directories for your target
target_include_directories(main PRIVATE
    ${glad_SOURCE_DIR}/include
)

# Treat all third-party includes as SYSTEM so they never warn
target_include_directories(main SYSTEM PRIVATE
    ${sdl2_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
)

find_package(OpenGL REQUIRED)

# ---------------------------------------
# Link libraries
target_link_libraries(main PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    glad
    OpenGL::GL
    imgui_impl
    glm::glm
    nlohmann_json::nlohmann_json
)

# ---------------------------------------
# ImGui backend implementation
add_library(imgui_impl STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_impl SYSTEM PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${sdl2_SOURCE_DIR}/include
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)