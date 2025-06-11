# Handle map integration: either use a provided file or download from Git.
string(TOUPPER "${WITH_MAP}" WITH_MAP_UPPER)
if(WITH_MAP_UPPER STREQUAL "OFF" OR WITH_MAP_UPPER STREQUAL "FALSE")
    message(STATUS "Map integration disabled.")
else()
    set(MAP_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/map")
    set(MAP_QRC_FILE "${MAP_DOWNLOAD_DIR}/map.qrc")
    set(MAP_FILE "${MAP_DOWNLOAD_DIR}/arda")
    set(MAP_TARGET_DEPENDENCY "")

    file(MAKE_DIRECTORY "${MAP_DOWNLOAD_DIR}")

    # User provides an existing map file
    if(IS_ABSOLUTE "${WITH_MAP}" OR EXISTS "${WITH_MAP}")
        get_filename_component(PROVIDED_MAP_ABS "${WITH_MAP}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

        if(NOT EXISTS "${PROVIDED_MAP_ABS}")
            message(FATAL_ERROR "Provided map file or directory does not exist: ${PROVIDED_MAP_ABS}")
        endif()

        add_custom_command(
            OUTPUT "${MAP_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROVIDED_MAP_ABS}" "${MAP_FILE}"
            COMMENT "Copying provided map: ${PROVIDED_MAP_ABS}"
            VERBATIM
        )
        add_custom_target(map_copy ALL DEPENDS "${MAP_FILE}")
        set(MAP_TARGET_DEPENDENCY map_copy)

    # Download map from Git repository
    else()
        set(MAP_SCHEMA "42") # Git tag for the map schema version.

        include(ExternalProject)
        ExternalProject_Add(map_repo
            GIT_REPOSITORY "https://github.com/MUME/arda"
            GIT_TAG "${MAP_SCHEMA}"

            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            UPDATE_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/arda.xml "${MAP_FILE}"

            BUILD_BYPRODUCTS "${MAP_FILE}"
        )
        set(MAP_TARGET_DEPENDENCY map_repo)
    endif()

    # Generate QRC
    add_custom_command(
        OUTPUT "${MAP_QRC_FILE}"
        COMMAND ${CMAKE_COMMAND} -DINPUT_DIR=${MAP_DOWNLOAD_DIR} -DOUTPUT_FILE=${MAP_QRC_FILE} -P "${CMAKE_SOURCE_DIR}/cmake/GenerateQRC.cmake"
        DEPENDS "${MAP_FILE}"
        COMMENT "Generating map.qrc"
        WORKING_DIRECTORY "${MAP_DOWNLOAD_DIR}"
        VERBATIM
    )
    add_custom_target(map_qrc ALL DEPENDS "${MAP_QRC_FILE}")
    add_dependencies(map_qrc ${MAP_TARGET_DEPENDENCY})
    message(STATUS "Map QRC file will be included: ${MAP_QRC_FILE}")
endif()
