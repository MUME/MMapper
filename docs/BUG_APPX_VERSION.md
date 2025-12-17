# Bug: AppX build fails on non-tagged commits (FIXED)

## Summary

The `build-appx` CI job was failing on PRs and non-tagged commits due to malformed version string.

## Error

```
-- AppX Manifest Version: ___0
MakeAppx : error: App manifest validation error: '...0' violates pattern constraint
```

## Root Cause

When `GIT_TAG_COMMIT_HASH` is just a commit hash (e.g., `cd668441`) with no version info:
1. The version parsing regex expects `X.X.X` format
2. `cd668441` has no dots, so the regex fails completely
3. `CMAKE_MATCH_1/2/3` become empty
4. `APPX_MANIFEST_VERSION` = `"" . "" . "" . "0"` = `"___0"`

## Fix Applied

Added fallback to `MMAPPER_VERSION` when parsing fails:

```cmake
# If parsing failed (e.g., version is just a commit hash), fall back to MMAPPER_VERSION
if(NOT CPACK_PACKAGE_VERSION_MAJOR)
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" _ "${MMAPPER_VERSION}")
    set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
    set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
endif()
```
