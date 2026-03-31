# cmake/WasmManifest.cmake
# Scans an assets directory and generates a JSON manifest to asset paths.
#
# Usage:
#   cmake -DASSETS_DIR=<path> -DOUTPUT_FILE=<path> -P WasmManifest.cmake
#
# The manifest maps "subdir/basename.ext" for each
# audio (.mp3, .ogg, .opus, .webm) and image (.jpg, .png) file found, plus the
# map file "arda" if present.

if(NOT ASSETS_DIR)
    message(FATAL_ERROR "ASSETS_DIR not specified for WasmManifest.cmake")
endif()
if(NOT OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not specified for WasmManifest.cmake")
endif()

file(GLOB_RECURSE AUDIO_FILES RELATIVE "${ASSETS_DIR}"
    "${ASSETS_DIR}/*.mp3"
    "${ASSETS_DIR}/*.ogg"
    "${ASSETS_DIR}/*.opus"
    "${ASSETS_DIR}/*.webm"
)

file(GLOB_RECURSE IMAGE_FILES RELATIVE "${ASSETS_DIR}"
    "${ASSETS_DIR}/*.jpg"
    "${ASSETS_DIR}/*.png"
)

file(GLOB MAP_FILES RELATIVE "${ASSETS_DIR}"
    "${ASSETS_DIR}/map/arda"
)

set(JSON_CONTENT "[\n")
set(FIRST_ENTRY TRUE)

foreach(rel_path IN LISTS AUDIO_FILES IMAGE_FILES MAP_FILES)
    if(NOT FIRST_ENTRY)
        string(APPEND JSON_CONTENT ",\n")
    endif()
    string(APPEND JSON_CONTENT "  \"${rel_path}\"")
    set(FIRST_ENTRY FALSE)
endforeach()

string(APPEND JSON_CONTENT "\n]\n")

file(WRITE "${OUTPUT_FILE}" "${JSON_CONTENT}")
message(STATUS "Generated manifest: ${OUTPUT_FILE}")


file(SIZE "${OUTPUT_FILE}" _FILE_SIZE)
message(STATUS "Audio: ${AUDIO_FILES}")
message(STATUS "Found Images: ${IMAGE_FILES}")
message(STATUS "Manifest Size: ${_FILE_SIZE}")

if(_FILE_SIZE LESS 10)
    message(FATAL_ERROR "${OUTPUT_FILE} is empty or contains no assets!")
endif()
