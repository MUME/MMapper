if(QTKEYCHAIN_FOUND)
    message(STATUS "QtKeychain found: ${QTKEYCHAIN_LIBRARIES} ${QTKEYCHAIN_INCLUDE_DIRS}")
else()
    if(WIN32 OR APPLE)
        message(STATUS "Could not find system QtKeychain; building from local source tree")
    else()
        message(FATAL_ERROR "QtKeychain NOT found: use `-DWITH_QTKEYCHAIN=OFF` to build without account autologin")
    endif()
    set(QTKEYCHAIN_LIBRARY_NAME qt5keychain)
    if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(QTKEYCHAIN_LIBRARY_NAME "${QTKEYCHAIN_LIBRARY_NAME}d")
    endif()
    set(QTKEYCHAIN_SHARED_FILENAME ${CMAKE_SHARED_LIBRARY_PREFIX}${QTKEYCHAIN_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    include(ExternalProject)
    ExternalProject_Add(qtkeychain
        GIT_REPOSITORY "https://github.com/frankosterfeld/qtkeychain.git"
        GIT_TAG "35c4f40186a6ac3260982256e846bb0774ea91de"

        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build"

        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/qtkeychain/keychain.h <BINARY_DIR>/qtkeychain
        CMAKE_ARGS -DBUILD_TRANSLATIONS:BOOL=OFF
                   -DBUILD_TEST_APPLICATION:BOOL=OFF
                   -DBUILD_SHARED_LIBS:BOOL=ON
                   -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                   -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
        CMAKE_CACHE_ARGS -DCMAKE_PREFIX_PATH:PATH=${Qt5_DIR}

        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        UPDATE_COMMAND ""
        ALWAYS 0

        BUILD_BYPRODUCTS "<BINARY_DIR>/bin/${QTKEYCHAIN_SHARED_FILENAME}"
                         "<BINARY_DIR>/lib/${QTKEYCHAIN_LIBRARY_NAME}.lib"
        )
    ExternalProject_Get_Property(qtkeychain SOURCE_DIR)
    ExternalProject_Get_Property(qtkeychain BINARY_DIR)
    set(QTKEYCHAIN_SHARED_LIBRARY ${BINARY_DIR}/bin/${QTKEYCHAIN_SHARED_FILENAME})
    if (MSVC)
        set(QTKEYCHAIN_LIBRARY ${BINARY_DIR}/lib/${QTKEYCHAIN_LIBRARY_NAME}.lib)
    else()
        set(QTKEYCHAIN_LIBRARY ${QTKEYCHAIN_SHARED_LIBRARY})
    endif()
    set(QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_LIBRARY})
    if(WIN32)
        if (MSVC)
            # windeployqt needs this in the Qt5 bin directory on MSVC
            add_custom_command(
                TARGET qtkeychain
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QTKEYCHAIN_SHARED_LIBRARY} ${Qt5_DIR}/../../../bin/
                )
        else()
            add_custom_command(
                TARGET qtkeychain
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo Deploying QtKeychain binary
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QTKEYCHAIN_SHARED_LIBRARY} ${CMAKE_BINARY_DIR}/src/bin/
                )
        endif()
    endif()

    message(STATUS "QtKeychain library: ${QTKEYCHAIN_LIBRARY}")
    set(QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_LIBRARIES} PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY ${QTKEYCHAIN_LIBRARY} PARENT_SCOPE)
    set(QTKEYCHAIN_INCLUDE_DIR ${BINARY_DIR}/qtkeychain/ PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY_DIR ${BINARY_DIR}/bin/ PARENT_SCOPE)
endif()
