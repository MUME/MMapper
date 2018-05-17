set(mmapper_SRCS
    main.cpp
    clock/mumemoment.cpp
    clock/mumeclock.cpp
    clock/mumeclockwidget.cpp
    client/clientwidget.cpp
    client/stackedinputwidget.cpp
    client/inputwidget.cpp
    client/displaywidget.cpp
    client/ctelnet.cpp
    configuration/configuration.cpp
    display/connectionselection.cpp
    display/mapcanvas.cpp
    display/mapwindow.cpp
    display/prespammedpath.cpp
    expandoracommon/component.cpp
    expandoracommon/coordinate.cpp
    expandoracommon/frustum.cpp
    expandoracommon/parseevent.cpp
    expandoracommon/property.cpp
    mainwindow/aboutdialog.cpp
    mainwindow/findroomsdlg.cpp
    mainwindow/infomarkseditdlg.cpp
    mainwindow/mainwindow.cpp
    mainwindow/roomeditattrdlg.cpp
    mainwindow/welcomewidget.cpp
    mapdata/customaction.cpp
    mapdata/roomfilter.cpp
    mapdata/shortestpath.cpp
    mapdata/mapdata.cpp
    mapdata/mmapper2exit.cpp
    mapdata/mmapper2room.cpp
    mapdata/roomfactory.cpp
    mapdata/roomselection.cpp
    mapfrontend/intermediatenode.cpp
    mapfrontend/mapaction.cpp
    mapfrontend/map.cpp
    mapfrontend/mapfrontend.cpp
    mapfrontend/roomcollection.cpp
    mapfrontend/roomlocker.cpp
    mapfrontend/searchtreenode.cpp
    mapstorage/abstractmapstorage.cpp
    mapstorage/basemapsavefilter.cpp
    mapstorage/filesaver.cpp
    mapstorage/jsonmapstorage.cpp
    mapstorage/mapstorage.cpp
    mapstorage/oldconnection.cpp
    mapstorage/progresscounter.cpp
    mapstorage/roomsaver.cpp
    pandoragroup/CGroupChar.cpp
    pandoragroup/CGroupClient.cpp
    pandoragroup/CGroupCommunicator.cpp
    pandoragroup/CGroup.cpp
    pandoragroup/CGroupServer.cpp
    pandoragroup/mmapper2group.cpp
    pandoragroup/groupaction.cpp
    pandoragroup/groupselection.cpp
    pandoragroup/groupwidget.cpp
    parser/abstractparser.cpp
    parser/mumexmlparser.cpp
    parser/parserutils.cpp
    parser/patterns.cpp
    parser/roompropertysetter.cpp
    parser/mmapper2event.cpp
    mpi/remoteedit.cpp
    mpi/mpifilter.cpp
    mpi/viewsessionprocess.cpp
    mpi/editsessionprocess.cpp
    mpi/remoteeditwidget.cpp
    pathmachine/approved.cpp
    pathmachine/crossover.cpp
    pathmachine/experimenting.cpp
    pathmachine/mmapper2pathmachine.cpp
    pathmachine/onebyone.cpp
    pathmachine/path.cpp
    pathmachine/pathmachine.cpp
    pathmachine/pathparameters.cpp
    pathmachine/roomsignalhandler.cpp
    pathmachine/syncing.cpp
    preferences/ansicombo.cpp
    preferences/configdialog.cpp
    preferences/generalpage.cpp
    preferences/groupmanagerpage.cpp
    preferences/graphicspage.cpp
    preferences/parserpage.cpp
    preferences/pathmachinepage.cpp
    preferences/clientpage.cpp
    preferences/mumeprotocolpage.cpp
    proxy/connectionlistener.cpp
    proxy/proxy.cpp
    proxy/telnetfilter.cpp
    proxy/mumesocket.cpp
)

set(mmapper_RCS
    resources/mmapper2.qrc
    resources/mmapper2.rc
    resources/macosx/m.icns
    resources/win32/m.ico
    resources/icons/hi16-app-mmapper.png
    resources/icons/hi32-app-mmapper.png
    resources/icons/hi48-app-mmapper.png
)

set(mmapper_UIS
    mainwindow/aboutdialog.ui
    mainwindow/findroomsdlg.ui
    mainwindow/infomarkseditdlg.ui
    mainwindow/roomeditattrdlg.ui
    mainwindow/welcomewidget.ui
    clock/mumeclockwidget.ui
    preferences/configdialog.ui
    preferences/generalpage.ui
    preferences/groupmanagerpage.ui
    preferences/graphicspage.ui
    preferences/parserpage.ui
    preferences/pathmachinepage.ui
    preferences/clientpage.ui
    preferences/mumeprotocolpage.ui
)

