# Building OpenGG

## Prerequisites

### Windows

- **Visual Studio 2022** (Community edition or higher)
  - Install "Desktop development with C++" workload
- **CMake 3.16+** (included with VS, or install separately)
- **Git**

### Linux (future)

- GCC 9+ or Clang 10+
- CMake 3.16+
- SDL2 and SDL2_mixer development packages

## Quick Build

The repository includes SDL2 libraries. Clone and build:

```bash
git clone https://github.com/sp00nznet/opengg.git
cd opengg

mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output: `build/Release/opengg.exe` and `build/Release/asset_tool.exe`

## Build Options

### Generators

```bash
# Visual Studio 2022 (recommended)
cmake .. -G "Visual Studio 17 2022" -A x64

# Visual Studio 2019
cmake .. -G "Visual Studio 16 2019" -A x64

# Ninja (fast incremental builds)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Configurations

```bash
cmake --build . --config Debug          # Debug symbols, no optimization
cmake --build . --config Release        # Optimized
cmake --build . --config RelWithDebInfo # Optimized + debug info
```

## Using System SDL2

```bash
# With vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# With system SDL2 (edit CMakeLists.txt to use find_package)
cmake .. -DSDL2_DIR=/path/to/SDL2/cmake
```

## Targets

| Target | Description |
|--------|-------------|
| `opengg` | Main game executable -- launcher, engine, all game states |
| `asset_tool` | Standalone asset extraction utility (no SDL dependency) |

## Project Structure

```
opengg/
+-- CMakeLists.txt          # Build configuration
+-- README.md
+-- include/                # Public headers
|   +-- game_loop.h         #   Game class, state machine
|   +-- game_registry.h     #   Multi-game discovery
|   +-- asset_cache.h       #   Unified asset loading
|   +-- renderer.h          #   SDL2 rendering
|   +-- neptune/            #   Neptune game headers
|   +-- ...
+-- src/
|   +-- main.cpp            #   Entry point, all UI states
|   +-- loader/             #   NE, GRP, Smacker, AssetCache
|   +-- engine/             #   Renderer, Audio, Input, GameLoop, Menu
|   +-- game/               #   Entities, Rooms, Puzzles, GameRegistry
|   +-- neptune/            #   Operation Neptune game logic
|   +-- bot/                #   Bot/AI system
|   +-- tools/              #   asset_tool CLI
+-- libs/                   # SDL2 + SDL2_mixer (bundled)
+-- docs/                   # Documentation
```

## Troubleshooting

### "SDL2 not found"

The build auto-uses libraries from `libs/`. If building without them:

1. Download SDL2-devel from https://github.com/libsdl-org/SDL/releases
2. Download SDL2_mixer-devel from https://github.com/libsdl-org/SDL_mixer/releases
3. Extract to `libs/SDL2-X.X.X/` and `libs/SDL2_mixer-X.X.X/`
4. Update version paths in CMakeLists.txt if needed

### Link errors with SDL_main

- Ensure `SDL_MAIN_HANDLED` is defined (it is by default in CMakeLists.txt)
- Call `SDL_SetMainReady()` before `SDL_Init()`

### DLLs not found at runtime

The build copies DLLs automatically. If missing:
```bash
copy libs\SDL2-2.28.5\lib\x64\SDL2.dll build\Release\
copy libs\SDL2_mixer-2.6.3\lib\x64\SDL2_mixer.dll build\Release\
```

## IDE Setup

### Visual Studio

Open `build/OpenGizmos.sln` after running CMake.

### VS Code

Extensions: C/C++ (Microsoft), CMake Tools.

```json
{
    "cmake.configureArgs": ["-G", "Visual Studio 17 2022", "-A", "x64"]
}
```

### CLion

Open the project folder. CLion auto-detects CMakeLists.txt.

## Code Style

- C++17 standard
- 4-space indentation
- `camelCase` for functions, `snake_case` for files
- `#pragma once` header guards
- `namespace opengg` wraps all engine code
