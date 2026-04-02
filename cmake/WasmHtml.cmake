# cmake/WasmHtml.cmake
# Post-processes the Qt-generated Wasm HTML file:
#   - Injects a favicon link tag
#   - Replaces the Qt logo reference with the MMapper logo
#   - Injects the COI service worker script tag
#   - Copies favicon.svg and logo.svg alongside the HTML
#   - Downloads coi-serviceworker.js alongside the HTML
#
# Usage (as a post-build cmake -P script):
#   cmake -DHTML_FILE=<path/mmapper.html>
#         -DLOGO_SRC=<path/mmapper-hi-release.svg>
#         -DFAVICON_SRC=<path/mmapper-lo-release.svg>
#         -P WasmHtml.cmake

foreach(var HTML_FILE LOGO_SRC FAVICON_SRC)
    if(NOT ${var})
        message(FATAL_ERROR "${var} not specified for WasmHtml.cmake")
    endif()
endforeach()

file(READ "${HTML_FILE}" HTML_CONTENT)

# Inject favicon link and COI service worker script after <head>
string(REPLACE
    "<head>"
    "<head>\n    <link rel=\"icon\" href=\"favicon.svg\" type=\"image/svg+xml\">\n    <script src=\"./coi-serviceworker.js\"></script>"
    HTML_CONTENT "${HTML_CONTENT}"
)

# Replace Qt logo with MMapper logo
string(REPLACE
    "src=\"qtlogo.svg\" width=\"320\" height=\"200\""
    "src=\"logo.svg\""
    HTML_CONTENT "${HTML_CONTENT}"
)

file(WRITE "${HTML_FILE}" "${HTML_CONTENT}")

# Copy assets alongside the HTML (use configure_file for portable rename support)
get_filename_component(OUT_DIR "${HTML_FILE}" DIRECTORY)
configure_file("${LOGO_SRC}" "${OUT_DIR}/logo.svg" COPYONLY)
configure_file("${FAVICON_SRC}" "${OUT_DIR}/favicon.svg" COPYONLY)

# Download coi-serviceworker.js if not already present
set(COI_JS "${OUT_DIR}/coi-serviceworker.js")
if(NOT EXISTS "${COI_JS}")
    message(STATUS "Downloading coi-serviceworker.js...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/gzuidhof/coi-serviceworker/refs/heads/master/coi-serviceworker.js"
        "${COI_JS}"
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_CODE)
    if(NOT DOWNLOAD_CODE EQUAL 0)
        list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
        message(WARNING "Failed to download coi-serviceworker.js: ${DOWNLOAD_ERROR}. "
                        "The Wasm build may not function correctly in cross-origin contexts.")
    endif()
endif()

message(STATUS "Patched Wasm HTML: ${HTML_FILE}")
