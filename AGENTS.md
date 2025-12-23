# AGENTS.md

## Mandatory Build Guidelines

1.  **Environment:** All builds **must** be performed on a **Linux** system.
2.  **Documentation:** Refer to **BUILD.md** for core steps.

---

## Speed Optimization: ccache (Mandatory)

To enable fast, cached builds, the following steps are **required**:

1.  **Install:** Ensure `ccache` is installed on Linux.
    * *Example:* `sudo apt-get install -y ccache`
2.  **CMake Configuration:** The initial `cmake` command **must** use the Debug build type and include the ccache launcher flags:

    ```bash
    cmake -B build/ \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
          -DCMAKE_C_COMPILER_LAUNCHER=ccache \
          ... [Other flags from BUILD.md]
    ```

---

## Code Formatting (clang-format)

CI uses Ubuntu clang-format 18.1.8 via Docker. To run the exact same check locally:

```bash
# Check a specific file
docker run --platform linux/amd64 --rm -v "$(pwd)":/src --entrypoint /bin/sh \
  ghcr.io/jidicula/clang-format:18 -c \
  "clang-format --dry-run -Werror -style=file /src/tests/TestHotkeyManager.cpp && echo '✓ Formatting OK'" \
  || echo '✗ Formatting errors found'

# Auto-fix a file
docker run --platform linux/amd64 --rm -v "$(pwd)":/src --entrypoint /bin/sh \
  ghcr.io/jidicula/clang-format:18 -c \
  "clang-format -i -style=file /src/tests/TestHotkeyManager.cpp && echo '✓ File formatted'"

# Check all src and tests files
docker run --platform linux/amd64 --rm -v "$(pwd)":/src --entrypoint /bin/sh \
  ghcr.io/jidicula/clang-format:18 -c \
  "find /src/src /src/tests -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror -style=file && echo '✓ All files OK'" \
  || echo '✗ Formatting errors found'
```

Note: `--platform linux/amd64` is required on ARM Macs for x86 emulation.

Alternative (faster but may differ slightly): `brew install llvm@18` then use `/opt/homebrew/opt/llvm@18/bin/clang-format`
