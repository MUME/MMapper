if(MINGW)
    include(ExternalProject)
    ExternalProject_Add(drmingw
        URL https://github.com/jrfonseca/drmingw/releases/download/0.9.1/drmingw-0.9.1-win64.7z
        URL_HASH SHA1=cc6076b94684cc635c0e7da6761a4611b3fe43c6

        INSTALL_DIR "${CMAKE_BINARY_DIR}/src/bin"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E echo Deploying Dr. Mingw 64-bit binary and dependencies
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/dbgcore.dll ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/dbghelp.dll ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/exchndl.dll ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/mgwhelp.dll ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/symsrv.dll ${CMAKE_BINARY_DIR}/src/bin/
                COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/symsrv.yes ${CMAKE_BINARY_DIR}/src/bin/

        UPDATE_COMMAND ""
        ALWAYS 0

        BUILD_BYPRODUCTS "<SOURCE_DIR>/bin/exchndl.dll"
    )
    ExternalProject_Get_Property(drmingw SOURCE_DIR)
    set(DRMINGW_INCLUDE_DIR ${SOURCE_DIR} ${SOURCE_DIR}/include PARENT_SCOPE)
    set(DRMINGW_LIBRARY ${SOURCE_DIR}/bin/exchndl.dll PARENT_SCOPE)
endif()
