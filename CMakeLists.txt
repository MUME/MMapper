cmake_minimum_required(VERSION 2.6)
project(mmapper)

find_package(Qt4 REQUIRED)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTOPENGL TRUE)
include(${QT_USE_FILE})

# Get SVN revision number
find_package(Subversion)
if (Subversion_FOUND)
    if (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
        Subversion_WC_INFO(${PROJECT_SOURCE_DIR} PROJECT)
        message(STATUS "Current SVN revision is ${PROJECT_WC_REVISION}")
        add_definitions(-DSVN_REVISION=${PROJECT_WC_REVISION})
    endif (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
endif (Subversion_FOUND)

if (UNIX)
    SET(SHARE_INSTALL_PREFIX
        "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Base directory for files which go to share/"
    )
endif (UNIX)

add_subdirectory(src)
