cmake_minimum_required(VERSION 3.19...3.22)
project(mmapper CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type (default RelWithDebInfo)" FORCE)
endif()

# Version
file(STRINGS "MMAPPER_VERSION" MMAPPER_VERSION)
message(STATUS "MMapper version ${MMAPPER_VERSION} (${CMAKE_BUILD_TYPE} distribution)")

# Options
option(WITH_ZLIB "zlib compressed old save backwards compatability" ON)
option(WITH_UPDATER "Check GitHub for new releases" ON)
if(NOT APPLE AND NOT WIN32)
    option(WITH_OPENSSL "Use OpenSSL for TLS encryption" ON)
endif()
option(WITH_WEBSOCKET "Allow connecting to MUME over WebSockets" OFF)
option(WITH_QTKEYCHAIN "Use QtKeychain to securely store your account credentials" OFF)
option(WITH_MAP "Download the default map or use the provided map path" ON)
option(WITH_IMAGES "Download the default images or use the provided images path" ON)
option(WITH_TESTS "Compile unit tests" ON)
option(USE_TIDY "Run clang-tidy with the compiler" OFF)
option(USE_IWYU "Run include-what-you-use with the compiler" OFF)
option(USE_CODE_COVERAGE "Run code coverage reporting" OFF)
if (NOT (WIN32 AND APPLE))
   option(USE_MOLD "Use the Mold linker" OFF)
endif()
if(WIN32 AND MINGW)
   option(WITH_DRMINGW "Include Dr. Mingw crash dumping (Windows only)" ON)
endif()
set(PACKAGE_TYPE "Source" CACHE STRING "Set the package type for update dialog")

set(VALID_PACKAGE_TYPES Source Deb Dmg Nsis AppImage AppX Flatpak Snap Wasm)
set(PACKAGE_TYPE_NORMALIZED "")
foreach(VALID_TYPE ${VALID_PACKAGE_TYPES})
    string(TOLOWER "${VALID_TYPE}" VALID_TYPE_LOWER)
    string(TOLOWER "${PACKAGE_TYPE}" PACKAGE_TYPE_LOWER)
    if(PACKAGE_TYPE_LOWER STREQUAL VALID_TYPE_LOWER)
        set(PACKAGE_TYPE_NORMALIZED ${VALID_TYPE})
        break()
    endif()
endforeach()
if(NOT PACKAGE_TYPE_NORMALIZED)
    message(FATAL_ERROR "Invalid PACKAGE_TYPE: ${PACKAGE_TYPE}. Must be one of: ${VALID_PACKAGE_TYPES}")
else()
    add_compile_definitions(MMAPPER_PACKAGE_TYPE="${PACKAGE_TYPE_NORMALIZED}")
endif()

set(MMAPPER_QT_COMPONENTS Core Widgets Network OpenGL OpenGLWidgets Test)
if(WITH_WEBSOCKET)
    list(APPEND MMAPPER_QT_COMPONENTS WebSockets)
endif()
find_package(Qt6 COMPONENTS ${MMAPPER_QT_COMPONENTS} REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(Qt6Core_FOUND)
    if(Qt6Core_VERSION VERSION_LESS 6.2.0)
        message(FATAL_ERROR "Minimum supported Qt version is 6.2")
    endif()
    add_definitions(/DQT_DISABLE_DEPRECATED_UP_TO=0x060200)
endif()

if(Qt6OpenGL_FOUND)
    file(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckQtGLES30.cpp
        "#include <QOpenGLExtraFunctions>
        #ifndef GL_TRIANGLES
        # error
        #endif
        #ifdef Q_OS_MAC
        # error
        #endif
        int main(int argc, char** argv) { return 0; }")
    try_compile(QT_HAS_GLES
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckQtGLES30.cpp
                    LINK_LIBRARIES Qt6::OpenGL
                    OUTPUT_VARIABLE OUTPUT)
    file(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckQtOpenGL33.cpp
        "#include <QOpenGLFunctions_3_3_Core>
        #ifndef GL_QUADS
        # error
        #endif
        int main(int argc, char** argv) { return 0; }")
    try_compile(QT_HAS_OPENGL
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckQtOpenGL33.cpp
                    LINK_LIBRARIES Qt6::OpenGL
                    OUTPUT_VARIABLE OUTPUT)
    if (NOT QT_HAS_GLES)
        add_definitions(/DMMAPPER_NO_GLES)
    endif()
    if (NOT QT_HAS_OPENGL)
        add_definitions(/DMMAPPER_NO_OPENGL)
    endif()
    if(QT_HAS_GLES AND QT_HAS_OPENGL)
        message(STATUS "QOpenGLExtraFunctions supports both GLES 3.0 and OpenGL 3.3")
    elseif(QT_HAS_GLES)
        message(STATUS "QOpenGLExtraFunctions supports GLES 3.0")
    elseif(QT_HAS_OPENGL)
        message(STATUS "QOpenGLExtraFunctions supports OpenGL 3.3")
    else()
        message(FATAL_ERROR "QOpenGLExtraFunctions does not support GLES 3.0 or OpenGL 3.3")
    endif()
endif()

if(WITH_WEBSOCKET)
    if(NOT Qt6WebSockets_FOUND)
        message(FATAL_ERROR "Qt6 WebSockets module not found: use `-DWITH_WEBSOCKET=OFF` to build without WebSocket support")
    endif()
else()
    message(STATUS "Building without WebSocket support")
    add_definitions(/DMMAPPER_NO_WEBSOCKET)
endif()

if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum OS X deployment version" FORCE)
endif()

if(WIN32)
    # Target Windows 10 and above
    add_definitions(/DNTDDI_VERSION=0x0A000000)
    add_definitions(/D_WIN32_WINNT=0x0A00)
    # Reduce the size of the Windows header files
    add_definitions(/DWIN32_LEAN_AND_MEAN)
endif()

if (USE_MOLD)
    add_link_options(-fuse-ld=mold)
endif()

# Try to find the system copy of zlib
if(WITH_ZLIB)
    if(EMSCRIPTEN)
        add_compile_options(-s USE_ZLIB=1)
    else()
        find_package(ZLIB)
        add_subdirectory(external/zlib)
    endif()
    add_definitions(/DZLIB_CONST)
else()
    message(STATUS "Building without zlib")
    add_definitions(/DMMAPPER_NO_ZLIB)
endif()

if(WITH_OPENSSL)
    find_package(OpenSSL)
    add_subdirectory(external/openssl)
else()
    message(STATUS "Building without OpenSSL TLS encryption")
    add_definitions(/DMMAPPER_NO_OPENSSL)
endif()

if(WITH_QTKEYCHAIN)
    find_package(QtKeychain QUIET)
    add_subdirectory(external/qtkeychain)
else()
    message(STATUS "Building without QtKeyChain to store account credentials")
    add_definitions(/DMMAPPER_NO_QTKEYCHAIN)
endif()

add_subdirectory(external/glm)
add_subdirectory(external/immer)

# Extract git branch and revision
if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    find_package(Git)
    if(GIT_FOUND)
        # Get the current working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_BRANCH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Describe the most recent tag, number of commits since, and abbreviated commit hash
        execute_process(
          COMMAND ${GIT_EXECUTABLE} describe --tags --always --long --exclude beta
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_TAG_COMMIT_HASH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Detect if this checkout is a tag and extract its name if so
        execute_process(
          COMMAND ${GIT_EXECUTABLE} name-rev --name-only --tags --no-undefined HEAD --exclude beta
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_TAG_NAME
          OUTPUT_STRIP_TRAILING_WHITESPACE
          ERROR_QUIET
        )

        # Detect commit date
        execute_process(
          COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=format:%Y-%m-%d
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE MMAPPER_BUILD_DATE
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(GIT_TAG_NAME)
           message(STATUS "Building from branch ${GIT_BRANCH} with tag ${GIT_TAG_NAME}")
           set(MMAPPER_VERSION_STRING "${GIT_TAG_NAME}")
           set(MMAPPER_BETA "false")
        else()
           message(STATUS "Building from branch ${GIT_BRANCH} with ref ${GIT_TAG_COMMIT_HASH}")
           set(MMAPPER_VERSION_STRING "${GIT_TAG_COMMIT_HASH}")
           set(MMAPPER_BETA "true")
        endif()
    endif()
endif()
if(NOT MMAPPER_VERSION_STRING)
    set(MMAPPER_VERSION_STRING "v${MMAPPER_VERSION}")
    set(MMAPPER_BETA "false")
endif()
if(NOT GIT_BRANCH)
    set(GIT_BRANCH "unknown")
endif()
if(NOT MMAPPER_BUILD_DATE)
    string(TIMESTAMP MMAPPER_BUILD_DATE "%Y-%m-%d")
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    message(STATUS "Debug build type: ${CMAKE_BUILD_TYPE}")
    set(MMAPPER_IS_DEBUG "YES")
else()
    message(STATUS "Release build type: ${CMAKE_BUILD_TYPE}")
    set(MMAPPER_IS_DEBUG "NO")
endif()

if(WITH_DRMINGW)
    if(CMAKE_BUILD_TYPE_UPPER MATCHES "^(DEBUG|RELWITHDEBINFO)$")
        add_subdirectory(external/drmingw)
        add_definitions(/DWITH_DRMINGW)
    else()
        message(FATAL_ERROR "Build MISSING debug symbols: use `-DWITH_DRMINGW=OFF` to build without crash reporting")
    endif()
endif()

# Code Coverage Configuration
add_library(coverage_config INTERFACE)
if(USE_CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Enabling code coverage reporting")
    # Add required flags (GCC & LLVM/Clang)
    target_compile_options(coverage_config INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags
    )
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options(coverage_config INTERFACE --coverage)
    else()
        target_link_libraries(coverage_config INTERFACE --coverage)
    endif()
endif()

if(MMAPPER_IS_DEBUG)
    set(CMAKE_VERBOSE_MAKEFILE true)

    # disable all the deprecated APIs in Qt <= 5.15
    add_compile_options(-DQT_DISABLE_DEPRECATED_BEFORE=0x050F00)

    if(NOT MSVC)
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
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Building with GCC (${CMAKE_CXX_COMPILER})")
    if(MMAPPER_IS_DEBUG)
        if(NOT WIN32)
            message(STATUS "-- Building with GCC address sanitizer")
            add_compile_options(-fsanitize=address)
            add_link_options(-fsanitize=address)

	    message(STATUS "Building with GCC undefined behavior sanitizer")
            add_compile_options(-fsanitize=undefined)
            add_link_options(-fsanitize=undefined)
        endif()

        add_compile_options(-Waddress)
        add_compile_options(-Wcast-align)
        add_compile_options(-Wcast-qual)
        add_compile_options(-Wconversion)
        add_compile_options(-Wdelete-non-virtual-dtor -Wnon-virtual-dtor)
        add_compile_options(-Wendif-labels)
        add_compile_options(-Wenum-compare)
        add_compile_options(-Werror=switch)
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
        add_compile_options(-Werror=return-local-addr)
        add_compile_options(-Werror=return-type)
        add_compile_options(-Werror=sign-compare)
    endif(MMAPPER_IS_DEBUG)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Building with Clang (${CMAKE_CXX_COMPILER})")
    add_compile_options(-fno-standalone-debug)

    if(WIN32)
        # Clang on MSYS2 need an additional option to work
        add_compile_options(-femulated-tls)
        add_link_options(-femulated-tls)
    endif()

    if(EMSCRIPTEN)
        add_compile_options(-Werror=bad-function-cast)
        add_compile_options(-Werror=cast-function-type)
    endif()

    if(MMAPPER_IS_DEBUG)
        if(NOT WIN32)
            # segfaults in ASAN initializer on 32-bit Ubuntu 18.04.1 with clang 6.0
            # note: apparently -fsanitize=address can't be used with gnu libc 2.25+
            # Ubuntu 18.04.1 has libc 2.27.
            message(STATUS "Building with Clang address sanitizer")
            add_compile_options(-fsanitize=address)
            add_link_options(-fsanitize=address)
        endif()

        if(TRUE)
            message(STATUS "Building with Clang undefined behavior sanitizer")
            add_compile_options(-fsanitize=undefined)
            add_link_options(-fsanitize=undefined)
        endif()

        if(WIN32)
            # Clang on MSYS2 needs an additional option for UBSAN to work
            add_compile_options(-fsanitize-undefined-trap-on-error)
            add_link_options(-fsanitize-undefined-trap-on-error)
        endif()

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
        # add_compile_options(-Wno-return-std-move-in-c++11)
        add_compile_options(-Wno-unreachable-code-break) # break after [[noreturn]] function is not a mistake; omitting the break is a recipe for future disaster.
        add_compile_options(-Wno-switch-default)
        if(APPLE)
            add_compile_options(-Wno-poison-system-directories)
        endif()

        add_compile_options(-Wno-extra-semi-stmt) # enabled only for src/ directory

        # require explicit template parameters when deduction guides are missing
        add_compile_options(-Werror=ctad-maybe-unsupported)

        # always errors
        add_compile_options(-Werror=cast-qual) # always a mistake unless you added the qualifier yourself.
        add_compile_options(-Werror=conversion)
        add_compile_options(-Werror=double-promotion)
        add_compile_options(-Werror=float-conversion)
        add_compile_options(-Werror=float-equal) # this is usually an error.
        add_compile_options(-Werror=implicit-fallthrough)
        add_compile_options(-Werror=inconsistent-missing-destructor-override)
        add_compile_options(-Werror=inconsistent-missing-override)
        add_compile_options(-Werror=newline-eof)
        add_compile_options(-Werror=old-style-cast)
        add_compile_options(-Werror=pedantic)
        add_compile_options(-Werror=return-stack-address)
        add_compile_options(-Werror=return-type)
        add_compile_options(-Werror=reorder)
        add_compile_options(-Werror=shadow)
        add_compile_options(-Werror=shadow-field-in-constructor)
        add_compile_options(-Werror=shadow-uncaptured-local)
        add_compile_options(-Werror=shorten-64-to-32)
        add_compile_options(-Werror=sign-conversion) # can warn about hard-to-spot bugs.
        add_compile_options(-Werror=switch)
        add_compile_options(-Werror=unused-result) # required for c++17 [[nodiscard]]
        add_compile_options(-Werror=weak-vtables) # can result in crashes for ODR violations.

        # remove noise (most of these cannot be fixed)
        add_compile_options(-Wno-unsafe-buffer-usage)
    endif(MMAPPER_IS_DEBUG)
endif()
if(MSVC)
    add_definitions(-DNOMINMAX)
    # modernize the __cplusplus value
    add_compile_options(/Zc:__cplusplus)
    add_compile_options(/utf-8)
    add_compile_options(/permissive-)
    if(CMAKE_BUILD_TYPE_UPPER MATCHES "^RELWITHDEBINFO$")
	    # improve performance
	    add_compile_options(/Ob2)
	endif()
endif()

# Download arda.mm2
if(WITH_MAP)
    add_subdirectory(external/map)
else()
    message(STATUS "Building without default map as a resource")
    add_definitions(/DMMAPPER_NO_MAP)
endif()

if(WITH_IMAGES)
    add_subdirectory(external/images)
else()
    message(STATUS "Building without default images as a resource")
    add_definitions(/DMMAPPER_NO_IMAGES)
endif()

if(NOT WITH_UPDATER)
    add_definitions(/DMMAPPER_NO_UPDATER)
endif()

add_subdirectory(src)
add_subdirectory(appdata)
if(WITH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message("") # blank line
include(FeatureSummary)
add_feature_info("WITH_ZLIB" WITH_ZLIB "backwards compatability with old .mm2 saves")
add_feature_info("WITH_UPDATER" WITH_UPDATER "check GitHub for new releases")
if(NOT APPLE)
    add_feature_info("WITH_OPENSSL" WITH_OPENSSL "encrypt connections with TLS")
endif()
add_feature_info("WITH_WEBSOCKET" WITH_WEBSOCKET "bypass firewalls and connect over Secure WebSockets")
add_feature_info("WITH_QTKEYCHAIN" WITH_QTKEYCHAIN "account credentials encryption and auto-login")
add_feature_info("WITH_MAP" WITH_MAP "include default map as a resource")
add_feature_info("WITH_IMAGES" WITH_IMAGES "include default images as a resource")
add_feature_info("WITH_TESTS" WITH_TESTS "compile unit tests")
add_feature_info("USE_TIDY" USE_TIDY "")
add_feature_info("USE_IWYU" USE_IWYU "")
add_feature_info("USE_DISTCC" USE_DISTCC "")
add_feature_info("USE_CODE_COVERAGE" USE_CODE_COVERAGE "")
if(NOT (WIN32 AND APPLE))
    add_feature_info("USE_MOLD" USE_MOLD "faster build linking")
endif()
if(WIN32 AND MINGW)
    add_feature_info("WITH_DRMINGW" WITH_DRMINGW "crash logging with Dr. Mingw (Windows only)")
endif()
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled options:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled options:")
