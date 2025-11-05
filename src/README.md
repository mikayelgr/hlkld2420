# Building the Native Target

Follow the steps below to build the native target of the project from the root directory:

1. **Configure the Project**  
   Use CMake to generate the build files:

   ```bash
   cmake -S . -B build/native
   ```

2. **Compile the Library**  
   Build the library using the generated build files:

   ```bash
   make -C build/native
   ```

3. **Locate the Compiled Library**  
   After a successful build, the compiled library (`libld2420_core.[a|dll|so]`) will be located in the `build/native/core` directory.  
   You can link this library to your application as required.
