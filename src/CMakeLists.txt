set(mmapper_SRCS
    main.cpp
    client/ClientTelnet.cpp
    client/ClientTelnet.h
    client/clientwidget.cpp
    client/clientwidget.h
    client/displaywidget.cpp
    client/displaywidget.h
    client/inputwidget.cpp
    client/inputwidget.h
    client/stackedinputwidget.cpp
    client/stackedinputwidget.h
    clock/mumeclock.cpp
    clock/mumeclock.h
    clock/mumeclockwidget.cpp
    clock/mumeclockwidget.h
    clock/mumemoment.cpp
    clock/mumemoment.h
    configuration/configuration.cpp
    configuration/configuration.h
    display/ConnectionLineBuilder.cpp
    display/ConnectionLineBuilder.h
    display/connectionselection.cpp
    display/connectionselection.h
    display/FontFormatFlags.h
    display/mapcanvas.cpp
    display/MapCanvasData.cpp
    display/MapCanvasData.h
    display/MapCanvasRoomDrawer.cpp
    display/MapCanvasRoomDrawer.h
    display/mapcanvas.h
    display/mapwindow.cpp
    display/mapwindow.h
    display/OpenGL.cpp
    display/OpenGL.h
    display/prespammedpath.cpp
    display/prespammedpath.h
    display/RoadIndex.cpp
    display/RoadIndex.h
    expandoracommon/abstractroomfactory.h
    expandoracommon/coordinate.cpp
    expandoracommon/coordinate.h
    expandoracommon/exit.cpp
    expandoracommon/exit.h
    expandoracommon/frustum.cpp
    expandoracommon/frustum.h
    expandoracommon/listcycler.h
    expandoracommon/parseevent.cpp
    expandoracommon/parseevent.h
    expandoracommon/property.cpp
    expandoracommon/property.h
    expandoracommon/roomadmin.h
    expandoracommon/room.h
    expandoracommon/roomrecipient.h
    global/bits.h
    global/CharBuffer.h
    global/Color.h
    global/DirectionType.cpp
    global/DirectionType.h
    global/EnumIndexedArray.h
    global/enums.h
    global/Flags.h
    global/io.cpp
    global/io.h
    global/RAII.cpp
    global/RAII.h
    global/range.h
    global/roomid.h
    global/StringView.cpp
    global/StringView.h
    global/utils.h
    mainwindow/aboutdialog.cpp
    mainwindow/aboutdialog.h
    mainwindow/findroomsdlg.cpp
    mainwindow/findroomsdlg.h
    mainwindow/infomarkseditdlg.cpp
    mainwindow/infomarkseditdlg.h
    mainwindow/mainwindow.cpp
    mainwindow/mainwindow.h
    mainwindow/roomeditattrdlg.cpp
    mainwindow/roomeditattrdlg.h
    mainwindow/welcomewidget.cpp
    mainwindow/welcomewidget.h
    mapdata/customaction.cpp
    mapdata/customaction.h
    mapdata/DoorFlags.h
    mapdata/drawstream.h
    mapdata/enums.cpp
    mapdata/enums.h
    mapdata/ExitDirection.cpp
    mapdata/ExitDirection.h
    mapdata/ExitFieldVariant.cpp
    mapdata/ExitFieldVariant.h
    mapdata/ExitFlags.h
    mapdata/infomark.h
    mapdata/mapdata.cpp
    mapdata/mapdata.h
    mapdata/mmapper2exit.h
    mapdata/mmapper2room.cpp
    mapdata/mmapper2room.h
    mapdata/roomfactory.cpp
    mapdata/roomfactory.h
    mapdata/roomfilter.cpp
    mapdata/roomfilter.h
    mapdata/roomselection.cpp
    mapdata/roomselection.h
    mapdata/shortestpath.cpp
    mapdata/shortestpath.h
    mapfrontend/AbstractRoomVisitor.h
    mapfrontend/mapaction.cpp
    mapfrontend/mapaction.h
    mapfrontend/map.cpp
    mapfrontend/mapfrontend.cpp
    mapfrontend/mapfrontend.h
    mapfrontend/map.h
    mapfrontend/ParseTree.cpp
    mapfrontend/ParseTree.h
    mapfrontend/roomcollection.cpp
    mapfrontend/roomcollection.h
    mapfrontend/roomlocker.cpp
    mapfrontend/roomlocker.h
    mapstorage/abstractmapstorage.cpp
    mapstorage/abstractmapstorage.h
    mapstorage/basemapsavefilter.cpp
    mapstorage/basemapsavefilter.h
    mapstorage/filesaver.cpp
    mapstorage/filesaver.h
    mapstorage/jsonmapstorage.cpp
    mapstorage/jsonmapstorage.h
    mapstorage/mapstorage.cpp
    mapstorage/mapstorage.h
    mapstorage/progresscounter.cpp
    mapstorage/progresscounter.h
    mapstorage/roomsaver.cpp
    mapstorage/roomsaver.h
    mapstorage/StorageUtils.cpp
    mapstorage/StorageUtils.h
    mpi/mpifilter.cpp
    mpi/mpifilter.h
    mpi/remoteedit.cpp
    mpi/remoteedit.h
    mpi/remoteeditprocess.cpp
    mpi/remoteeditprocess.h
    mpi/remoteeditsession.cpp
    mpi/remoteeditsession.h
    mpi/remoteeditwidget.cpp
    mpi/remoteeditwidget.h
    pandoragroup/CGroupChar.cpp
    pandoragroup/CGroupChar.h
    pandoragroup/CGroupClient.cpp
    pandoragroup/CGroupClient.h
    pandoragroup/CGroupCommunicator.cpp
    pandoragroup/CGroupCommunicator.h
    pandoragroup/CGroup.cpp
    pandoragroup/CGroup.h
    pandoragroup/CGroupServer.cpp
    pandoragroup/CGroupServer.h
    pandoragroup/groupaction.cpp
    pandoragroup/groupaction.h
    pandoragroup/groupselection.cpp
    pandoragroup/groupselection.h
    pandoragroup/groupwidget.cpp
    pandoragroup/groupwidget.h
    pandoragroup/mmapper2group.cpp
    pandoragroup/mmapper2group.h
    parser/Abbrev.cpp
    parser/Abbrev.h
    parser/AbstractParser-Commands.cpp
    parser/AbstractParser-Commands.h
    parser/abstractparser.cpp
    parser/abstractparser.h
    parser/AbstractParser-Utils.cpp
    parser/AbstractParser-Utils.h
    parser/CommandId.cpp
    parser/CommandId.h
    parser/ConnectedRoomFlags.h
    parser/DoorAction.cpp
    parser/DoorAction.h
    parser/ExitsFlags.h
    parser/mumexmlparser.cpp
    parser/mumexmlparser.h
    parser/parserutils.cpp
    parser/parserutils.h
    parser/patterns.cpp
    parser/patterns.h
    parser/PromptFlags.h
    pathmachine/approved.cpp
    pathmachine/approved.h
    pathmachine/crossover.cpp
    pathmachine/crossover.h
    pathmachine/experimenting.cpp
    pathmachine/experimenting.h
    pathmachine/forced.cpp
    pathmachine/forced.h
    pathmachine/mmapper2pathmachine.cpp
    pathmachine/mmapper2pathmachine.h
    pathmachine/onebyone.cpp
    pathmachine/onebyone.h
    pathmachine/path.cpp
    pathmachine/path.h
    pathmachine/pathmachine.cpp
    pathmachine/pathmachine.h
    pathmachine/pathparameters.cpp
    pathmachine/pathparameters.h
    pathmachine/roomsignalhandler.cpp
    pathmachine/roomsignalhandler.h
    pathmachine/syncing.cpp
    pathmachine/syncing.h
    preferences/ansicombo.cpp
    preferences/ansicombo.h
    preferences/clientpage.cpp
    preferences/clientpage.h
    preferences/configdialog.cpp
    preferences/configdialog.h
    preferences/generalpage.cpp
    preferences/generalpage.h
    preferences/graphicspage.cpp
    preferences/graphicspage.h
    preferences/groupmanagerpage.cpp
    preferences/groupmanagerpage.h
    preferences/mumeprotocolpage.cpp
    preferences/mumeprotocolpage.h
    preferences/parserpage.cpp
    preferences/parserpage.h
    preferences/pathmachinepage.cpp
    preferences/pathmachinepage.h
    proxy/AbstractTelnet.cpp
    proxy/AbstractTelnet.h
    proxy/connectionlistener.cpp
    proxy/connectionlistener.h
    proxy/MudTelnet.cpp
    proxy/MudTelnet.h
    proxy/mumesocket.cpp
    proxy/mumesocket.h
    proxy/proxy.cpp
    proxy/proxy.h
    proxy/telnetfilter.cpp
    proxy/telnetfilter.h
    proxy/TextCodec.cpp
    proxy/TextCodec.h
    proxy/UserTelnet.cpp
    proxy/UserTelnet.h
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

if(WITH_ZLIB)
    include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})
