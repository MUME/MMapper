# WebAssembly Build Guide

## First-Time Setup

### 1. Install Emscripten SDK
```bash
cd ~/dev  # or any directory you prefer
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 3.1.25
./emsdk activate 3.1.25
```

### 2. Install Qt WebAssembly
```bash
brew install aqtinstall
cd <MMAPPER_ROOT>  # your MMapper source directory
aqt install-qt mac desktop 6.5.3 wasm_multithread -m qtwebsockets -O .
```

### 3. Create build script
Save as `build-wasm.sh` in MMapper root:
```bash
#!/bin/bash
set -e

# Source Emscripten environment
# Adjust path if you installed emsdk elsewhere
source "$HOME/dev/emsdk/emsdk_env.sh"

# Paths - adjust these to match your setup
MMAPPER_SRC="<MMAPPER_ROOT>"              # e.g., /Users/yourname/dev/MMapper
QT_WASM="$MMAPPER_SRC/6.5.3/wasm_multithread"
QT_HOST="$MMAPPER_SRC/6.5.3/macos"

"$QT_WASM/bin/qt-cmake" \
    -S "$MMAPPER_SRC" \
    -B "$MMAPPER_SRC/build-wasm" \
    -DQT_HOST_PATH="$QT_HOST" \
    -DWITH_OPENSSL=OFF \
    -DWITH_TESTS=OFF \
    -DWITH_WEBSOCKET=ON \
    -DWITH_UPDATER=OFF \
    -DCMAKE_BUILD_TYPE=Release

# Build with limited parallelism to avoid system slowdown
# --parallel N uses N CPU cores. Omit the number to use all cores.
# Reduce N if your system becomes unresponsive during build.
cmake --build "$MMAPPER_SRC/build-wasm" --parallel 4
```

```bash
chmod +x build-wasm.sh
```

### 4. Create server script
Save as `server.py` in MMapper root:
```python
import http.server
import socketserver

PORT = 9742

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Required headers for SharedArrayBuffer (WASM multithreading)
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        http.server.SimpleHTTPRequestHandler.end_headers(self)

with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
    print(f"Serving at http://localhost:{PORT}/mmapper.html")
    httpd.serve_forever()
```

---

## Daily Use (Everything Installed)

### Build
```bash
cd <MMAPPER_ROOT>
./build-wasm.sh
```

### Run
```bash
cd build-wasm/src
python3 ../../server.py
```

### Open
```
http://localhost:9742/mmapper.html
```

### Clean rebuild
```bash
rm -rf build-wasm && ./build-wasm.sh
```

---

## Path Reference

| Placeholder | Description | Example |
|-------------|-------------|---------|
| `<MMAPPER_ROOT>` | MMapper source directory | `/Users/yourname/dev/MMapper` |
| `$HOME/dev/emsdk` | Emscripten SDK location | `~/dev/emsdk` |
| `6.5.3/wasm_multithread` | Qt WASM installed by aqt | Created inside `<MMAPPER_ROOT>` |
| `6.5.3/macos` | Qt native macOS (host tools) | Created inside `<MMAPPER_ROOT>` |
