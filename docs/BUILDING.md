# Building OpenGizmos

## Prerequisites

### Windows

- **Visual Studio 2022** (Community edition or higher)
  - Install "Desktop development with C++" workload
- **CMake 3.16+** (included with VS, or install separately)
- **Git** (for cloning)

### Linux (future)

- GCC 9+ or Clang 10+
- CMake 3.16+
- SDL2 and SDL2_mixer development packages

## Quick Build (Windows)

The repository includes SDL2 libraries. Just clone and build:

```bash
git clone https://github.com/sp00nznet/-OpenGizmos.git
cd -OpenGizmos

mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output: `build/Release/opengg.exe`

## Build Options

### Generator Options

```bash
# Visual Studio 2022 (recommended)
cmake .. -G "Visual Studio 17 2022" -A x64

# Visual Studio 2019
cmake .. -G "Visual Studio 16 2019" -A x64

# Ninja (fast incremental builds)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Configuration

```bash
# Debug build
cmake --build . --config Debug

# Release build (optimized)
cmake --build . --config Release

# Release with debug info
cmake --build . --config RelWithDebInfo
```

## Using System SDL2

If you have SDL2 installed system-wide or via vcpkg:

```bash
# With vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# With system SDL2 (edit CMakeLists.txt to use find_package)
cmake .. -DSDL2_DIR=/path/to/SDL2/cmake
```

## Project Structure

```
opengg/
├── CMakeLists.txt      # Build configuration
├── include/            # Public headers
├── src/
│   ├── main.cpp        # Entry point
│   ├── loader/         # Asset loaders
│   ├── engine/         # SDL2 engine
│   ├── game/           # Game logic
│   ├── formats/        # Format headers
│   └── tools/          # Utilities
└── libs/               # SDL2 libraries (auto-downloaded)
```

## Targets

| Target | Description |
|--------|-------------|
| `opengg` | Main game executable |
| `asset_tool` | Asset extraction utility |

## Troubleshooting

### "SDL2 not found"

The build should auto-use libraries from `libs/`. If building without them:

1. Download SDL2-devel-*.zip from https://github.com/libsdl-org/SDL/releases
2. Download SDL2_mixer-devel-*.zip from https://github.com/libsdl-org/SDL_mixer/releases
3. Extract to `libs/SDL2-X.X.X/` and `libs/SDL2_mixer-X.X.X/`
4. Update paths in CMakeLists.txt if version differs

### Link errors with SDL_main

If you see errors about `SDL_main` or `WinMain`:
- Ensure `SDL_MAIN_HANDLED` is defined
- Don't link `SDL2main.lib`
- Call `SDL_SetMainReady()` in main()

### DLLs not found at runtime

The build copies DLLs to the output directory. If missing:
```bash
copy libs\SDL2-2.28.5\lib\x64\SDL2.dll build\Release\
copy libs\SDL2_mixer-2.6.3\lib\x64\SDL2_mixer.dll build\Release\
```

## Development

### Adding Source Files

1. Add `.cpp` to appropriate list in CMakeLists.txt
2. Add `.h` to `include/` or `src/` as appropriate
3. Reconfigure: `cmake ..`

### Code Style

- C++17 standard
- 4-space indentation
- `camelCase` for functions, `snake_case` for files
- Header guards: `#pragma once`

## IDE Setup

### Visual Studio

Open `build/OpenGizmos.sln` after running CMake.

### VS Code

Install extensions:
- C/C++ (Microsoft)
- CMake Tools

Configure:
```json
{
    "cmake.configureArgs": ["-G", "Visual Studio 17 2022", "-A", "x64"]
}
```

### CLion

Open the project folder. CLion auto-detects CMakeLists.txt.