if(WITH_QTIOCOMPRESSOR)
    if(NOT QTIOCOMPRESSOR_FOUND)
        # Build QtIOCompressor as an object library
        set(mmapper_SRCS ${mmapper_SRCS} ${QTIOCOMPRESSOR_OBJECTS})
    endif()
    include_directories(${QTIOCOMPRESSOR_INCLUDE_DIRS})
endif()

set(mmapper_DATA
    mmapper.desktop
    mmapper.astylerc
    MacOSXBundleInfo.plist.in
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/clock
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration
    ${CMAKE_CURRENT_SOURCE_DIR}/display
    ${CMAKE_CURRENT_SOURCE_DIR}/expandoracommon
    ${CMAKE_CURRENT_SOURCE_DIR}/global
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow
    ${CMAKE_CURRENT_SOURCE_DIR}/mapdata
    ${CMAKE_CURRENT_SOURCE_DIR}/mapfrontend
    ${CMAKE_CURRENT_SOURCE_DIR}/mapstorage
    ${CMAKE_CURRENT_SOURCE_DIR}/pandoragroup
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/pathmachine
    ${CMAKE_CURRENT_SOURCE_DIR}/preferences
    ${CMAKE_CURRENT_SOURCE_DIR}/proxy
    ${ZLIB_INCLUDE_DIRS}
)

# Build the executable
add_executable(mmapper WIN32 MACOSX_BUNDLE
    ${mmapper_SRCS}
    ${mmapper_UIS}
    ${mmapper_RCS}
    ${mmapper_DATA}
)

target_link_libraries(mmapper
    Qt5::Core
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    Qt5::OpenGL
)

set_target_properties(
  mmapper PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  COMPILE_FLAGS "${WARNING_FLAGS}"
)

if(WITH_QTIOCOMPRESSOR)
    if(QTIOCOMPRESSOR_FOUND)
        target_link_libraries(mmapper ${QTIOCOMPRESSOR_LIBRARIES})
    else()
        # Build QtIOCompressor as an object library
        IF (NOT ZLIB_FOUND)
            # Assume we are building a static zlib
            add_dependencies(mmapper zlib)
        endif()
        target_link_libraries(mmapper ${ZLIB_LIBRARIES})
    endif()
endif()

# clang-tidy support
find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy-6.0"
    DOC "Path to clang-tidy executable"
    )
if(NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}") # "-checks=*,-clang-analyzer-alpha.*")
endif()
if(CLANG_TIDY_EXE)
    set_target_properties(
        mmapper PROPERTIES
        CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif()

# Begin CPack Settings
set(CPACK_PACKAGE_VERSION ${MMAPPER_VERSION})
string(REGEX REPLACE "[.-]" " " VERSION_NUMBERS ${MMAPPER_VERSION})
separate_arguments(VERSION_NUMBERS)
list(GET VERSION_NUMBERS 0 CPACK_PACKAGE_VERSION_MAJOR)
list(REMOVE_AT VERSION_NUMBERS 0)
list(GET VERSION_NUMBERS 0 CPACK_PACKAGE_VERSION_MINOR)
list(REMOVE_AT VERSION_NUMBERS 0)
list(GET VERSION_NUMBERS 0 CPACK_PACKAGE_VERSION_PATCH)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-source")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")

# Linux Install Settings
if(UNIX AND NOT APPLE)
    # Install Executables
    install(
        TARGETS mmapper
        RUNTIME DESTINATION bin
        COMPONENT applications
    )

    # Install Map
    install(
        FILES ${MMAPPER_MAP_FILE}
        DESTINATION share/games/mmapper
        COMPONENT map
    )

    # Linux Desktop Integration
    install(FILES mmapper.desktop
            DESTINATION share/applications
            COMPONENT desktopintegration
    )

    # Install icons
    foreach(RES 16 32 48)
        install(
            FILES resources/icons/hi${RES}-app-mmapper.png
            DESTINATION share/icons/hicolor/${RES}x${RES}/apps/
            COMPONENT desktopintegration
            RENAME mmapper.png
        )
    endforeach(RES)

    install(
        FILES resources/icons/m.png
        DESTINATION share/icons/hicolor/128x128/apps/
        COMPONENT desktopintegration
        RENAME mmapper.png
    )

    set(CPACK_GENERATOR "DEB;TGZ") # Others: RPM, STGZ
    set(CPACK_SOURCE_GENERATOR "TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nschimme@gmail.com")
    set(CPACK_DEBIAN_PACKAGE_NAME "mmapper")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.4.0), libqt5gui5 (>= 5.4.0), libqt5opengl5 (>= 5.4.0), libqt5network5 (>= 5.4.0), libqt5xml5 (>= 5.4.0)")
    if(WITH_QTIOCOMPRESSOR)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g")
    endif()

    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif(UNIX AND NOT APPLE)

