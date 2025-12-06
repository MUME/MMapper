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
