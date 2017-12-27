CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(mmapper CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(default_build_type "Debug")

# Version
FILE(STRINGS "MMAPPER_VERSION" MMAPPER_VERSION)
ADD_DEFINITIONS(-DMMAPPER_VERSION="${MMAPPER_VERSION}" -DWITH_SPLASH)
MESSAGE(STATUS "MMapper version ${MMAPPER_VERSION} (${CMAKE_BUILD_TYPE} distribution)")

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

# try to find system copy of qtiocompressor
FIND_PATH(QTIOCOMPRESSOR_INCLUDE_DIRS qtiocompressor.h PATH_SUFFIXES QtSolutions)
FIND_LIBRARY(QTIOCOMPRESSOR_LIBRARIES QtSolutions_IOCompressor-2.3)
IF(QTIOCOMPRESSOR_INCLUDE_DIRS AND QTIOCOMPRESSOR_LIBRARIES)
    MESSAGE(STATUS "Found QtIOCompressor: ${QTIOCOMPRESSOR_LIBRARIES}")
ELSE()
    MESSAGE(STATUS "QtIOCompressor not found; building embedded library.")
    INCLUDE(ExternalProject)
    ExternalProject_Add(
      QtIOCompressor

      GIT_REPOSITORY "https://github.com/nschimme/QtIOCompressor.git"
      GIT_TAG "master"

      SOURCE_DIR "${CMAKE_BINARY_DIR}/QtIOCompressor-src"
      BINARY_DIR "${CMAKE_BINARY_DIR}/QtIOCompressor-build"
      CMAKE_ARGS "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
      INSTALL_COMMAND ""
    )
    SET(QTIOCOMPRESSOR_LIBRARY ${CMAKE_BINARY_DIR}/QtIOCompressor-build/${CMAKE_SHARED_LIBRARY_PREFIX}QtSolutions_IOCompressor-2.3${CMAKE_SHARED_LIBRARY_SUFFIX})
    SET(QTIOCOMPRESSOR_LIBRARIES ${QTIOCOMPRESSOR_LIBRARY})
    ADD_LIBRARY(QtSolutions_IOCompressor-2.3 SHARED IMPORTED)
    SET_TARGET_PROPERTIES(QtSolutions_IOCompressor-2.3 PROPERTIES IMPORTED_LOCATION ${QTIOCOMPRESSOR_LIBRARY})

    SET(QTIOCOMPRESSOR_SRC_DIR ${CMAKE_BINARY_DIR}/QtIOCompressor-src/src)
    SET(QTIOCOMPRESSOR_INCLUDE_DIRS ${QTIOCOMPRESSOR_SRC_DIR} ${CMAKE_BINARY_DIR}/QtIOCompressor-build)

    # Linux/Apple needs an additional relative path for bundling the library
    IF(UNIX)
        IF(APPLE)
            SET(QTIOCOMPRESSOR_LIBRARIES -Wl,-install_name,@rpath/ ${QTIOCOMPRESSOR_LIBRARY})
        ELSE()
            SET(QTIOCOMPRESSOR_LIBRARIES -Wl,-rpath,../lib ${QTIOCOMPRESSOR_LIBRARY})
        ENDIF()
    ENDIF()
ENDIF()

# Extract git branch and revision
FIND_PACKAGE(Git)
IF(EXISTS ${GIT_EXECUTABLE})
    IF(EXISTS "${PROJECT_SOURCE_DIR}/.git")
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
