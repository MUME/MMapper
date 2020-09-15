if(MINIUPNPC_FOUND)
    message(STATUS "MiniUPnPc found: ${MINIUPNPC_LIBRARIES} ${MINIUPNPC_INCLUDE_DIR}")
    # Ensure that we package miniupnpc DLLs with MMapper for Windows
    if(WIN32)
        set(MINIUPNPC_SHARED_LIBRARY ${CMAKE_SHARED_LIBRARY_PREFIX}miniupnpc${CMAKE_SHARED_LIBRARY_SUFFIX})
        # Find miniupnpc root directory
        list(GET MINIUPNPC_LIBRARIES -1 MINIUPNPC_DIRECTORY)
        get_filename_component(MINIUPNPC_ROOT_DIR ${MINIUPNPC_DIRECTORY} DIRECTORY)
        string(REGEX REPLACE "(.:[\\/]+[^\\/]+).*" "\\1" MINIUPNPC_ROOT_DIR ${MINIUPNPC_ROOT_DIR})
        find_file(MINIUPNPC_BIN
                NAMES
                ${MINIUPNPC_SHARED_LIBRARY}
                HINTS
                ${MINIUPNPC_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(NOT MINIUPNPC_BIN)
            message(FATAL_ERROR "MiniUPnPc dependency ${MINIUPNPC_SHARED_LIBRARY} was NOT found")
        else()
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/bin)
            configure_file(${MINIUPNPC_BIN} ${CMAKE_BINARY_DIR}/src/bin/${MINIUPNPC_SHARED_LIBRARY} COPYONLY)
            message(STATUS "   Copied ${MINIUPNPC_BIN} to src/bin/")
        endif()
    endif()
else()
    if(WIN32 OR APPLE)
        message(STATUS "Could not find system MiniUPnPc; building from local source tree")
    else()
        message(FATAL_ERROR "MiniUPnPc NOT found: use `-DWITH_MINIUPNPC=OFF` to build without UPnP group manager port forwarding")
    endif()

    include(ExternalProject)
    ExternalProject_Add(miniupnpc
        URL "http://miniupnp.free.fr/files/download.php?file=miniupnpc-2.1.tar.gz"
        URL "https://miniupnp.tuxfamily.org/files/download.php?file=miniupnpc-2.1.tar.gz"
        URL_HASH SHA1=7abef19a5460afad704814d8db0d998781799825

        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/miniupnpc"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/miniupnpc-build"
        INSTALL_COMMAND ""
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release"
        CMAKE_CACHE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                         -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
                         -DUPNPC_BUILD_SAMPLE:BOOL=OFF
                         -DUPNPC_BUILD_TESTS:BOOL=OFF
                         -DUPNPC_BUILD_SHARED:BOOL=ON
                         -DUPNPC_BUILD_STATIC:BOOL=ON

        UPDATE_COMMAND ""
        ALWAYS 0

        BUILD_BYPRODUCTS "<BINARY_DIR>/${CMAKE_SHARED_LIBRARY_PREFIX}miniupnpc${CMAKE_SHARED_LIBRARY_SUFFIX}"
        BUILD_BYPRODUCTS "<BINARY_DIR>/${CMAKE_STATIC_LIBRARY_PREFIX}miniupnpc${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
    ExternalProject_Get_Property(miniupnpc SOURCE_DIR)
    ExternalProject_Get_Property(miniupnpc BINARY_DIR)
    set(MINIUPNPC_SHARED_LIBRARY ${BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}miniupnpc${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(MINIUPNPC_STATIC_LIBRARY ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}miniupnpc${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(MINIUPNPC_LIBRARIES ${MINIUPNPC_SHARED_LIBRARY} ${MINIUPNPC_STATIC_LIBRARY})
    if(APPLE OR MINIUPNPC_PREFER_STATIC_LIB)
        set(MINIUPNPC_LIBRARY ${MINIUPNPC_STATIC_LIBRARY})
    else()
        if(MSVC)
            # Remove this hack once we upgrade to miniUPnPC 2.2
            set(MINIUPNPC_LIBRARY ${MINIUPNPC_STATIC_LIBRARY})
        else()
            set(MINIUPNPC_LIBRARY ${MINIUPNPC_SHARED_LIBRARY})
        endif()
        if(WIN32)
            add_custom_command(
                TARGET miniupnpc
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo Deploying MiniUPnPc binary
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy ${MINIUPNPC_SHARED_LIBRARY} ${CMAKE_BINARY_DIR}/src/bin/
                )
        endif()
    endif()

    message(STATUS "MiniUPnPc library: ${MINIUPNPC_LIBRARY}")
    set(MINIUPNPC_LIBRARIES ${MINIUPNPC_LIBRARIES} PARENT_SCOPE)
    set(MINIUPNPC_LIBRARY ${MINIUPNPC_LIBRARY} PARENT_SCOPE)
    set(MINIUPNPC_INCLUDE_DIR ${SOURCE_DIR}/../ PARENT_SCOPE)
    set(MINIUPNPC_API_VERSION 17 PARENT_SCOPE)
    set(MINIUPNPC_VERSION 2.1 PARENT_SCOPE)
endif()
