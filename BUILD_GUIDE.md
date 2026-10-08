# MMapper Build Guide - Seasonal Textures Feature

## Prerequisites

### Required Software
1. **CMake** (version 3.19 or higher)
   - Download from: https://cmake.org/download/
   - Make sure to add CMake to your PATH during installation

2. **Qt6** (version 6.2.0 or higher)
   - Download from: https://www.qt.io/download
   - Install Qt6 with OpenGL components
   - Set environment variable: `QTDIR=C:\Qt\6.x.x\msvc2019_64` (adjust path)

3. **Ninja Build System**
   - Already included in your project directory (ninja.exe)
   - Or download from: https://github.com/ninja-build/ninja/releases

4. **C++ Compiler** (one of):
   - **MSVC** (Visual Studio 2019 or 2022) - Recommended for Windows
   - **MinGW-w64**
   - **Clang**

## Quick Build (Windows)

### Option 1: Using the build script
```batch
build.bat
```

### Option 2: Manual build
```batch
# Create build directory
mkdir build
cd build

# Configure
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Build
cmake --build . --config RelWithDebInfo

# Run
src\mmapper.exe
```

## Common Build Errors and Fixes

### Error: "Qt6 not found"
**Solution:** Set QTDIR environment variable:
```batch
set QTDIR=C:\Qt\6.x.x\msvc2019_64
```
Or add to CMake command:
```batch
cmake -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64" ..
```

### Error: "Cannot find Qt6OpenGLWidgets"
**Solution:** Install Qt6 OpenGL components. Run Qt Maintenance Tool and select:
- Qt6 → MSVC 2019 64-bit → Qt OpenGL

### Error: "undefined reference to `setCurrentSeason`"
**Fix:** Already fixed in the code - the include for `Filenames.h` was added to `mapcanvas.cpp`

### Error: "MumeSeasonEnum does not name a type"
**Fix:** Already fixed - forward declaration added to `mapcanvas.h`

### Error: "sig_seasonChanged is not a member of MumeClock"
**Solution:**
1. Delete the build directory: `rmdir /s /q build`
2. Rebuild: This forces Qt's MOC (Meta-Object Compiler) to re-generate signal/slot code

## Building with Visual Studio

### Using Visual Studio IDE
```batch
# Generate Visual Studio solution
cmake -G "Visual Studio 16 2019" -A x64 ..

# Open mmapper.sln in Visual Studio
start mmapper.sln
```

Then build from Visual Studio (F7 or Build → Build Solution)

### Using Visual Studio from Command Line
```batch
# Open Visual Studio Developer Command Prompt
# Then:
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config RelWithDebInfo
```

## Build Types

- **Debug**: Full debug symbols, no optimization
  ```batch
  cmake -DCMAKE_BUILD_TYPE=Debug ..
  ```

- **Release**: Optimized, no debug symbols
  ```batch
  cmake -DCMAKE_BUILD_TYPE=Release ..
  ```

- **RelWithDebInfo**: Optimized with debug symbols (recommended)
  ```batch
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
  ```

## Testing the Seasonal Texture Feature

After successful build:

1. **Organize your textures** following this structure:
   ```
   src/resources/pixmaps/
   ├── Classic/              # Old textures
   │   ├── terrain-*.png
   │   ├── road-*.png
   │   └── ...
   └── Modern/               # New textures
       ├── Spring/           # Seasonal variants
       │   ├── terrain-field.png
       │   ├── terrain-forest.png
       │   └── ...
       ├── Summer/
       ├── Autumn/
       ├── Winter/
       ├── terrain-indoors.png  # Non-seasonal
       ├── mob-*.png
       └── ...
   ```

2. **Build the resources** into the executable:
   - The textures need to be compiled into the Qt resource file
   - Run: `cmake --build . --target mmapper`

3. **Test in-game**:
   - Launch MMapper
   - Connect to MUME
   - Type `time` in MUME to check current season
   - Watch the textures reload when season changes!

## Troubleshooting Qt MOC Issues

If you get errors about missing signals/slots:

```batch
# Clean build
cd build
del /q CMakeCache.txt
del /q /s moc_*.cpp
cmake ..
cmake --build .
```

## CMake Configuration Options

```batch
# Disable tests (faster build)
cmake -DWITH_TESTS=OFF ..

# Enable additional features
cmake -DWITH_WEBSOCKET=ON ..      # WebSocket support
cmake -DWITH_QTKEYCHAIN=ON ..     # Secure password storage
```

## Next Steps

After building successfully:

1. **Add UI Elements**:
   - Open `src/preferences/graphicspage.ui` in Qt Designer
   - Add QComboBox named `textureSetComboBox` with items: Classic, Modern, Custom
   - Add QCheckBox named `enableSeasonalTexturesCheckBox`

2. **Uncomment UI Code** in `src/preferences/graphicspage.cpp`:
   - Lines 98-103 (signal connections)
   - Lines 139-142 (loadConfig)

3. **Rebuild** to include UI changes

## Getting Help

If you encounter build errors:

1. Check the error message carefully
2. Search for the error in MMapper GitHub issues
3. Make sure all prerequisites are installed
4. Try a clean build (delete `build` folder and rebuild)
5. Check Qt version compatibility (must be 6.2.0+)

## Development Tips

- Use **RelWithDebInfo** build type for development (balanced speed + debugging)
- Run `ninja clean` to clean build artifacts without deleting CMake cache
- Use `cmake --build . -j8` to build with 8 parallel jobs (faster)
- Set `VERBOSE=1` for detailed build output: `cmake --build . --verbose`
