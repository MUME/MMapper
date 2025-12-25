# AGENTS.md

## Mandatory Build Guidelines

1.  **Environment:** All builds **must** be performed on a **Linux** system.
2.  **Code Style:** All commits **must** strictly adhere to the project's `clang-format` configuration.

---

## Speed Optimization: ccache (Mandatory)

To enable incremental, cached builds and reduce compilation time, `ccache` is **required**.

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

## Code Style Application (Mandatory)

We use `clang-format-18` via Docker to ensure style consistency. Use the following command to automatically format your code.

1. **Directory:** Navigate to the Git root directory.

2. **Execution:** Run this command to perform an in-place edit to all source files with the required style:

    ```bash
    docker run --rm --platform linux/amd64 \
         -v "$(pwd):$(pwd)" -w "$(pwd)" \
         ghcr.io/jidicula/clang-format:18 \
         -i --Werror --style=file \
         $(find src tests -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp')
    ```
