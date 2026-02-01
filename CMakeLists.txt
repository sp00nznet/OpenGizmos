cmake_minimum_required(VERSION 3.16)
project(OpenGizmos VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 paths (using local libs)
set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2-2.28.5")
set(SDL2_MIXER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2_mixer-2.6.3")

# Determine architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDL2_ARCH "x64")
else()
    set(SDL2_ARCH "x86")
endif()

# SDL2 include and library paths
set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include")
set(SDL2_LIBRARY "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.lib")
set(SDL2_MAIN_LIBRARY "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2main.lib")
set(SDL2_DLL "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll")

set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include")
set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.lib")
set(SDL2_MIXER_DLL "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll")

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
    ${SDL2_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR}
)

target_link_libraries(opengg PRIVATE
    ${SDL2_LIBRARY}
    ${SDL2_MIXER_LIBRARY}
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(opengg PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        SDL_MAIN_HANDLED
    )
endif()

# Copy DLLs on Windows
if(WIN32)
    add_custom_command(TARGET opengg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL}"
            $<TARGET_FILE_DIR:opengg>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_MIXER_DLL}"
            $<TARGET_FILE_DIR:opengg>
    )
endif()

# Asset extraction tool (standalone, no SDL needed)
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

# Install targets
install(TARGETS opengg asset_tool
    RUNTIME DESTINATION bin
)

install(FILES "${SDL2_DLL}" "${SDL2_MIXER_DLL}"
    DESTINATION bin
)
