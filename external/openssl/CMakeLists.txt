if(WIN32 AND NOT OPENSSL_FOUND)
    # Perl is required if we want to try building from the local source tree
    find_package(Perl)
endif()
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL ${OPENSSL_VERSION} found: ${OPENSSL_LIBRARIES}")
    if(WIN32)
        # Verify we have the correct OpenSSL version
        if (OPENSSL_VERSION VERSION_LESS 1.1.1)
            message(FATAL_ERROR "Qt versions after 5.12.3 require OpenSSL version 1.1.1 or higher")
        endif()
        # Ensure that we package OpenSSL DLLs with MMapper for Windows
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/bin)
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
        # Find OpenSSL root directory
        if(NOT OPENSSL_ROOT_DIR)
            list(GET OPENSSL_LIBRARIES -1 OPENSSL_DIRECTORY)
            get_filename_component(OPENSSL_ROOT_DIR ${OPENSSL_DIRECTORY} DIRECTORY)
            string(REGEX REPLACE "(.:[\\/]+[^\\/]+).*" "\\1" OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR})
        endif()
        if(${OPENSSL_ROOT_DIR} MATCHES ".*[Mm][Ss][Yy][Ss]64.*")
            message(STATUS "   Detected MSYS2 MinGW compiled OpenSSL library")
        else()
            message(STATUS "   Assuming Shining Light Productions MSVC compiled OpenSSL library")
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
elseif(WIN32 AND PERL_FOUND AND Qt5Core_VERSION VERSION_GREATER 5.12.3)
    message(STATUS "Could not find a local OpenSSL; building from local source tree")
    include(ExternalProject)
    set(OPENSSL_VERSION 1.1.1s)
    set(OPENSSL_URL_HASH d316e1523a609bbfc4ddd3abfa9861db99f17044)
    # Build the shared library
    set(OPENSSL_CONFIGURATION no-tests no-unit-test shared)
    # Improve security by disabling weak ciphers
    set(OPENSSL_CONFIGURATION no-weak-ssl-ciphers no-idea no-mdc2 no-rc5 ${OPENSSL_CONFIGURATION})
    if(MINGW)
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            # Additional options taken from: https://github.com/curl/curl-for-win/blob/master/openssl.sh
            set(OPENSSL_CONFIGURATION ${OPENSSL_CONFIGURATION} no-asm enable-ec_nistp_64_gcc_128 mingw64)
        else()
            set(OPENSSL_CONFIGURATION ${OPENSSL_CONFIGURATION} mingw)
        endif()
        ExternalProject_Add(openssl
            URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
            URL_HASH SHA1=${OPENSSL_URL_HASH}

            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-src"
            INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-bin"
            CONFIGURE_COMMAND ${PERL_EXECUTABLE} Configure --prefix=<INSTALL_DIR> --openssldir=<INSTALL_DIR> ${OPENSSL_CONFIGURATION}

            BUILD_COMMAND make depends
            COMMAND make
            INSTALL_COMMAND make install_sw

            UPDATE_COMMAND ""
            BUILD_IN_SOURCE 1
            ALWAYS 0

            BUILD_BYPRODUCTS "<INSTALL_DIR>/lib/libcrypto.dll.a"
            BUILD_BYPRODUCTS "<INSTALL_DIR>/lib/libssl.dll.a"
        )
        ExternalProject_Get_Property(openssl INSTALL_DIR)
        set(OPENSSL_LIBRARIES
            ${INSTALL_DIR}/lib/libcrypto.dll.a
            ${INSTALL_DIR}/lib/libssl.dll.a
            PARENT_SCOPE)
    elseif(MSVC)
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OPENSSL_CONFIGURATION ${OPENSSL_CONFIGURATION} VC-WIN64A)
        else()
            set(OPENSSL_CONFIGURATION ${OPENSSL_CONFIGURATION} VC-WIN32)
        endif()
        ExternalProject_Add(openssl
            URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
            URL_HASH SHA1=${OPENSSL_URL_HASH}

            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-src"
            INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/openssl-bin"
            CONFIGURE_COMMAND ${PERL_EXECUTABLE} Configure --prefix=<INSTALL_DIR> --openssldir=<INSTALL_DIR> ${OPENSSL_CONFIGURATION}

            BUILD_COMMAND nmake
            INSTALL_COMMAND nmake install_sw

            UPDATE_COMMAND ""
            BUILD_IN_SOURCE 1
            ALWAYS 0

            BUILD_BYPRODUCTS "<INSTALL_DIR>/lib/libcrypto.lib"
            BUILD_BYPRODUCTS "<INSTALL_DIR>/lib/libssl.lib"
        )
        ExternalProject_Get_Property(openssl INSTALL_DIR)
        set(OPENSSL_LIBRARIES
            ${INSTALL_DIR}/lib/libcrypto.lib
            ${INSTALL_DIR}/lib/libssl.lib
            PARENT_SCOPE)
    endif()
    set(OPENSSL_INCLUDE_DIR "${INSTALL_DIR}/include" PARENT_SCOPE)
    set(OPENSSL_VERSION ${OPENSSL_VERSION} PARENT_SCOPE)
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(OPENSSL_CRYPTO_LIBRARY_FILE "libcrypto-1_1-x64.dll")
        set(OPENSSL_SSL_LIBRARY_FILE "libssl-1_1-x64.dll")
    else()
        set(OPENSSL_CRYPTO_LIBRARY_FILE "libcrypto-1_1.dll")
        set(OPENSSL_SSL_LIBRARY_FILE "libssl-1_1.dll")
    endif()
    add_custom_command(
        TARGET openssl
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo Deploying OpenSSL binary
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${INSTALL_DIR}/bin/${OPENSSL_CRYPTO_LIBRARY_FILE} ${CMAKE_BINARY_DIR}/src/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${INSTALL_DIR}/bin/${OPENSSL_SSL_LIBRARY_FILE} ${CMAKE_BINARY_DIR}/src/bin/
     )
else()
    if(WIN32)
        message(STATUS "OpenSSL NOT Found: download OpenSSL v1.1.1: https://slproweb.com/products/Win32OpenSSL.html")
        message(STATUS "     or")
    endif()
    message(FATAL_ERROR "OpenSSL NOT found: use `-DWITH_OPENSSL=OFF` to build without TLS encryption")
endif()
