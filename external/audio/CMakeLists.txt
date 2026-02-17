# Handle audio resource integration: use a provided directory or download from Git.
string(TOUPPER "${WITH_AUDIO}" WITH_AUDIO_UPPER)
if(WITH_AUDIO_UPPER STREQUAL "OFF" OR WITH_AUDIO_UPPER STREQUAL "FALSE")
    message(STATUS "Audio resources disabled.")
else()
    set(AUDIO_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/audio")
    set(MMAPPER_QRC_FILE "${AUDIO_DOWNLOAD_DIR}/audio.qrc")
    set(AUDIO_TARGET_DEPENDENCY "")

    file(MAKE_DIRECTORY "${AUDIO_DOWNLOAD_DIR}")

    # User provides an existing resources directory
    if(IS_ABSOLUTE "${WITH_AUDIO}" OR EXISTS "${WITH_AUDIO}")
        get_filename_component(PROVIDED_AUDIO_AB "${WITH_AUDIO}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

        if(NOT EXISTS "${PROVIDED_AUDIO_AB}")
            message(FATAL_ERROR "Provided resources directory does not exist: ${PROVIDED_AUDIO_AB}")
        endif()

        set(AUDIO_COPY_STAMP "${AUDIO_DOWNLOAD_DIR}/CMakeLists.txt.copy_stamp")
        add_custom_command(
            OUTPUT "${AUDIO_COPY_STAMP}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROVIDED_AUDIO_AB}" "${AUDIO_DOWNLOAD_DIR}"
            COMMAND ${CMAKE_COMMAND} -E touch "${AUDIO_COPY_STAMP}"
            DEPENDS ${PROVIDED_AUDIO_AB}
            COMMENT "Copying provided resources to ${AUDIO_DOWNLOAD_DIR}"
            VERBATIM
        )
        add_custom_target(audio_copy_stamp ALL DEPENDS "${AUDIO_COPY_STAMP}")
        set(AUDIO_TARGET_DEPENDENCY audio_copy_stamp)

    # Download audio from Git repository
    else()
        set(AUDIO_TAG "main") # Git tag for the audio resources.

        include(ExternalProject)
        ExternalProject_Add(audio_repo
            GIT_REPOSITORY "https://github.com/MUME/audio"
            GIT_TAG "${AUDIO_TAG}"
            SOURCE_DIR "${AUDIO_DOWNLOAD_DIR}"

            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            UPDATE_COMMAND ""
        )
        set(AUDIO_TARGET_DEPENDENCY audio_repo)
    endif()

    # Generate QRC from audio
    add_custom_command(
        OUTPUT "${MMAPPER_QRC_FILE}"
        COMMAND ${CMAKE_COMMAND} -DINPUT_DIR=${AUDIO_DOWNLOAD_DIR} -DOUTPUT_FILE=${MMAPPER_QRC_FILE} -P "${CMAKE_SOURCE_DIR}/cmake/GenerateQRC.cmake"
        DEPENDS "${AUDIO_TARGET_DEPENDENCY}"
        COMMENT "Generating audio.qrc"
        WORKING_DIRECTORY "${AUDIO_DOWNLOAD_DIR}"
        VERBATIM
    )
    add_custom_target(audio_qrc ALL DEPENDS "${MMAPPER_QRC_FILE}")
    add_dependencies(audio_qrc ${AUDIO_TARGET_DEPENDENCY})
    message(STATUS "Audio QRC file will be included: ${MMAPPER_QRC_FILE}")
endif()
