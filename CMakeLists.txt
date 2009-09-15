cmake_minimum_required(VERSION 2.6)
project(mmapper)

find_package(Qt4 REQUIRED)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTOPENGL TRUE)
include(${QT_USE_FILE})

if (NOT MINGW)
    find_package(ZLIB REQUIRED)
else (NOT MINGW)
    message(STATUS "Warning: MingW requires zlib to be installed manually")
endif (NOT MINGW)

# Version
set(VERSION "2.1.0")
add_definitions(-DMMAPPER_VERSION="${VERSION}")
message(STATUS "MMapper version ${VERSION} (${CMAKE_BUILD_TYPE} distribution)")

# Get SVN revision number
find_package(Subversion)
if (Subversion_FOUND)
    if (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
        Subversion_WC_INFO(${PROJECT_SOURCE_DIR} PROJECT)
        message(STATUS "Current SVN revision is ${PROJECT_WC_REVISION}")
        add_definitions(-DSVN_REVISION=${PROJECT_WC_REVISION})
	set(SVN_BUILD ${PROJECT_WC_REVISION})
    endif (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
endif (Subversion_FOUND)

if (UNIX AND NOT APPLE)
    SET(SHARE_INSTALL_PREFIX
        "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Base directory for files which go to share/"
    )

    SET(CMAKE_CXX_FLAGS_DEBUG 
      "-g -Wall -g3 -ggdb -gdwarf-2 -Wunused-variable -Wno-long-long -Wno-unknown-pragmas -Wno-system-headers"
      CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
endif (UNIX AND NOT APPLE)

if (APPLE)
    # This is bundle metadata to create "bundles" and "Universal Binaries" on
    # Mac OS X
    # http://www.cmake.org/Wiki/CMake_FAQ#How_do_I_build_universal_binaries_on_Mac_OS_X.3F
    # By the way, there is a bug in the cross-compilation SDKs for
    # Tiger in Leopard, so you need to follow these instructions:
    # http://groups.google.com/group/vim_mac/browse_thread/thread/064da55ec412e768
    # which means typing the following command in Terminal.app:
    # $ sudo ln -s /Developer/SDKs/MacOSX10.4u.sdk/usr/lib/crt1.o  /Developer/SDKs/MacOSX10.4u.sdk/usr/lib/crt1.10.5.o

    # In order to build a universal binary we need the following line:
    set(CMAKE_OSX_ARCHITECTURES ppc;i386)

    # To support much older OSX machines we need the following lines:
    #set(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.4u.sdk)
    #set(CMAKE_FIND_ROOT_PATH /Developer/SDKs/MacOSX10.4u.sdk)
endif (APPLE)

# Visual C++ Parallel Builds
if ("$ENV{QMAKESPEC}" MATCHES "win32-msvc")
    message(STATUS "Enabling parallel builds with the -MP CFLAG")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -MP")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -MP")
endif ("$ENV{QMAKESPEC}" MATCHES "win32-msvc")

add_subdirectory(src)
