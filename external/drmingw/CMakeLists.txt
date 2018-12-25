if(MSVC OR MINGW)
    include(ExternalProject)
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        ExternalProject_Add(drmingw
            URL https://github.com/jrfonseca/drmingw/releases/download/0.8.1/drmingw-0.8.1-win64.7z
            URL_HASH SHA1=8be29cc7efc0d611056c3b745e7f8de30d7c8baa

            INSTALL_DIR "${CMAKE_BINARY_DIR}/src/bin"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ${CMAKE_COMMAND} -E echo Deploying Dr. Mingw 64-bit binary and dependencies
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/dbghelp.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/exchndl.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/mgwhelp.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/symsrv.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/symsrv.yes ${CMAKE_BINARY_DIR}/src/bin/

            UPDATE_COMMAND ""
            ALWAYS 0
        )
    else()
        ExternalProject_Add(drmingw
            URL https://github.com/jrfonseca/drmingw/releases/download/0.8.1/drmingw-0.8.1-win32.7z
            URL_HASH SHA1=5eefeefc392cae36afb891ca81e3756aa0d79644

            INSTALL_DIR "${CMAKE_BINARY_DIR}/src/bin"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ${CMAKE_COMMAND} -E echo Deploying Dr. Mingw 32-bit binary and dependencies
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/dbghelp.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/exchndl.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/mgwhelp.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/symsrv.dll ${CMAKE_BINARY_DIR}/src/bin/
                    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/bin/symsrv.yes ${CMAKE_BINARY_DIR}/src/bin/

            UPDATE_COMMAND ""
            ALWAYS 0
        )
    endif()
endif()
