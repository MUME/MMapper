cmake_minimum_required(VERSION 3.0.2)
project(mmapper CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type (default RelWithDebInfo)" FORCE)
endif()

# Version
file(STRINGS "MMAPPER_VERSION" MMAPPER_VERSION)
add_definitions(-DMMAPPER_VERSION="${MMAPPER_VERSION}")
message(STATUS "MMapper version ${MMAPPER_VERSION} (${CMAKE_BUILD_TYPE} distribution)")

# Options
option(WITH_ZLIB "zlib compressed old save backwards compatability" ON)
option(WITH_OPENSSL "Use OpenSSL for TLS encryption" ON)
option(USE_TIDY "Run clang-tidy with the compiler" OFF)
option(USE_IWYU "Run include-what-you-use with the compiler" OFF)

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

# Try to find the system copy of zlib
if(WITH_ZLIB)
    find_package(ZLIB)
    if(ZLIB_FOUND)
        message(STATUS "zlib found: ${ZLIB_LIBRARIES} ${ZLIB_INCLUDE_DIRS}")
    else()
        if(WIN32)
            message(STATUS "Could not find local zlib; building as a static library")
        else()
            message(FATAL_ERROR "zlib NOT found: use `-DWITH_ZLIB=OFF` to build with old map backwards compatability")
        endif()
        include(ExternalProject)
        ExternalProject_Add(zlib
            GIT_REPOSITORY "https://github.com/madler/zlib.git"
            GIT_TAG "v1.2.11"
            SOURCE_DIR "${CMAKE_BINARY_DIR}/zlib-src"
            BINARY_DIR "${CMAKE_BINARY_DIR}/zlib-build"
            INSTALL_COMMAND ""
            CMAKE_ARGS "-DBUILD_SHARED_LIBS=OFF"
            CMAKE_CACHE_ARGS "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
            )
        if(WIN32)
            set(ZLIB_STATIC_LIBRARY_NAME zlibstatic)
        else()
            set(ZLIB_STATIC_LIBRARY_NAME z)
        endif()
        set(ZLIB_STATIC_LIBRARY ${CMAKE_BINARY_DIR}/zlib-build/${CMAKE_STATIC_LIBRARY_PREFIX}${ZLIB_STATIC_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(ZLIB_LIBRARIES ${ZLIB_STATIC_LIBRARY})
        set(ZLIB_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/zlib-src ${CMAKE_BINARY_DIR}/zlib-build)
    endif()
else()
    message(STATUS "Building without zlib")
    add_definitions(/DMMAPPER_NO_ZLIB)
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

        # Get the latest annotated tag of the working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_ANNOTATED_TAG
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        message(STATUS "Building from branch ${GIT_BRANCH} on commit ${GIT_COMMIT_HASH}")
        add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
        add_definitions(-DGIT_BRANCH="${GIT_BRANCH}")
    endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG
      "-ggdb3 -Wall -Wextra -Wunused-variable -Wno-long-long -Wno-unknown-pragmas -Wno-system-headers -Werror=reorder -Wparentheses -Werror=sign-compare -Wconversion -Werror=double-promotion"
      CACHE STRING "Debug builds CMAKE CXX flags" FORCE )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE true)
    add_compile_options( $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:-D_GLIBCXX_DEBUG> )
endif()

# Download arda.mm2
set(MMAPPER_MAP_FILE "${CMAKE_BINARY_DIR}/map/arda.mm2")
if(NOT EXISTS ${MMAPPER_MAP_FILE})
    set(MMAPPER_MAP_URL "https://github.com/MUME/MMapper/releases/download/v${MMAPPER_VERSION}/arda.mm2")
    message(STATUS "Fetching MMapper Map from: ${MMAPPER_MAP_URL}")

    file(DOWNLOAD ${MMAPPER_MAP_URL} ${MMAPPER_MAP_FILE} STATUS MMAPPER_MAP_HTTP_STATUS)
    list(GET MMAPPER_MAP_HTTP_STATUS 0 MMAPPER_MAP_STATUS)

    if(MMAPPER_MAP_STATUS)
        file(REMOVE MMAPPER_MAP_FILE)
        if(GIT_ANNOTATED_TAG)
            set(MMAPPER_MAP_URL "https://github.com/MUME/MMapper/releases/download/${GIT_ANNOTATED_TAG}/arda.mm2")
            message(STATUS "Map not found. Fetching alternative map from: ${MMAPPER_MAP_URL}")
            file(DOWNLOAD ${MMAPPER_MAP_URL} ${MMAPPER_MAP_FILE})
        endif()
    endif()

    message(STATUS "Downloaded map to: ${MMAPPER_MAP_FILE}")
else()
    message(STATUS "Using cached MMapper map: ${MMAPPER_MAP_FILE}")
endif()

add_subdirectory(src)
add_subdirectory(appdata)
add_subdirectory(tests)

include(FeatureSummary)
add_feature_info("WITH_ZLIB" WITH_ZLIB "backwards compatability with old .mm2 saves")
add_feature_info("WITH_OPENSSL" WITH_OPENSSL "encrypt connection to MUME with TLS")
add_feature_info("USE_TIDY" USE_TIDY "")
add_feature_info("USE_IWYU" USE_IWYU "")
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled options:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled options:")
