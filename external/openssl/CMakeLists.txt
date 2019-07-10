if(OPENSSL_FOUND)
    message(STATUS "OpenSSL ${OPENSSL_VERSION} found: ${OPENSSL_LIBRARIES}")
    if(WIN32)
        # Verify we have the correct OpenSSL version
        if (Qt5Core_VERSION VERSION_LESS 5.12.4 AND (OPENSSL_VERSION VERSION_EQUAL 1.1.0 OR OPENSSL_VERSION VERSION_GREATER 1.1.0))
            message(FATAL_ERROR "Qt versions before 5.12.4 require OpenSSL version 1.0.2 or lower")
        elseif(Qt5Core_VERSION VERSION_GREATER 5.12.3 AND OPENSSL_VERSION VERSION_LESS 1.1.1)
            message(FATAL_ERROR "Qt versions after 5.12.3 require OpenSSL version 1.1.1 or higher")
        endif()
        # Ensure that we package OpenSSL DLLs with MMapper for Windows
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/bin)
        if(NOT OPENSSL_ROOT_DIR)
            if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                if (OPENSSL_VERSION VERSION_LESS 1.1.0)
                    set(OPENSSL_ROOT_DIR "C:\\OpenSSL-Win64")
                else()
                    set(OPENSSL_ROOT_DIR "C:\\OpenSSL-v111-Win64")
                endif()
                set(MSVCR120_ROOT_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\redist\\x64\\Microsoft.VC120.CRT")
            else()
                if (OPENSSL_VERSION VERSION_LESS 1.1.0)
                    set(OPENSSL_ROOT_DIR "C:\\OpenSSL-Win32")
                else()
                    set(OPENSSL_ROOT_DIR "C:\\OpenSSL-v111-Win32")
                endif()
                set(MSVCR120_ROOT_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\redist\\x86\\Microsoft.VC120.CRT")
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
                ${MSVCR120_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(MSVCR120_BIN-NOTFOUND)
            message(FATAL_ERROR "OpenSSL dependency msvcr120.dll was NOT found")
        else()
            configure_file(${MSVCR120_BIN} ${CMAKE_BINARY_DIR}/src/bin/msvcr120.dll COPYONLY)
            message(STATUS "   Copied ${MSVCR120_BIN} to src/bin/")
        endif()
    endif()
elseif(MINGW AND Qt5Core_VERSION VERSION_GREATER 5.12.3)
    message(STATUS "Could not find a local OpenSSL; downloading a precompiled library")
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
else()
    if(WIN32)
        if(Qt5Core_VERSION VERSION_GREATER 5.12.3)
            message(STATUS "OpenSSL NOT Found: download OpenSSL v1.1.1: https://slproweb.com/products/Win32OpenSSL.html")
        else()
            message(STATUS "OpenSSL NOT Found: download OpenSSL v1.0.2: https://slproweb.com/products/Win32OpenSSL.html")
        endif()
        message(STATUS "     or")
    endif()
    message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_OPENSSL=OFF` to build without TLS encryption")
endif()