# Windows Install Settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)

    # Install Executables
    install(
        TARGETS mmapper RUNTIME
        DESTINATION bin
        COMPONENT applications
    )

    # Install Map
    install(
        FILES ${MMAPPER_MAP_FILE}
        DESTINATION .
        COMPONENT map
    )

    # Bundle Library Files
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(WINDEPLOYQT_ARGS --debug)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            set(WINDEPLOYQT_ARGS --release-with-debug-info)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            set(WINDEPLOYQT_ARGS --release)
    endif()
    find_program(WINDEPLOYQT_APP windeployqt)
    message(" - windeployqt path: ${WINDEPLOYQT_APP}")
    add_custom_command(
            TARGET mmapper
            POST_BUILD
            COMMAND ${WINDEPLOYQT_APP} ${WINDEPLOYQT_ARGS} ${CMAKE_CURRENT_BINARY_DIR}/mmapper.exe --dir ${CMAKE_CURRENT_BINARY_DIR}/bin --compiler-runtime
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Finding the Qt framework dependencies"
    )
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/bin
        DESTINATION .
        COMPONENT libraries
    )

    # Install Help Documentation
    install(FILES
        "${PROJECT_SOURCE_DIR}/docs/Installation Guide.URL"
        "${PROJECT_SOURCE_DIR}/docs/mapping_advice.txt"
        DESTINATION .
        COMPONENT help
    )

    # ZIP requires 7-zip
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_SOURCE_GENERATOR "ZIP")

    # Windows (NSIS) Settings
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_MENU_LINKS "bin/mmapper.exe;MMapper;Installation Guide.URL;Installation Guide")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "mmapper.exe")
    set(CPACK_NSIS_DISPLAY_NAME "MMapper")
    set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\github.com\\\\mume\\\\mmapper")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m.ico")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons\\\\m.png")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m.ico")
    set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "mmapper.exe")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Caligor soft")

endif(WIN32)

# Apple Install Settings
if(APPLE)
    set(CMAKE_MACOSX_BUNDLE ON)
    set_source_files_properties(resources/macosx/m.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(mmapper PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in )

    # Bundle the libraries with the binary
    find_program(MACDEPLOYQT_APP macdeployqt)
    message(" - macdeployqt path: ${MACDEPLOYQT_APP}")
    add_custom_command(
        TARGET mmapper
        POST_BUILD
        COMMAND ${MACDEPLOYQT_APP} ${CMAKE_CURRENT_BINARY_DIR}/mmapper.app
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Deploying the Qt Framework onto the bundle"
        )

    # Install Executables
    install(
        TARGETS mmapper RUNTIME BUNDLE
        DESTINATION MMapper
        COMPONENT applications
    )

    # Install Map
    install(
        FILES ${MMAPPER_MAP_FILE}
        DESTINATION MMapper
        COMPONENT map
    )

    # Install Help Documentation
    install(FILES
        "${PROJECT_SOURCE_DIR}/docs/Installation Guide.URL"
        "${PROJECT_SOURCE_DIR}/docs/mapping_advice.txt"
        DESTINATION MMapper
        COMPONENT help
    )

    # Users don't understand the term Darwin
    string(REGEX REPLACE "Darwin" "Mac" CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME})

    # Package Settings
    set(CPACK_GENERATOR "DragNDrop;ZIP")
    set(CPACK_SOURCE_GENERATOR "TGZ")

    # Libraries are bundled directly
    set(CPACK_COMPONENT_LIBRARIES_HIDDEN TRUE)

    # Bundle Properties
    set(MACOSX_BUNDLE_BUNDLE_NAME MMapper)
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${MMAPPER_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${MMAPPER_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MMAPPER_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE m.icns)
endif(APPLE)

# More General CPack Settings
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING.txt")
#set(CPACK_RESOURCE_FILE_README  "${PROJECT_SOURCE_DIR}/INSTALL.txt")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Nils Schimmelmann")
set(CPACK_PACKAGE_CONTACT "nschimme@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A mud mapper especially written for the mud MUME")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MMapper")

# Components:
set(CPACK_STRIP_FILES TRUE)
# Applications Component
set(CPACK_COMPONENTS_ALL applications map libraries help)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "MMapper")
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "A mud mapper especially written for the mud MUME")
set(CPACK_COMPONENT_APPLICATIONS_REQUIRED TRUE)

# Map Component
set(CPACK_COMPONENT_MAP_DISPLAY_NAME "Included Map")
set(CPACK_COMPONENT_MAP_DESCRIPTION "A base map of Arda")

# Libraries Component
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Qt/MingW libraries for running MMapper")

# Help Component
set(CPACK_COMPONENT_HELP_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_HELP_DESCRIPTION "Guide on how to set up your client")
set(CPACK_COMPONENT_HELP_HIDDEN TRUE)

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};applications;ALL;/")

# Source Package
set(CPACK_SOURCE_IGNORE_FILES
    "~$"
    "/\\\\.git/"
    "/build/"
    "/winbuild/"
)
include(CPack)
