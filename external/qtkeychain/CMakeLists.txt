if(QTKEYCHAIN_FOUND)
    message(STATUS "QtKeychain found: ${QTKEYCHAIN_LIBRARIES} ${QTKEYCHAIN_INCLUDE_DIRS}")
else()
    if(WIN32 OR APPLE)
        message(STATUS "Could not find system QtKeychain; building from local source tree")
    else()
        message(FATAL_ERROR "QtKeychain NOT found: use `-DWITH_QTKEYCHAIN=OFF` to build without account autologin")
    endif()

    include(ExternalProject)
    ExternalProject_Add(qtkeychain
        GIT_REPOSITORY "https://github.com/frankosterfeld/qtkeychain.git"
        GIT_TAG "35c4f40186a6ac3260982256e846bb0774ea91de"

        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build"

        INSTALL_COMMAND ""
        CMAKE_ARGS -DBUILD_TRANSLATIONS:BOOL=OFF
                   -DBUILD_TEST_APPLICATION:BOOL=OFF
                   -DBUILD_SHARED_LIBS:BOOL=ON
                   -DCMAKE_BUILD_TYPE:STRING=RelWithDebugInfo
                   -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
        CMAKE_CACHE_ARGS -DCMAKE_PREFIX_PATH:PATH=${Qt5_DIR}

        UPDATE_COMMAND ""
        ALWAYS 0

        BUILD_BYPRODUCTS "<BINARY_DIR>/bin/${CMAKE_SHARED_LIBRARY_PREFIX}qt5keychain${CMAKE_SHARED_LIBRARY_SUFFIX}"
        )
    ExternalProject_Get_Property(qtkeychain SOURCE_DIR)
    ExternalProject_Get_Property(qtkeychain BINARY_DIR)
    set(QTKEYCHAIN_SHARED_LIBRARY ${BINARY_DIR}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}qt5keychain${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_SHARED_LIBRARY})
    set(QTKEYCHAIN_LIBRARY ${QTKEYCHAIN_SHARED_LIBRARY})
    if(WIN32)
        add_custom_command(
            TARGET qtkeychain
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo Deploying QtKeychain binary
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src/bin/
            COMMAND ${CMAKE_COMMAND} -E copy ${QTKEYCHAIN_SHARED_LIBRARY} ${CMAKE_BINARY_DIR}/src/bin/
            )
    endif()

    if (NOT EXISTS "${BINARY_DIR}/qtkeychain/keychain.h")
        # Copy "keychain.h" header files from to the include directory
        add_custom_command(
            TARGET qtkeychain
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_DIR}/qtkeychain/keychain.h ${BINARY_DIR}/qtkeychain
            )
    endif()

    message(STATUS "QtKeychain library: ${QTKEYCHAIN_LIBRARY}")
    set(QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_LIBRARIES} PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY ${QTKEYCHAIN_LIBRARY} PARENT_SCOPE)
    set(QTKEYCHAIN_INCLUDE_DIR ${BINARY_DIR}/qtkeychain/ PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY_DIR ${BINARY_DIR}/bin/ PARENT_SCOPE)
endif()
