# Building MMapper

## Overview

This guide describes how to build **MMapper** from source on Linux, macOS, and Windows using CMake and Ninja. For tested build configurations, see the [GitHub Actions workflows](.github/workflows).

## Prerequisites

Ensure the following tools are installed:

- A C++17-compatible compiler (e.g., GCC, Clang, MSVC)
- [CMake 3.20+](https://cmake.org/)
- [Ninja](https://ninja-build.org/)
- [Qt 6.2+](https://www.qt.io/) (6.5 recommended) with `QtWebSockets`

---

## 1. Get the Code

```bash
git clone https://github.com/MUME/MMapper.git
cd MMapper
```

---

## 2. Platform-Specific Setup

### Linux (Debian/Ubuntu)

Install the required packages with:

```bash
sudo apt-get update
sudo apt-get install -y build-essential zlib1g-dev libssl-dev libglm-dev cmake ninja-build qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev libqt6websockets6-dev qtkeychain-qt6-dev
```

After installing dependencies, build as usual:

```bash
mkdir build
cd build
cmake -G Ninja -DWITH_WEBSOCKET=ON -DWITH_QTKEYCHAIN=ON -S ..
cmake --build . --parallel
```

---

### macOS

Install dependencies with Homebrew and aqtinstall:

```bash
brew install cmake ninja aqtinstall
aqt install-qt mac desktop 6.5.3 clang_64 -m qtwebsockets
```

Make sure Xcode Command Line Tools are installed:

```bash
xcode-select --install
```

Add Qt to your PATH:

```bash
echo 'export PATH="~/6.5.3/macos/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Then build:

```bash
mkdir build
cd build
cmake -G Ninja -DWITH_WEBSOCKET=ON -DWITH_QTKEYCHAIN=ON -S ..
cmake --build . --parallel
```

---

### Windows

Install:

- Python + `pip install aqtinstall`
- CMake, Ninja, NSIS, 7-Zip
- Visual Studio with *Desktop development with C++*

#### MSVC (Recommended)

```bash
aqt install-qt windows desktop 6.5.3 win64_msvc2019_64 -m qtwebsockets
```

Add `Qt\6.5.3\msvc2019_64\bin` to PATH.

#### MinGW (Alternative)

```bash
aqt install-qt windows desktop 6.5.3 win64_mingw -m qtwebsockets
aqt install-tool windows desktop tools_mingw1310
```

Add both Qt and MingW `bin` directories to PATH.

---

## 3. Build Instructions

From the project root:

```bash
mkdir build
cd build
cmake -G Ninja -DWITH_WEBSOCKET=ON -DWITH_QTKEYCHAIN=ON -S ..
cmake --build . --parallel
```

### Optional: Run Tests

```bash
ctest
```

### Optional: Create Package

```bash
cpack
```

---

## Optional: Code Style Check

Run `clang-format` via Docker:

```bash
docker run --rm --platform linux/amd64 \
  -v "$(pwd):$(pwd)" -w "$(pwd)" \
  ghcr.io/jidicula/clang-format:14 \
  --dry-run --Werror --style=file \
  $(find src tests -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp')
```

---

## CI Reference

Our [GitHub Actions workflow](.github/workflows/ci.yml) defines the tested build environment across platforms.\
If you're troubleshooting local issues, consider comparing your setup with the Docker images or tool versions used in CI.
