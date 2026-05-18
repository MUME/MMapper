if(QTKEYCHAIN_FOUND)
    message(STATUS "QtKeychain found: ${QTKEYCHAIN_LIBRARIES} ${QTKEYCHAIN_INCLUDE_DIRS}")
else()
    if(WIN32 OR APPLE OR EMSCRIPTEN)
        message(STATUS "Could not find system QtKeychain; building from local source tree")
    else()
        message(FATAL_ERROR "QtKeychain NOT found: use `-DWITH_QTKEYCHAIN=OFF` to build without account autologin")
    endif()

    set(QTKEYCHAIN_LIBRARY_NAME qt6keychain)
    if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(QTKEYCHAIN_LIBRARY_NAME "${QTKEYCHAIN_LIBRARY_NAME}d")
    endif()

    set(QTKEYCHAIN_SHARED_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/bin/${CMAKE_SHARED_LIBRARY_PREFIX}${QTKEYCHAIN_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    if (MSVC OR EMSCRIPTEN)
        set(QTKEYCHAIN_LIBRARY
            "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${QTKEYCHAIN_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    else()
        set(QTKEYCHAIN_LIBRARY "${QTKEYCHAIN_SHARED_LIBRARY}")
    endif()

    set(QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_LIBRARY})
    set(QTKEYCHAIN_INCLUDE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/qtkeychain/" PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/bin/" PARENT_SCOPE)

    if(EXISTS "${QTKEYCHAIN_SHARED_LIBRARY}")
        message(STATUS "QtKeychain already built: ${QTKEYCHAIN_SHARED_LIBRARY}")
        add_custom_target(qtkeychain_repo)
        add_custom_target(qtkeychain)
        add_dependencies(qtkeychain qtkeychain_repo)
    else()
        include(ExternalProject)

        set(QTKEYCHAIN_EXTRA_ARGS -DBUILD_SHARED_LIBS:BOOL=ON)
        if(EMSCRIPTEN)
            set(QTKEYCHAIN_EXTRA_ARGS
                -DBUILD_SHARED_LIBS:BOOL=OFF
                -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                -DQT_HOST_PATH=${QT_HOST_PATH}
            )
        endif()

        ExternalProject_Add(qtkeychain_repo
            GIT_REPOSITORY "https://github.com/frankosterfeld/qtkeychain.git"
            GIT_TAG "708d09927d52743c801c1a974d432f19266f2fb1"

            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-src"
            BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build"

            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/qtkeychain/keychain.h <BINARY_DIR>/qtkeychain
            CMAKE_ARGS -DBUILD_WITH_QT6:BOOL=ON
                       -DBUILD_TRANSLATIONS:BOOL=OFF
                       -DBUILD_TEST_APPLICATION:BOOL=OFF
                       -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                       -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
                       ${QTKEYCHAIN_EXTRA_ARGS}
            CMAKE_CACHE_ARGS -DCMAKE_PREFIX_PATH:PATH=${Qt6_DIR}

            UPDATE_COMMAND ""
            BUILD_ALWAYS 0

            BUILD_BYPRODUCTS "${QTKEYCHAIN_SHARED_LIBRARY}"
                             "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${QTKEYCHAIN_LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
            )

        add_custom_target(qtkeychain)
        if(WIN32)
            if(MSVC)
                get_target_property(QT6_CORE_LOCATION Qt6::Core LOCATION)
                get_filename_component(QTKEYCHAIN_WIN_DEPLOY_DEST "${QT6_CORE_LOCATION}" DIRECTORY)
            else()
                set(QTKEYCHAIN_WIN_DEPLOY_DEST "${CMAKE_BINARY_DIR}/src/bin/")
            endif()
            add_custom_command(
                OUTPUT "${QTKEYCHAIN_WIN_DEPLOY_DEST}"
                COMMAND ${CMAKE_COMMAND} -E echo "Copying QtKeychain DLL to runtime output directory"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QTKEYCHAIN_SHARED_LIBRARY}" "${QTKEYCHAIN_WIN_DEPLOY_DEST}"
                DEPENDS qtkeychain_repo
                COMMENT "Windows-only: copying ${QTKEYCHAIN_LIBRARY_NAME} DLL"
            )
            add_custom_target(qtkeychain_copy DEPENDS "${QTKEYCHAIN_WIN_DEPLOY_DEST}")
            add_dependencies(qtkeychain qtkeychain_copy)
        endif()
        add_dependencies(qtkeychain qtkeychain_repo)
    endif()

    message(STATUS "QtKeychain library: ${QTKEYCHAIN_LIBRARY}")
    set(QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_LIBRARIES} PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY ${QTKEYCHAIN_LIBRARY} PARENT_SCOPE)
    set(QTKEYCHAIN_INCLUDE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/qtkeychain/" PARENT_SCOPE)
    set(QTKEYCHAIN_LIBRARY_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-build/bin/" PARENT_SCOPE)
endif()
