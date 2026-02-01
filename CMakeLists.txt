cmake_minimum_required(VERSION 3.16)
project(OpenGizmos VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Source files
set(LOADER_SOURCES
    src/loader/ne_resource.cpp
    src/loader/grp_archive.cpp
    src/loader/smacker.cpp
    src/loader/asset_cache.cpp
)

set(ENGINE_SOURCES
    src/engine/renderer.cpp
    src/engine/audio.cpp
    src/engine/input.cpp
    src/engine/game_loop.cpp
)

set(GAME_SOURCES
    src/game/entity.cpp
    src/game/room.cpp
    src/game/puzzle.cpp
    src/game/player.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${LOADER_SOURCES}
    ${ENGINE_SOURCES}
    ${GAME_SOURCES}
)

# Main executable
add_executable(opengg ${ALL_SOURCES})

target_include_directories(opengg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(opengg PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_mixer::SDL2_mixer
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(opengg PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Copy SDL2 DLLs on Windows (for convenience)
if(WIN32)
    add_custom_command(TARGET opengg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:opengg>
    )
endif()

# Asset extraction tool (standalone)
add_executable(asset_tool
    src/tools/asset_tool.cpp
    src/loader/ne_resource.cpp
    src/loader/grp_archive.cpp
)

target_include_directories(asset_tool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(WIN32)
    target_compile_definitions(asset_tool PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
endif()
