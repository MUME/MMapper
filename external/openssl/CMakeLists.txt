if(OPENSSL_FOUND)
    message(STATUS "OpenSSL found: ${OPENSSL_LIBRARIES}")
    if(WIN32)
        # Ensure that we package OpenSSL DLLs with MMapper for Windows
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/bin)
        if(NOT OPENSSL_ROOT_DIR)
            if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                set(OPENSSL_ROOT_DIR "C:\\OpenSSL-Win64")
            else()
                set(OPENSSL_ROOT_DIR "C:\\OpenSSL-Win32")
            endif()
        endif()
        find_file(LIB_EAY_BIN
                NAMES
                libeay32.dll
                HINTS
                ${OPENSSL_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(LIB_EAY_BIN-NOTFOUND)
            message(FATAL_ERROR "OpenSSL libeay32.dll was NOT found")
        else()
            configure_file(${LIB_EAY_BIN} ${CMAKE_BINARY_DIR}/src/bin/libeay32.dll COPYONLY)
            message(STATUS "   Copied ${LIB_EAY_BIN} to src/bin/")
        endif()
        find_file(SSL_EAY_BIN
                NAMES
                ssleay32.dll
                HINTS
                ${OPENSSL_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(SSL_EAY_BIN-NOTFOUND)
            message(FATAL_ERROR "OpenSSL ssleay32.dll was NOT found")
        else()
            configure_file(${SSL_EAY_BIN} ${CMAKE_BINARY_DIR}/src/bin/ssleay32.dll COPYONLY)
            message(STATUS "   Copied ${SSL_EAY_BIN} to src/bin/")
        endif()
        # Shining Light Productions OpenSSL requires the VC++2013 runtime
        find_file(MSVCR120_BIN
                NAMES
                msvcr120.dll
                HINTS
                ${OPENSSL_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(MSVCR120_BIN-NOTFOUND)
            message(FATAL_ERROR "OpenSSL dependency msvcr120.dll was NOT found")
        else()
            configure_file(${SSL_EAY_BIN} ${CMAKE_BINARY_DIR}/src/bin/msvcr120.dll COPYONLY)
            message(STATUS "   Copied ${MSVCR120_BIN} to src/bin/")
        endif()
    endif()
else()
    if(WIN32)
        message(STATUS "OpenSSL NOT Found: download OpenSSL v1.0.2: https://slproweb.com/products/Win32OpenSSL.html")
        message(STATUS "     or")
    endif()
    message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_OPENSSL=OFF` to build without TLS encryption")

    # Note: Qt 5.12 is still being built against OpenSSL 1.0.2 so the below DLLs won't work :(
    # Consider re-enabling it at a later time
    if(NOT WIN32)
        message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_OPENSSL=OFF` to build without TLS encryption")
    else()
        message(STATUS "Could not find local OpenSSL; downloading a precompiled library")
        if(MINGW)
            include(ExternalProject)
            set(OPENSSL_VERSION 1.1.1a)
            if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                ExternalProject_Add(openssl
                    URL https://bintray.com/vszakats/generic/download_file?file_path=openssl-${OPENSSL_VERSION}-win64-mingw.zip
                    URL_HASH SHA1=dcb33213d739d01ffa892ee39516add37f995129

                    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-src"
                    INSTALL_DIR "${CMAKE_BINARY_DIR}/src/bin"
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ${CMAKE_COMMAND} -E echo Deploying OpenSSL 64-bit binary and dependencies
                    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libcrypto-1_1-x64.dll ${CMAKE_BINARY_DIR}/src/bin/
                            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libssl-1_1-x64.dll ${CMAKE_BINARY_DIR}/src/bin/

                    UPDATE_COMMAND ""
                    ALWAYS 0
                )
            else()
                ExternalProject_Add(openssl
                    URL https://bintray.com/vszakats/generic/download_file?file_path=openssl-${OPENSSL_VERSION}-win32-mingw.zip
                    URL_HASH SHA1=632c92a048bca7aa35ba07c2db514bf9a10907a3

                    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-src"
                    INSTALL_DIR "${CMAKE_BINARY_DIR}/src/bin"
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ${CMAKE_COMMAND} -E echo Deploying OpenSSL 32-bit binary and dependencies
                    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libcrypto-1_1.dll ${CMAKE_BINARY_DIR}/src/bin/
                            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libssl-1_1.dll ${CMAKE_BINARY_DIR}/src/bin/

                    UPDATE_COMMAND ""
                    ALWAYS 0
                )
            endif()
            set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-src/include" PARENT_SCOPE)
            set(OPENSSL_LIBRARIES
                ${CMAKE_CURRENT_BINARY_DIR}/openssl-src/lib/libcrypto.dll.a
                ${CMAKE_CURRENT_BINARY_DIR}/openssl-src/lib/libssl.dll.a
                PARENT_SCOPE)
            set(OPENSSL_VERSION ${OPENSSL_VERSION} PARENT_SCOPE)
        endif()
    endif()
endif()
