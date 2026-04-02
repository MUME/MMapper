# Handle image resource integration: use a provided directory or download from Git.
string(TOUPPER "${WITH_IMAGES}" WITH_IMAGES_UPPER)
if(WITH_IMAGES_UPPER STREQUAL "OFF" OR WITH_IMAGES_UPPER STREQUAL "FALSE")
    message(STATUS "Image resources disabled.")
else()
    set(IMAGES_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/images")
    set(IMAGES_TARGET_DEPENDENCY "")

    file(MAKE_DIRECTORY "${IMAGES_DOWNLOAD_DIR}")

    # User provides an existing resources directory
    if(IS_ABSOLUTE "${WITH_IMAGES}" OR EXISTS "${WITH_IMAGES}")
        get_filename_component(PROVIDED_IMAGES_ABS "${WITH_IMAGES}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

        if(NOT EXISTS "${PROVIDED_IMAGES_ABS}")
            message(FATAL_ERROR "Provided resources directory does not exist: ${PROVIDED_IMAGES_ABS}")
        endif()

        set(IMAGES_COPY_STAMP "${IMAGES_DOWNLOAD_DIR}/CMakeLists.txt.copy_stamp")
        add_custom_command(
            OUTPUT "${IMAGES_COPY_STAMP}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROVIDED_IMAGES_ABS}" "${IMAGES_DOWNLOAD_DIR}"
            COMMAND ${CMAKE_COMMAND} -E touch "${IMAGES_COPY_STAMP}"
            DEPENDS ${PROVIDED_IMAGES_ABS}
            COMMENT "Copying provided resources to ${IMAGES_DOWNLOAD_DIR}"
            VERBATIM
        )
        add_custom_target(images_copy_stamp ALL DEPENDS "${IMAGES_COPY_STAMP}")
        set(IMAGES_TARGET_DEPENDENCY images_copy_stamp)

    # Download images from Git repository
    else()
        set(IMAGES_TAG "v26.04.0") # Git tag for the image resources.

        include(ExternalProject)
        ExternalProject_Add(images_repo
            GIT_REPOSITORY "https://github.com/MUME/images"
            GIT_TAG "${IMAGES_TAG}"
            SOURCE_DIR "${IMAGES_DOWNLOAD_DIR}"

            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            UPDATE_COMMAND ""
        )
        set(IMAGES_TARGET_DEPENDENCY images_repo)
    endif()

    # For all platforms: copy assets to the sideloaded assets directory
    set(IMAGES_ASSETS_STAMP "${IMAGES_DOWNLOAD_DIR}/CMakeLists.txt.assets_stamp")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${ASSETS_NAME}")
    add_custom_command(
        OUTPUT "${IMAGES_ASSETS_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${IMAGES_DOWNLOAD_DIR}" "${CMAKE_BINARY_DIR}/${ASSETS_NAME}"
        COMMAND ${CMAKE_COMMAND} -E touch "${IMAGES_ASSETS_STAMP}"
        DEPENDS "${IMAGES_TARGET_DEPENDENCY}"
        COMMENT "Copying image assets to ${ASSETS_NAME} directory"
        VERBATIM
    )
    add_custom_target(images_assets ALL DEPENDS "${IMAGES_ASSETS_STAMP}")
    add_dependencies(images_assets ${IMAGES_TARGET_DEPENDENCY})
    message(STATUS "Image assets will be sideloaded: ${CMAKE_BINARY_DIR}/${ASSETS_NAME}")
endif()
