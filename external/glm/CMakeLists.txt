find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS /app/include /usr/include)

# Check if glm was found and set up include directories
if(GLM_INCLUDE_DIR)
    message(STATUS "Found glm: ${GLM_INCLUDE_DIR}")
    set(GLM_INCLUDE_DIR ${GLM_INCLUDE_DIR} PARENT_SCOPE)
    set(GLM_FOUND true PARENT_SCOPE)
else()
    message(STATUS "Could not find a local glm; downloading to local source tree")
    include(ExternalProject)
    ExternalProject_Add(glm
        URL "https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip"
        URL_HASH SHA1=45408897f419944fb28d8fd835791f237be2ec19

        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/glm"
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        ALWAYS 0
        )
    ExternalProject_Get_Property(glm SOURCE_DIR)
    set(GLM_INCLUDE_DIR ${SOURCE_DIR} PARENT_SCOPE)
endif()
