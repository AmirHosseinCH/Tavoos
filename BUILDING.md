# Building Tavoos

Developed and tested on Ubuntu 24.04 (Linux). Other platforms aren't validated.

## Requirements

- CMake >= 3.24
- GCC >= 14 - Tavoos's fluent widget-builder API uses C++23 "deducing this", which needs GCC 14 or newer. Configuring with an older compiler fails immediately with a clear error, whether you're building Tavoos itself or consuming it via `find_package(Tavoos)`.
- OpenGL

## Dependencies

Available via apt on Ubuntu 24.04:

```bash
sudo apt install g++-14 libglfw3-dev libspdlog-dev libglm-dev libfreetype-dev libfontconfig-dev
```

`lunasvg` has no Debian package - build and install it from source first:

```bash
git clone https://github.com/sammycage/lunasvg.git
cmake -S lunasvg -B lunasvg/build -DCMAKE_BUILD_TYPE=Release
cmake --build lunasvg/build -j"$(nproc)"
sudo cmake --install lunasvg/build
```

`Fontconfig` is required on Linux only (used for system-default font fallback).

## Configure and build

GCC 14 must be selected explicitly if it isn't your system's default `g++`:

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

## Build options

| Option | Default | Description |
|---|---|---|
| `TAVOOS_BUILD_EXAMPLES` | `ON` | Build the example app under `examples/` |
| `TAVOOS_WARNINGS_AS_ERRORS` | `OFF` | Treat compiler warnings as errors |

## Installing / using Tavoos in another project

```bash
sudo cmake --install build
```

Then, in a consuming project's `CMakeLists.txt`:

```cmake
find_package(Tavoos REQUIRED)
target_link_libraries(your_app PRIVATE Tavoos::Tavoos)
```

## Building the .deb package

```bash
cd build
cpack -G DEB
```

Produces a single `libtavoos-dev` package (headers, CMake config, and the shared library together - Tavoos isn't in an apt repository yet, so there's no benefit to a separate runtime package that consumers would have to install in the right order by hand).
