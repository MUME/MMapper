cmake_minimum_required(VERSION 3.0.0)
project(mmapper CXX)

set(CMAKE_CXX_STANDARD 14)
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

# Version
file(STRINGS "MMAPPER_VERSION" MMAPPER_VERSION)
add_definitions(-DMMAPPER_VERSION="${MMAPPER_VERSION}" -DWITH_SPLASH)
message(STATUS "MMapper version ${MMAPPER_VERSION} (${CMAKE_BUILD_TYPE} distribution)")

# Options
option(WITH_QTIOCOMPRESSOR "QtIOCompressor old save backwards compatability" ON)
option(WITH_OPENSSL "Use OpenSSL for TLS encryption" ON)

# Apple users are most likely using brew
if(APPLE)
   set(CMAKE_PREFIX_PATH /usr/local/opt/qt5/)
endif()

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Test REQUIRED)

enable_testing()
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Try to find the system copy of QtIOCompressor
if(WITH_QTIOCOMPRESSOR)
    find_path(QTIOCOMPRESSOR_INCLUDE_DIRS qtiocompressor.h PATH_SUFFIXES QtSolutions)
    find_library(QTIOCOMPRESSOR_LIBRARIES QtSolutions_IOCompressor-2.3)
    if(QTIOCOMPRESSOR_INCLUDE_DIRS AND QTIOCOMPRESSOR_LIBRARIES)
        set(QTIOCOMPRESSOR_FOUND 1)
        message(STATUS "Found QtIOCompressor: ${QTIOCOMPRESSOR_LIBRARIES}")
    else()
        set(QTIOCOMPRESSOR_FOUND 0)
        message(STATUS "QtIOCompressor not found: use `-DWITH_QTIOCOMPRESSOR=OFF` to build without old map backwards compatability")
        message(STATUS "Building QtIOCompressor as an object library...")

        # QtIOCompressor requires zlib
        find_package(ZLIB QUIET)
        if(ZLIB_FOUND)
            message(STATUS "Found ZLIB: ${ZLIB_LIBRARIES} ${ZLIB_INCLUDE_DIRS}")
        else()
            # QtIOCompressor will build a static zlib under zlib-build
            if(WIN32)
                set(ZLIB_STATIC_LIBRARY_NAME zlibstatic)
            else()
                set(ZLIB_STATIC_LIBRARY_NAME z)
            endif()
            set(ZLIB_STATIC_LIBRARY ${CMAKE_BINARY_DIR}/zlib-build/${CMAKE_STATIC_LIBRARY_PREFIX}${ZLIB_STATIC_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
            set(ZLIB_LIBRARIES ${ZLIB_STATIC_LIBRARY})
            set(ZLIB_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/zlib-src ${CMAKE_BINARY_DIR}/zlib-build)
        endif()

        # Add QtIOCommpressor sources
        option(SKIP_INSTALL_ALL "Do not install QtIOCompressor libraries or headers" TRUE)
        configure_file(CMakeLists.QtIOCompressor.in QtIOCompressor-download/CMakeLists.txt)
        execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/QtIOCompressor-download" )
        execute_process(COMMAND "${CMAKE_COMMAND}" --build .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/QtIOCompressor-download" )
        add_subdirectory("${CMAKE_BINARY_DIR}/QtIOCompressor-src" "${CMAKE_BINARY_DIR}/QtIOCompressor-build")
        set(QTIOCOMPRESSOR_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/QtIOCompressor-src/src")

        set(QTIOCOMPRESSOR_OBJECTS $<TARGET_OBJECTS:qtiocompressorobj>)
    endif()
else()
    message(STATUS "Building without QtIOCompressor which provides old map backwards compatability")
    add_definitions(/DMMAPPER_NO_QTIOCOMPRESSOR)
endif()

if(WITH_OPENSSL)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        message(STATUS "OpenSSL found: ${OPENSSL_LIBRARIES}")
    else()
        if(WIN32)
            message(STATUS "OpenSSL NOT Found: download Win32 OpenSSL v1.0.2: https://slproweb.com/products/Win32OpenSSL.html")
            message(STATUS "     or")
        endif()
        message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_OPENSSL=OFF` to build without TLS encryption")
    endif()

    # Ensure that we package OpenSSL DLLs with MMapper for Windows
    if(WIN32)
        if(NOT OPENSSL_ROOT_DIR)
            set(OPENSSL_ROOT_DIR "C:\\OpenSSL-Win32")
        endif()
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/bin)
        find_library(OPENSSL_LIBEAY32 NAMES libeay32 PATHS ${OPENSSL_ROOT_DIR})
        if(OPENSSL_LIBEAY32-NOTFOUND)
            message(FATAL_ERROR "OpenSSL libeay32.dll was NOT found")
        else()
            configure_file(${OPENSSL_LIBEAY32} ${CMAKE_CURRENT_BINARY_DIR}/src/bin/libeay32.dll COPYONLY)
            message(STATUS "   Copied ${OPENSSL_LIBEAY32} to src/bin/")
        endif()
        find_library(OPENSSL_SSLEAY32 NAMES ssleay32 PATHS ${OPENSSL_ROOT_DIR})
        if(OPENSSL_SSLEAY32-NOTFOUND)
        message(FATAL_ERROR "OpenSSL ssleay32.dll was NOT found")
        else()
            configure_file(${OPENSSL_SSLEAY32} ${CMAKE_CURRENT_BINARY_DIR}/src/bin/ssleay32.dll COPYONLY)
            message(STATUS "   Copied ${OPENSSL_SSLEAY32} to src/bin/")
        endif()
    endif()
else()
    message(STATUS "Building without OpenSSL TLS encryption")
    add_definitions(/DMMAPPER_NO_OPENSSL)
endif()

# Extract git branch and revision
find_package(Git QUIET)
if(EXISTS ${GIT_EXECUTABLE})
    if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
        # Get the current working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_BRANCH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Get the latest abbreviated commit hash of the working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_COMMIT_HASH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        message(STATUS "Building from branch ${GIT_BRANCH} on commit ${GIT_COMMIT_HASH}")
        add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
        add_definitions(-DGIT_BRANCH="${GIT_BRANCH}")
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(SHARE_INSTALL_PREFIX
        "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Base directory for files which go to share/"
    )

    set(CMAKE_CXX_FLAGS_DEBUG
      "-ggdb3 -Wunused-variable -Wno-long-long -Wno-unknown-pragmas -Wno-system-headers -Wreorder -Wparentheses -Wsign-compare"
      CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
endif(UNIX AND NOT APPLE)

# Download arda.mm2
set(MMAPPER_MAP_FILE "${CMAKE_BINARY_DIR}/map/arda.mm2")
if(NOT EXISTS ${MMAPPER_MAP_FILE})
    set(MMAPPER_MAP_URL "https://github.com/MUME/MMapper/releases/download/v${MMAPPER_VERSION}/arda.mm2")
    message(STATUS "Fetching MMapper Map from: ${MMAPPER_MAP_URL}")

    file(DOWNLOAD
        ${MMAPPER_MAP_URL}
        ${MMAPPER_MAP_FILE}
    )
    message(STATUS "Downloaded map to: ${MMAPPER_MAP_FILE}")
else()
    message(STATUS "Using cached MMapper map: ${MMAPPER_MAP_FILE}")
endif()

add_subdirectory(src)
add_subdirectory(appdata)
add_subdirectory(tests)
