string(TOUPPER "${WITH_MAP}" WITH_MAP_UPPER)
if(WITH_MAP_UPPER STREQUAL "OFF" OR WITH_MAP_UPPER STREQUAL "FALSE")
    message(STATUS "Map is disabled")
else()
    set(MAP_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/map")
    file(MAKE_DIRECTORY "${MAP_DOWNLOAD_DIR}")
    if(IS_ABSOLUTE "${WITH_MAP}" OR EXISTS "${WITH_MAP}")
        get_filename_component(PROVIDED_MAP_ABS "${WITH_MAP}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        get_filename_component(MAP_FILENAME "${WITH_MAP}" NAME)
        set(MMAPPER_MAP_FILE "${MAP_DOWNLOAD_DIR}/${MAP_FILENAME}")

        if(NOT EXISTS "${PROVIDED_MAP_ABS}")
            message(FATAL_ERROR "Provided map file does not exist: ${PROVIDED_MAP_ABS}")
        endif()

        add_custom_command(
            OUTPUT "${MMAPPER_MAP_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROVIDED_MAP_ABS}" "${MMAPPER_MAP_FILE}"
            DEPENDS "${PROVIDED_MAP_ABS}"
            COMMENT "Copying provided map to build directory"
            VERBATIM
        )
        add_custom_target(map ALL DEPENDS "${MMAPPER_MAP_FILE}")
    else()
        set(MMAPPER_MAP_FILENAME "arda.xml")
        set(MMAPPER_MAP_SCHEMA "41")
        set(MMAPPER_MAP_FILE "${MAP_DOWNLOAD_DIR}/${MMAPPER_MAP_FILENAME}")
        include(ExternalProject)
        ExternalProject_Add(map
            GIT_REPOSITORY "https://github.com/MUME/arda"
            GIT_TAG "${MMAPPER_MAP_SCHEMA}"

            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/${MMAPPER_MAP_FILENAME} ${MMAPPER_MAP_FILE}

            UPDATE_COMMAND ""
            BUILD_BYPRODUCTS "${MMAPPER_MAP_FILE}"
        )
    endif()
endif()

message(STATUS "Map will be included: ${MMAPPER_MAP_FILE}")
set(MMAPPER_MAP_FILE "${MMAPPER_MAP_FILE}" PARENT_SCOPE)