endif()

set(mmapper_DATA
    mmapper.desktop
    MacOSXBundleInfo.plist.in
        display/OpenGL.cpp)

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
    Qt5::Network
    Qt5::OpenGL
)

set_target_properties(
  mmapper PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)

if(WITH_ZLIB)
    target_link_libraries(mmapper ${ZLIB_LIBRARIES})
    if(NOT ZLIB_FOUND)
        add_dependencies(mmapper zlib)
    endif()
endif()

if(USE_TIDY)
    find_program(
        CLANG_TIDY_EXE
        NAMES clang-tidy clang-tidy-6.0
        DOC "Path to clang-tidy executable"
        PATHS /usr/local/opt/llvm/bin
        )
    if(NOT CLANG_TIDY_EXE)
        message(FATAL_ERROR "clang-tidy not found: use `-DUSE_TIDY=OFF` to disable")
    else()
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
        set_target_properties(
            mmapper PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
        )
    endif()
endif()

if(USE_IWYU)
    find_program(IWYU_EXE
        NAMES include-what-you-use iwyu
        DOC "Path to iwyu executable"
        )
    if(NOT IWYU_EXE)
        message(FATAL_ERROR "Could not find IWYU: use `-DUSE_IWYU=OFF` to disable")
    else()
        message(STATUS "Found IWYU: ${IWYU_EXE}")
        set(IWYU_MAP_FILE "${PROJECT_SOURCE_DIR}/iwyu-mapping.imp")
        set(IWYU_PATH_AND_OPTIONS
                ${IWYU_EXE}
                        -Xiwyu "--mapping_file=${IWYU_MAP_FILE}"
        )
        set_target_properties(
                mmapper PROPERTIES
                CXX_INCLUDE_WHAT_YOU_USE "${IWYU_PATH_AND_OPTIONS}"
        )
    endif()
