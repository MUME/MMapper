include(ExternalProject)
ExternalProject_Add(glm
    URL "https://github.com/g-truc/glm/archive/0.9.9-a2.zip"
    URL_HASH SHA1=8220869d0a85cba4f1e4f6da77ce9c4ada18f5bf

    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/glm"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    ALWAYS 0
    )
ExternalProject_Get_Property(glm SOURCE_DIR)
set(GLM_INCLUDE_DIR ${SOURCE_DIR} PARENT_SCOPE)
