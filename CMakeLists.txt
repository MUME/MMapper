cmake_minimum_required(VERSION 3.0.0)
project(mmapper)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# try to find system copy of qtiocompressor
find_path(QTIOCOMPRESSOR_INCLUDE_DIRS qtiocompressor.h PATH_SUFFIXES QtSolutions)
find_library(QTIOCOMPRESSOR_LIBRARIES QtSolutions_IOCompressor-2.3)
if (QTIOCOMPRESSOR_INCLUDE_DIRS AND QTIOCOMPRESSOR_LIBRARIES)
    message(STATUS "Found QtIOCompressor: ${QTIOCOMPRESSOR_LIBRARIES}")
    set(qtiocompressor_SRCS "")
    set(qtiocompressor_MOC_HDRS "")
else()
    message(STATUS "QtIOCompressor not found; using bundled copy.")
    find_package(ZLIB REQUIRED)
    set(QTIOCOMPRESSOR_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/3rdparty/qtiocompressor)
    set(QTIOCOMPRESSOR_LIBRARIES "")
    set(qtiocompressor_SRCS     ${PROJECT_SOURCE_DIR}/3rdparty/qtiocompressor/qtiocompressor.cpp)
    set(qtiocompressor_MOC_HDRS ${PROJECT_SOURCE_DIR}/3rdparty/qtiocompressor/qtiocompressor.h)
endif()

# Version
set(VERSION "2.3.4")
add_definitions(-DMMAPPER_VERSION="${VERSION}" -DWITH_SPLASH)
message(STATUS "MMapper version ${VERSION} (${CMAKE_BUILD_TYPE} distribution)")

# Extract git branch and revision
if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Get the current working branch
    execute_process(
      COMMAND git rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_BRANCH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the latest abbreviated commit hash of the working branch
    execute_process(
      COMMAND git log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(STATUS "Building from branch ${GIT_BRANCH} on commit ${GIT_COMMIT_HASH}")
    add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
    add_definitions(-DGIT_BRANCH="${GIT_BRANCH}")
endif()

if (UNIX AND NOT APPLE)
    SET(SHARE_INSTALL_PREFIX
        "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Base directory for files which go to share/"
    )

    SET(CMAKE_CXX_FLAGS_DEBUG 
      "-g -Wall -g3 -ggdb -gdwarf-2 -Wunused-variable -Wno-long-long -Wno-unknown-pragmas -Wno-system-headers"
      CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
endif (UNIX AND NOT APPLE)

add_subdirectory(src)
