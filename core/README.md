# LD2420 Core Library

The core library provides platform-agnostic functionality for the HLK-LD2420 radar module. This is a standalone CMake project that can be built independently.

## Building the Core Library

Follow these steps to build the core library from the `core/` directory:

### Debug Build

```bash
cd core
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build .
```

### Release Build

```bash
cd core
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build .
```

## Using the Core Library in Your Project (no install)

Add the core as a subdirectory and link the target directly:

```cmake
add_subdirectory(/path/to/hlkld2420/core ld2420_core)
add_executable(your_app main.c)
target_link_libraries(your_app PRIVATE ld2420_core)
```

## Output Location

The compiled library (`libld2420_core.a`) will be in the build directory you created.