endif()

if(USE_DISTCC)
    find_program(
        DISTCC_EXE
        NAMES distcc
        DOC "Path to distcc executable"
        )
    if(NOT DISTCC_EXE)
        message(FATAL_ERROR "distcc not found: use `-DUSE_DISTCC=OFF` to disable")
    else()
        message(STATUS "distcc found: ${DISTCC_EXE}")
        set(CMAKE_C_COMPILER_LAUNCHER "${DISTCC_EXE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${DISTCC_EXE}")
    endif()
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
    set(SHARE_INSTALL_PREFIX
        "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Base directory for files which go to share/"
    )

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
        DESTINATION share/icons/hicolor/256x256/apps/
        COMPONENT desktopintegration
        RENAME mmapper.png
    )

    set(CPACK_GENERATOR "DEB;TGZ") # Others: RPM, STGZ
    set(CPACK_SOURCE_GENERATOR "TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nschimme@gmail.com")
    set(CPACK_DEBIAN_PACKAGE_NAME "mmapper")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.4.0), libqt5gui5 (>= 5.4.0), libqt5opengl5 (>= 5.4.0), libqt5network5 (>= 5.4.0)")
    if(WITH_ZLIB)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g")
    endif()
    if(WITH_OPENSSL)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libssl1.0.0")
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
