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
option(USE_DISTCC "Use distcc for distributed builds" OFF)

# Apple users are most likely using brew
if(APPLE)
   set(CMAKE_PREFIX_PATH /usr/local/opt/qt5/)
endif()

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
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

if (CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    message(STATUS "Debug build type: ${CMAKE_BUILD_TYPE}")
    set(MMAPPER_IS_DEBUG "YES")
else()
    message(STATUS "Release build type: ${CMAKE_BUILD_TYPE}")
    set(MMAPPER_IS_DEBUG "NO")
endif()

if (MMAPPER_IS_DEBUG)
    set(CMAKE_VERBOSE_MAKEFILE true)

    # common options for gcc and clang
    add_compile_options(-O0)
    add_compile_options(-Wall -Wextra -pedantic)

    # -g     = debugging in OS native format (also note: -g is ignored by the linker)
    # -ggdb  = use gdb debugging format (defaults to -ggdb2)
    # -ggdb3 = adds macro definitions
    add_compile_options(-ggdb3)

    # improved debugging experience
    add_compile_options(-fno-omit-frame-pointer -fno-optimize-sibling-calls)
    add_compile_options(-fno-inline -fno-inline-functions)

    message(STATUS "Forcing QT assertions")
    add_definitions(-DQT_FORCE_ASSERTS)

    message(STATUS "Using _GLIBCXX_DEBUG")
    add_definitions(-D_GLIBCXX_DEBUG) # c++ STL debugging
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Building with GCC (${CMAKE_CXX_COMPILER})")
    if (MMAPPER_IS_DEBUG)
        if (FALSE)
            # This doesn't work on my machine. All the __asan_xxx symbols are undefined.
            message("-- Building with GCC address sanitizer")
            add_compile_options(-fsanitize=address)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        endif()

        add_compile_options(-Waddress)
        add_compile_options(-Wcast-align)
        add_compile_options(-Wcast-qual)
        add_compile_options(-Wconversion)
        add_compile_options(-Wendif-labels)
        add_compile_options(-Wenum-compare)
        add_compile_options(-Wdelete-non-virtual-dtor -Wnon-virtual-dtor)
        add_compile_options(-Wignored-qualifiers)
        add_compile_options(-Winit-self)
        add_compile_options(-Wmultichar)
        add_compile_options(-Wnarrowing)
        add_compile_options(-Wnonnull)
        add_compile_options(-Wnormalized)
        add_compile_options(-Wold-style-cast)
        add_compile_options(-Wparentheses)
        add_compile_options(-Wpointer-arith)
        add_compile_options(-Wredundant-decls)
        add_compile_options(-Wreturn-local-addr)
        add_compile_options(-Wreturn-type)
        add_compile_options(-Wsequence-point)
        add_compile_options(-Wsign-conversion)
        add_compile_options(-Wsign-promo)
        add_compile_options(-Wstrict-aliasing)
        add_compile_options(-Wstrict-null-sentinel)
        add_compile_options(-Wsuggest-override)
        add_compile_options(-Wtype-limits)
        add_compile_options(-Wundef)
        add_compile_options(-Wuninitialized)
        add_compile_options(-Wunused-result)
        add_compile_options(-Wunused-variable)
        add_compile_options(-Wvla)
        add_compile_options(-Wwrite-strings)

        # always ignored
        add_compile_options(-Wno-system-headers)
        # add_compile_options(-Wno-unknown-pragmas)
        # add_compile_options(-Wno-deprecated-declarations)

        # always errors
        add_compile_options(-Werror=double-promotion)
        add_compile_options(-Werror=reorder)
        add_compile_options(-Werror=sign-compare)
    endif(MMAPPER_IS_DEBUG)
    if(APPLE)
        # Minimum version of OS X that supports c++14 is 10.6 "Snow Leopard" with GCC
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.6")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Building with Clang (${CMAKE_CXX_COMPILER})")
    add_compile_options(-fno-standalone-debug)

    if (MMAPPER_IS_DEBUG)
        if (FALSE)
            # segfaults in ASAN initializer on 32-bit Ubuntu 18.04.1 with clang 6.0
            # note: apparently -fsanitize=address can't be used with gnu libc 2.25+
            # Ubuntu 18.04.1 has libc 2.27.
            message(STATUS "Building with Clang address sanitizer")
            add_compile_options(-fsanitize=address)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        endif()

        if (TRUE)
            message(STATUS "Building with Clang undefined behavior sanitizer")
            add_compile_options(-fsanitize=undefined)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
        endif()

        if(TRUE)
            add_compile_options(-Weverything)
            add_compile_options(-Wno-c++98-c++11-compat-binary-literal) # never useful.
            add_compile_options(-Wno-c++98-compat) # never useful.
            add_compile_options(-Wno-c++98-compat-pedantic) # sometimes useful.
            add_compile_options(-Wno-covered-switch-default) # this is never an error; it's just extra info.
            # add_compile_options(-Wno-deprecated-declarations)
            add_compile_options(-Wno-documentation)
            add_compile_options(-Wno-global-constructors -Wno-exit-time-destructors) # these go together.
            add_compile_options(-Wno-padded) # this isn't usually a mistake; it's just extra info.
            add_compile_options(-Wno-redundant-parens) # MOCs spam this.
            add_compile_options(-Wno-shadow-field-in-constructor -Wno-shadow-uncaptured-local) # common idioms
        endif()

        # These should probably be errors, but someone using gcc might add them and break the build.
        add_compile_options(-Wcast-qual) # always a mistake unless you added the qualifier yourself.
        add_compile_options(-Wconversion)
        add_compile_options(-Wdouble-promotion)
        add_compile_options(-Wfloat-conversion)
        add_compile_options(-Wfloat-equal) # this is usually an error.
        add_compile_options(-Winconsistent-missing-destructor-override)
        add_compile_options(-Winconsistent-missing-override)
        add_compile_options(-Wnewline-eof)
        add_compile_options(-Wold-style-cast)
        add_compile_options(-Wpedantic)
        add_compile_options(-Wshorten-64-to-32)
        add_compile_options(-Wsign-conversion) # can warn about hard-to-spot bugs.
        add_compile_options(-Wunused-result) # required for c++17 [[nodiscard]]
        add_compile_options(-Wweak-vtables) # can result in crashes for ODR violations.
    endif(MMAPPER_IS_DEBUG)
    if(APPLE)
        # Minimum version of OS X that supports c++14 is 10.9 "Mavericks" with Clang
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
    endif()
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

message("") # blank line
include(FeatureSummary)
add_feature_info("WITH_ZLIB" WITH_ZLIB "backwards compatability with old .mm2 saves")
add_feature_info("WITH_OPENSSL" WITH_OPENSSL "encrypt connection to MUME with TLS")
add_feature_info("USE_TIDY" USE_TIDY "")
add_feature_info("USE_IWYU" USE_IWYU "")
add_feature_info("USE_DISTCC" USE_DISTCC "")
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled options:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled options:")
