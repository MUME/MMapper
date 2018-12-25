set(OPENSSL_VERSION 1.1.1a)
if(MINGW)
    include(ExternalProject)
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        ExternalProject_Add(openssl
            URL https://bintray.com/vszakats/generic/download_file?file_path=openssl-${OPENSSL_VERSION}-win64-mingw.zip
            URL_HASH SHA1=dcb33213d739d01ffa892ee39516add37f995129

            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-src"
            INSTALL_DIR "${CMAKE_BINARY_DIR}/src/bin"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ${CMAKE_COMMAND} -E echo Deploying OpenSSL 64-bit binary and dependencies
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libcrypto-1_1.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libssl-1_1.dll ${CMAKE_BINARY_DIR}/src/bin/

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
    if(MSVC)
        message(STATUS "OpenSSL NOT Found: download Win32 OpenSSL v${OPENSSL_VERSION}: https://slproweb.com/products/Win32OpenSSL.html")
        message(STATUS "     or")
    endif()
    message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_OPENSSL=OFF` to build without TLS encryption")
endif()
