# Handle image resource integration: use a provided directory or download from Git.
string(TOUPPER "${WITH_IMAGES}" WITH_IMAGES_UPPER)
if(WITH_IMAGES_UPPER STREQUAL "OFF" OR WITH_IMAGES_UPPER STREQUAL "FALSE")
    message(STATUS "Image resources disabled.")
else()
    set(IMAGES_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/images")
    set(MMAPPER_QRC_FILE "${IMAGES_DOWNLOAD_DIR}/images.qrc")
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
        set(IMAGES_TAG "v25.06.0") # Git tag for the image resources.

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

    # Generate QRC from images
    add_custom_command(
        OUTPUT "${MMAPPER_QRC_FILE}"
        COMMAND ${CMAKE_COMMAND} -DINPUT_DIR=${IMAGES_DOWNLOAD_DIR} -DOUTPUT_FILE=${MMAPPER_QRC_FILE} -P "${CMAKE_SOURCE_DIR}/cmake/GenerateQRC.cmake"
        DEPENDS "${IMAGES_TARGET_DEPENDENCY}"
        COMMENT "Generating images.qrc"
        WORKING_DIRECTORY "${IMAGES_DOWNLOAD_DIR}"
        VERBATIM
    )
    add_custom_target(images_qrc ALL DEPENDS "${MMAPPER_QRC_FILE}")
    add_dependencies(images_qrc ${IMAGES_TARGET_DEPENDENCY})
    message(STATUS "Images QRC file will be included: ${MMAPPER_QRC_FILE}")
endif()
