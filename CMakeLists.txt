CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(mmapper CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(default_build_type "Debug")

# Version
FILE(STRINGS "MMAPPER_VERSION" MMAPPER_VERSION)
ADD_DEFINITIONS(-DMMAPPER_VERSION="${MMAPPER_VERSION}" -DWITH_SPLASH)
MESSAGE(STATUS "MMapper version ${MMAPPER_VERSION} (${CMAKE_BUILD_TYPE} distribution)")

# Options
OPTION(WITH_QTIOCOMPRESSOR "QtIOCompressor old save backwards compatability" ON)

# Apple users are most likely using brew
IF(APPLE)
   SET(CMAKE_PREFIX_PATH /usr/local/opt/qt5/)
ENDIF()

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Xml REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5Test REQUIRED)

ENABLE_TESTING()
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)

# Try to find the system copy of QtIOCompressor
IF(WITH_QTIOCOMPRESSOR)
    FIND_PATH(QTIOCOMPRESSOR_INCLUDE_DIRS qtiocompressor.h PATH_SUFFIXES QtSolutions)
    FIND_LIBRARY(QTIOCOMPRESSOR_LIBRARIES QtSolutions_IOCompressor-2.3)
    IF(QTIOCOMPRESSOR_INCLUDE_DIRS AND QTIOCOMPRESSOR_LIBRARIES)
        SET(QTIOCOMPRESSOR_FOUND 1)
        SET(QTIOCOMPRESSOR_INTERNAL "")
        MESSAGE(STATUS "Found QtIOCompressor: ${QTIOCOMPRESSOR_LIBRARIES}")
    ELSE()
        SET(QTIOCOMPRESSOR_FOUND 0)
        MESSAGE(STATUS "QtIOCompressor not found: use `-DWITH_QTIOCOMPRESSOR=OFF` to build without old map backwards compatability")
        MESSAGE(STATUS "Building QtIOCompressor as an object library...")

        # QtIOCompressor requires zlib
        FIND_PACKAGE(ZLIB QUIET)
        IF(ZLIB_FOUND)
            MESSAGE(STATUS "Found ZLIB: ${ZLIB_LIBRARIES} ${ZLIB_INCLUDE_DIRS}")
        ELSE()
            # QtIOCompressor will build a static zlib under zlib-build
            IF(WIN32)
                SET(ZLIB_STATIC_LIBRARY_NAME zlibstatic)
            ELSE()
                SET(ZLIB_STATIC_LIBRARY_NAME z)
            ENDIF()
            SET(ZLIB_STATIC_LIBRARY ${CMAKE_BINARY_DIR}/zlib-build/${CMAKE_STATIC_LIBRARY_PREFIX}${ZLIB_STATIC_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
            SET(ZLIB_LIBRARIES ${ZLIB_STATIC_LIBRARY})
            SET(ZLIB_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/zlib-src ${CMAKE_BINARY_DIR}/zlib-build)
        ENDIF()

        # Add QtIOCommpressor sources
        OPTION(SKIP_INSTALL_ALL "Do not install QtIOCompressor libraries or headers" TRUE)
        CONFIGURE_FILE(CMakeLists.QtIOCompressor.in QtIOCompressor-download/CMakeLists.txt)
        EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/QtIOCompressor-download" )
        EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" --build .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/QtIOCompressor-download" )
        ADD_SUBDIRECTORY("${CMAKE_BINARY_DIR}/QtIOCompressor-src" "${CMAKE_BINARY_DIR}/QtIOCompressor-build")
        SET(QTIOCOMPRESSOR_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/QtIOCompressor-src/src")

        SET(QTIOCOMPRESSOR_OBJECTS $<TARGET_OBJECTS:qtiocompressorobj>)
    ENDIF()
ELSE()
    MESSAGE(STATUS "Building without QtIOCompressor which provides old map backwards compatability")
    ADD_DEFINITIONS(/DMMAPPER_NO_QTIOCOMPRESSOR)
ENDIF()

# Extract git branch and revision
FIND_PACKAGE(Git QUIET)
IF(EXISTS ${GIT_EXECUTABLE})
    IF(EXISTS "${PROJECT_SOURCE_DIR}/.git")
        # Get the current working branch
        EXECUTE_PROCESS(
          COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_BRANCH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Get the latest abbreviated commit hash of the working branch
        EXECUTE_PROCESS(
          COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE GIT_COMMIT_HASH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        MESSAGE(STATUS "Building from branch ${GIT_BRANCH} on commit ${GIT_COMMIT_HASH}")
        ADD_DEFINITIONS(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
        ADD_DEFINITIONS(-DGIT_BRANCH="${GIT_BRANCH}")
    ENDIF()
ENDIF()

IF(UNIX AND NOT APPLE)
    SET(SHARE_INSTALL_PREFIX
        "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Base directory for files which go to share/"
    )

    SET(CMAKE_CXX_FLAGS_DEBUG
      "-g -Wall -g3 -ggdb -gdwarf-2 -Wunused-variable -Wno-long-long -Wno-unknown-pragmas -Wno-system-headers"
      CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
ENDIF(UNIX AND NOT APPLE)

# Download arda.mm2
SET(MMAPPER_MAP_FILE "${CMAKE_BINARY_DIR}/map/arda.mm2")
IF(NOT EXISTS ${MMAPPER_MAP_FILE})
    SET(MMAPPER_MAP_URL "https://github.com/MUME/MMapper/releases/download/v${MMAPPER_VERSION}/arda.mm2")
    MESSAGE(STATUS "Fetching MMapper Map from: ${MMAPPER_MAP_URL}")

    FILE(DOWNLOAD
        ${MMAPPER_MAP_URL}
        ${MMAPPER_MAP_FILE}
    )
    MESSAGE(STATUS "Downloaded map to: ${MMAPPER_MAP_FILE}")
ELSE()
    MESSAGE(STATUS "Using cached MMapper map: ${MMAPPER_MAP_FILE}")
ENDIF()

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(appdata)
ADD_SUBDIRECTORY(tests)
