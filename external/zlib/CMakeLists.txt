if(ZLIB_FOUND)
    message(STATUS "zlib found: ${ZLIB_LIBRARIES} ${ZLIB_INCLUDE_DIRS}")
    # Ensure that we package zlib DLLs with MMapper for Windows
    if(WIN32)
        set(ZLIB_SHARED_LIBRARY_NAME "zlib1.dll")
        # Find zlib root directory
        list(GET ZLIB_LIBRARIES -1 ZLIB_DIRECTORY)
        get_filename_component(ZLIB_ROOT_DIR ${ZLIB_DIRECTORY} DIRECTORY)
        string(REGEX REPLACE "(.:[\\/]+[^\\/]+).*" "\\1" ZLIB_ROOT_DIR ${ZLIB_ROOT_DIR})
        find_file(ZLIB_BIN
                NAMES
                ${ZLIB_SHARED_LIBRARY_NAME}
                HINTS
                ${ZLIB_ROOT_DIR}
                ${ZLIB_ROOT}
                PATH_SUFFIXES
                bin)
        if(NOT ZLIB_BIN)
            message(FATAL_ERROR "zlib dependency ${ZLIB_SHARED_LIBRARY_NAME} was NOT found")
        else()
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/bin)
            configure_file(${ZLIB_BIN} ${CMAKE_BINARY_DIR}/src/bin/${ZLIB_SHARED_LIBRARY_NAME} COPYONLY)
            message(STATUS "   Copied ${ZLIB_BIN} to src/bin/")
        endif()
    endif()
else()
    if(NOT WIN32)
        message(FATAL_ERROR "zlib NOT found: use `-DWITH_ZLIB=OFF` to build without old map backwards compatability")
    endif()
    if (MSVC)
        set(ZLIB_STATIC_LIBRARY_NAME zlibstatic)
    else()
        set(ZLIB_STATIC_LIBRARY_NAME z)
    endif()
    message(STATUS "Could not find system zlib; building zlib-ng as a static library")
    include(ExternalProject)
    ExternalProject_Add(zlib
        URL "https://github.com/zlib-ng/zlib-ng/archive/refs/tags/2.2.4.tar.gz"
        URL_HASH SHA256=a73343c3093e5cdc50d9377997c3815b878fd110bf6511c2c7759f2afb90f5a3

        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng-build"

        INSTALL_COMMAND ""
        CMAKE_ARGS -DZLIB_COMPAT=ON
                   -DCMAKE_BUILD_TYPE:STRING=Release
                   -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                   -DBUILD_SHARED_LIBS:BOOL=OFF
                   -DWITH_GZFILEOP=OFF
                   -DZLIB_ENABLE_TESTS=OFF
                   -DWITH_SANITIZER=OFF
                   -DWITH_GTEST=OFF
                   -DZLIBNG_ENABLE_TESTS=OFF

        UPDATE_COMMAND ""
        ALWAYS 0

        BUILD_BYPRODUCTS "<BINARY_DIR>/${CMAKE_STATIC_LIBRARY_PREFIX}${ZLIB_STATIC_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
    ExternalProject_Get_Property(zlib SOURCE_DIR)
    ExternalProject_Get_Property(zlib BINARY_DIR)
    set(ZLIB_STATIC_LIBRARY ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${ZLIB_STATIC_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(ZLIB_LIBRARIES ${ZLIB_STATIC_LIBRARY} PARENT_SCOPE)
    set(ZLIB_INCLUDE_DIRS ${SOURCE_DIR} ${BINARY_DIR} PARENT_SCOPE)
endif()
