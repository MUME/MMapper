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
        if (OPENSSL_VERSION VERSION_LESS 1.1.0)
            set(OPENSSL_CRYPTO_LIBRARY_FILE "libeay32.dll")
            set(OPENSSL_SSL_LIBRARY_FILE "ssleay32.dll")
            set(MSVCR_LIBRARY_FILE "msvcr120.dll")
            if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                set(MSVCR_ROOT_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\redist\\x64\\Microsoft.VC120.CRT")
            else()
                set(MSVCR_ROOT_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\redist\\x86\\Microsoft.VC120.CRT")
            endif()
        else()
            set(MSVCR_LIBRARY_FILE "vcruntime140.dll")
            if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                set(OPENSSL_CRYPTO_LIBRARY_FILE "libcrypto-1_1-x64.dll")
                set(OPENSSL_SSL_LIBRARY_FILE "libssl-1_1-x64.dll")
                set(MSVCR_ROOT_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\redist\\x64\\Microsoft.VC140.CRT")
            else()
                set(OPENSSL_CRYPTO_LIBRARY_FILE "libcrypto-1_1.dll")
                set(OPENSSL_SSL_LIBRARY_FILE "libssl-1_1.dll")
                set(MSVCR_ROOT_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\redist\\x86\\Microsoft.VC140.CRT")
            endif()
        endif()
        # Find OpenSSL root directory
        if(NOT OPENSSL_ROOT_DIR)
            list(GET OPENSSL_LIBRARIES -1 OPENSSL_DIRECTORY)
            get_filename_component(OPENSSL_ROOT_DIR ${OPENSSL_DIRECTORY} DIRECTORY)
            string(REGEX REPLACE "(.:[\\/]+[^\\/]+).*" "\\1" OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR})
        endif()
        # Shining Light Productions OpenSSL requires the VC++ runtime
        find_file(MSVCR_LIB_BIN
                NAMES
                ${MSVCR_LIBRARY_FILE}
                HINTS
                ${OPENSSL_ROOT_DIR}
                ${MSVCR_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(NOT MSVCR_LIB_BIN)
            message(FATAL_ERROR "OpenSSL dependency ${MSVCR_LIBRARY_FILE} was NOT found")
        else()
            configure_file(${MSVCR_LIB_BIN} ${CMAKE_BINARY_DIR}/src/bin/${MSVCR_LIBRARY_FILE} COPYONLY)
            message(STATUS "   Copied ${MSVCR_LIB_BIN} to src/bin/")
        endif()
        find_file(CRYPTO_LIB_BIN
                NAMES
                ${OPENSSL_CRYPTO_LIBRARY_FILE}
                HINTS
                ${OPENSSL_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(NOT CRYPTO_LIB_BIN)
            message(FATAL_ERROR "OpenSSL ${OPENSSL_CRYPTO_LIBRARY_FILE} was NOT found")
        else()
            configure_file(${CRYPTO_LIB_BIN} ${CMAKE_BINARY_DIR}/src/bin/${OPENSSL_CRYPTO_LIBRARY_FILE} COPYONLY)
            message(STATUS "   Copied ${CRYPTO_LIB_BIN} to src/bin/")
        endif()
        find_file(SSL_LIB_BIN
                NAMES
                ${OPENSSL_SSL_LIBRARY_FILE}
                HINTS
                ${OPENSSL_ROOT_DIR}
                PATH_SUFFIXES
                bin)
        if(NOT SSL_LIB_BIN)
            message(FATAL_ERROR "OpenSSL ${OPENSSL_SSL_LIBRARY_FILE} was NOT found")
        else()
            configure_file(${SSL_LIB_BIN} ${CMAKE_BINARY_DIR}/src/bin/${OPENSSL_SSL_LIBRARY_FILE} COPYONLY)
            message(STATUS "   Copied ${SSL_LIB_BIN} to src/bin/")
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
