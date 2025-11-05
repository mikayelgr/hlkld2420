# Building the Native Target

Follow the steps below to build the native target of the project from the root directory:

## Debug Build

1. **Configure the Project**  
   Use CMake presets to configure for native debug build:

   ```bash
   cmake --preset native-debug
   ```

2. **Compile the Library**  
   Build the library using the preset:

   ```bash
   cmake --build --preset native-debug
   ```

## Release Build

1. **Configure the Project**  
   Use CMake presets to configure for native release build (optimized):

   ```bash
   cmake --preset native-release
   ```

2. **Compile the Library**  
   Build the library using the preset:

   ```bash
   cmake --build --preset native-release
   ```

## Output Location

After a successful build, the compiled library (`libld2420_core.[a|dll|so]`) will be located in:

- Debug: `build/native-debug/core`
- Release: `build/native-release/core`

You can link this library to your application as required.
