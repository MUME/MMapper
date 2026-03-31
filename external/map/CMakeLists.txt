# Handle map integration: either use a provided file or download from Git.
string(TOUPPER "${WITH_MAP}" WITH_MAP_UPPER)
if(WITH_MAP_UPPER STREQUAL "OFF" OR WITH_MAP_UPPER STREQUAL "FALSE")
    message(STATUS "Map integration disabled.")
else()
    set(MAP_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/map")
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

    # For all platforms: copy map to the sideloaded assets directory
    set(MAP_ASSETS_DIR "${CMAKE_BINARY_DIR}/${ASSETS_NAME}/map")
    set(MAP_ASSETS_FILE "${MAP_ASSETS_DIR}/arda")
    set(MAP_ASSETS_STAMP "${MAP_DOWNLOAD_DIR}/CMakeLists.txt.assets_stamp")
    file(MAKE_DIRECTORY "${MAP_ASSETS_DIR}")
    add_custom_command(
        OUTPUT "${MAP_ASSETS_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MAP_FILE}" "${MAP_ASSETS_FILE}"
        COMMAND ${CMAKE_COMMAND} -E touch "${MAP_ASSETS_STAMP}"
        DEPENDS "${MAP_FILE}"
        COMMENT "Copying map to ${ASSETS_NAME} directory"
        VERBATIM
    )
    add_custom_target(map_assets ALL DEPENDS "${MAP_ASSETS_STAMP}")
    add_dependencies(map_assets ${MAP_TARGET_DEPENDENCY})
    message(STATUS "Map will be sideloaded: ${MAP_ASSETS_FILE}")
endif()
