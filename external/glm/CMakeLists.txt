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
        URL "https://github.com/g-truc/glm/releases/download/0.9.9.7/glm-0.9.9.7.zip"
        URL_HASH SHA1=CCA9124F1E06FAB44B17D0C0BE651977E508F3AC

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
