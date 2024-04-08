set(mmapper_SRCS
    main.cpp
    adventure/adventuresession.cpp
    adventure/adventuresession.h
    adventure/adventuretracker.cpp
    adventure/adventuretracker.h
    adventure/adventurewidget.cpp
    adventure/adventurewidget.h
    adventure/lineparsers.h
    adventure/lineparsers.cpp
    adventure/xpstatuswidget.cpp
    adventure/xpstatuswidget.h
    client/ClientTelnet.cpp
    client/ClientTelnet.h
    client/ClientWidget.cpp
    client/ClientWidget.h
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
    configuration/NamedConfig.h
    configuration/configuration.cpp
    configuration/configuration.h
    configuration/configobserver.h
    display/CanvasMouseModeEnum.h
    display/Characters.cpp
    display/Characters.h
    display/ConnectionLineBuilder.cpp
    display/ConnectionLineBuilder.h
    display/Connections.cpp
    display/Connections.h
    display/Filenames.cpp
    display/Filenames.h
    display/InfoMarkSelection.cpp
    display/InfoMarkSelection.h
    display/Infomarks.cpp
    display/Infomarks.h
    display/MapCanvasConfig.h
    display/MapCanvasData.cpp
    display/MapCanvasData.h
    display/MapCanvasRoomDrawer.cpp
    display/MapCanvasRoomDrawer.h
    display/RoadIndex.cpp
    display/RoadIndex.h
    display/RoomSelections.cpp
    display/Textures.cpp
    display/Textures.h
    display/connectionselection.cpp
    display/connectionselection.h
    display/mapcanvas.cpp
    display/mapcanvas.h
    display/mapcanvas_gl.cpp
    display/mapwindow.cpp
    display/mapwindow.h
    display/prespammedpath.cpp
    display/prespammedpath.h
    expandoracommon/MmQtHandle.h
    expandoracommon/RoomAdmin.cpp
    expandoracommon/RoomAdmin.h
    expandoracommon/RoomRecipient.cpp
    expandoracommon/RoomRecipient.h
    expandoracommon/coordinate.cpp
    expandoracommon/coordinate.h
    expandoracommon/exit.cpp
    expandoracommon/exit.h
    expandoracommon/parseevent.cpp
    expandoracommon/parseevent.h
    expandoracommon/property.cpp
    expandoracommon/property.h
    expandoracommon/room.cpp
    expandoracommon/room.h
    global/AnsiColor.h
    global/Array.h
    global/ChangeMonitor.h
    global/CharBuffer.h
    global/Charset.cpp
    global/Charset.h
    global/Color.cpp
    global/Color.h
    global/Debug.h
    global/EnumIndexedArray.h
    global/Flags.h
    global/NamedColors.cpp
    global/NamedColors.h
    global/NullPointerException.cpp
    global/NullPointerException.h
    global/RAII.cpp
    global/RAII.h
    global/RuleOf5.h
    global/Signal.h
    global/SignalBlocker.cpp
    global/SignalBlocker.h
    global/StringView.cpp
    global/StringView.h
    global/TaggedInt.h
    global/TaggedString.h
    global/TextUtils.cpp
    global/TextUtils.h
    global/Version.h
    global/WeakHandle.cpp
    global/WeakHandle.h
    global/WinSock.cpp
    global/WinSock.h
    global/bits.h
    global/entities.cpp
    global/entities.h
    global/enums.h
    global/hash.h
    global/io.cpp
    global/io.h
    global/macros.h
    global/random.cpp
    global/random.h
    global/range.h
    global/roomid.h
    global/string_view_utils.cpp
    global/string_view_utils.h
    global/unquote.cpp
    global/unquote.h
    global/utils.cpp
    global/utils.h
    logger/autologger.cpp
    logger/autologger.h
    mainwindow/UpdateDialog.cpp
    mainwindow/UpdateDialog.h
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
    mapdata/DoorFlags.h
    mapdata/ExitDirection.cpp
    mapdata/ExitDirection.h
    mapdata/ExitFieldVariant.h
    mapdata/ExitFlags.h
    mapdata/RoomFieldVariant.h
    mapdata/customaction.cpp
    mapdata/customaction.h
    mapdata/drawstream.cpp
    mapdata/drawstream.h
    mapdata/enums.cpp
    mapdata/enums.h
    mapdata/infomark.cpp
    mapdata/infomark.h
    mapdata/mapdata.cpp
    mapdata/mapdata.h
    mapdata/mmapper2exit.h
    mapdata/mmapper2room.h
    mapdata/roomfilter.cpp
    mapdata/roomfilter.h
    mapdata/roomselection.cpp
    mapdata/roomselection.h
    mapdata/shortestpath.cpp
    mapdata/shortestpath.h
    mapfrontend/AbstractRoomVisitor.cpp
    mapfrontend/AbstractRoomVisitor.h
    mapfrontend/ParseTree.cpp
    mapfrontend/ParseTree.h
    mapfrontend/map.cpp
    mapfrontend/map.h
    mapfrontend/mapaction.cpp
    mapfrontend/mapaction.h
    mapfrontend/mapfrontend.cpp
    mapfrontend/mapfrontend.h
    mapfrontend/roomcollection.cpp
    mapfrontend/roomcollection.h
    mapfrontend/roomlocker.cpp
    mapfrontend/roomlocker.h
    mapstorage/MmpMapStorage.cpp
    mapstorage/MmpMapStorage.h
    mapstorage/PandoraMapStorage.cpp
    mapstorage/PandoraMapStorage.h
    mapstorage/StorageUtils.cpp
    mapstorage/StorageUtils.h
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
    mapstorage/XmlMapStorage.cpp
    mapstorage/XmlMapStorage.h
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
    observer/gameobserver.cpp
    observer/gameobserver.h
    opengl/Font.cpp
    opengl/Font.h
    opengl/FontFormatFlags.h
    opengl/OpenGL.cpp
    opengl/OpenGL.h
    opengl/OpenGLTypes.cpp
    opengl/OpenGLTypes.h
    opengl/legacy/AbstractShaderProgram.cpp
    opengl/legacy/AbstractShaderProgram.h
    opengl/legacy/Binders.cpp
    opengl/legacy/Binders.h
    opengl/legacy/FontMesh3d.cpp
    opengl/legacy/FontMesh3d.h
    opengl/legacy/Legacy.cpp
    opengl/legacy/Legacy.h
    opengl/legacy/Meshes.cpp
    opengl/legacy/Meshes.h
    opengl/legacy/ShaderUtils.cpp
    opengl/legacy/ShaderUtils.h
    opengl/legacy/Shaders.cpp
    opengl/legacy/Shaders.h
    opengl/legacy/SimpleMesh.cpp
    opengl/legacy/SimpleMesh.h
    opengl/legacy/VBO.cpp
    opengl/legacy/VBO.h
    opengl/legacy/impl_gl20.cpp
    pandoragroup/CGroup.cpp
    pandoragroup/CGroup.h
    pandoragroup/CGroupChar.cpp
    pandoragroup/CGroupChar.h
    pandoragroup/CGroupCommunicator.cpp
    pandoragroup/CGroupCommunicator.h
    pandoragroup/GroupClient.cpp
    pandoragroup/GroupClient.h
    pandoragroup/GroupManagerApi.cpp
    pandoragroup/GroupManagerApi.h
    pandoragroup/GroupPortMapper.cpp
    pandoragroup/GroupPortMapper.h
    pandoragroup/GroupServer.cpp
    pandoragroup/GroupServer.h
    pandoragroup/GroupSocket.cpp
    pandoragroup/GroupSocket.h
    pandoragroup/enums.cpp
    pandoragroup/enums.h
    pandoragroup/groupaction.cpp
    pandoragroup/groupaction.h
    pandoragroup/groupauthority.cpp
    pandoragroup/groupauthority.h
    pandoragroup/groupselection.cpp
    pandoragroup/groupselection.h
    pandoragroup/groupwidget.cpp
    pandoragroup/groupwidget.h
    pandoragroup/mmapper2character.h
    pandoragroup/mmapper2group.cpp
    pandoragroup/mmapper2group.h
    parser/Abbrev.cpp
    parser/Abbrev.h
    parser/AbstractParser-Actions.cpp
    parser/AbstractParser-Commands.cpp
    parser/AbstractParser-Commands.h
    parser/AbstractParser-Config.cpp
    parser/AbstractParser-Group.cpp
    parser/AbstractParser-Mark.cpp
    parser/AbstractParser-Room.cpp
    parser/AbstractParser-Utils.cpp
    parser/AbstractParser-Utils.h
    parser/AbstractParser-Timer.cpp
    parser/Action.cpp
    parser/Action.h
    parser/CommandId.cpp
    parser/CommandId.h
    parser/CommandQueue.cpp
    parser/CommandQueue.h
    parser/ConnectedRoomFlags.h
    parser/DoorAction.cpp
    parser/DoorAction.h
    parser/ExitsFlags.h
    parser/LineFlags.h
    parser/PromptFlags.h
    parser/abstractparser.cpp
    parser/abstractparser.h
    parser/mumexmlparser.cpp
    parser/mumexmlparser.h
    parser/parserutils.cpp
    parser/parserutils.h
    parser/patterns.cpp
    parser/patterns.h
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
    pathmachine/pathparameters.h
    pathmachine/roomsignalhandler.cpp
    pathmachine/roomsignalhandler.h
    pathmachine/syncing.cpp
    pathmachine/syncing.h
    preferences/AdvancedGraphics.cpp
    preferences/AdvancedGraphics.h
    preferences/AnsiColorDialog.cpp
    preferences/AnsiColorDialog.h
    preferences/ansicombo.cpp
    preferences/ansicombo.h
    preferences/autologpage.cpp
    preferences/autologpage.h
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
    proxy/GmcpMessage.cpp
    proxy/GmcpMessage.h
    proxy/GmcpModule.cpp
    proxy/GmcpModule.h
    proxy/GmcpUtils.cpp
    proxy/GmcpUtils.h
    proxy/MudTelnet.cpp
    proxy/MudTelnet.h
    proxy/ProxyParserApi.cpp
    proxy/ProxyParserApi.h
    proxy/TextCodec.cpp
    proxy/TextCodec.h
    proxy/UserTelnet.cpp
    proxy/UserTelnet.h
    proxy/connectionlistener.cpp
    proxy/connectionlistener.h
    proxy/mumesocket.cpp
    proxy/mumesocket.h
    proxy/proxy.cpp
    proxy/proxy.h
    proxy/telnetfilter.cpp
    proxy/telnetfilter.h
    roompanel/RoomManager.cpp
    roompanel/RoomManager.h
    roompanel/RoomMob.cpp
    roompanel/RoomMob.h
    roompanel/RoomMobs.cpp
    roompanel/RoomMobs.h
    roompanel/RoomWidget.cpp
    roompanel/RoomWidget.h
    syntax/Accept.cpp
    syntax/Accept.h
    syntax/IArgument.cpp
    syntax/IArgument.h
    syntax/IMatchErrorLogger.h
    syntax/MatchResult.cpp
    syntax/MatchResult.h
    syntax/ParseResult.h
    syntax/ParserInput.cpp
    syntax/ParserInput.h
    syntax/Sublist.cpp
    syntax/Sublist.h
    syntax/SyntaxArgs.cpp
    syntax/SyntaxArgs.h
    syntax/TokenMatcher.cpp
    syntax/TokenMatcher.h
    syntax/TreeParser.cpp
    syntax/TreeParser.h
    syntax/User.h
    syntax/Value.cpp
    syntax/Value.h
    timers/CTimers.cpp
    timers/CTimers.h
)

set(mmapper_RCS
    resources/mmapper2.qrc)

if(MMAPPER_BETA)
    list(APPEND mmapper_RCS "resources/beta.qrc")
    list(APPEND mmapper_RCS "resources/win32/beta.rc")
    set(MMAPPER_BRAND_SUFFIX "beta")
else()
    list(APPEND mmapper_RCS "resources/release.qrc")
    list(APPEND mmapper_RCS "resources/win32/release.rc")
    set(MMAPPER_BRAND_SUFFIX "release")
endif()
list(APPEND mmapper_RCS "resources/macosx/m-${MMAPPER_BRAND_SUFFIX}.icns")

if(WITH_MAP)
    configure_file(resources/arda.qrc ${CMAKE_BINARY_DIR}/map/arda.qrc COPYONLY)
    list(APPEND mmapper_RCS "${CMAKE_BINARY_DIR}/map/arda.qrc")
endif()

set(mmapper_UIS
    client/ClientWidget.ui
    clock/mumeclockwidget.ui
    mainwindow/aboutdialog.ui
    mainwindow/infomarkseditdlg.ui
    mainwindow/findroomsdlg.ui
    mainwindow/roomeditattrdlg.ui
    preferences/AnsiColorDialog.ui
    preferences/autologpage.ui
    preferences/configdialog.ui
    preferences/clientpage.ui
    preferences/generalpage.ui
    preferences/groupmanagerpage.ui
    preferences/graphicspage.ui
    preferences/mumeprotocolpage.ui
    preferences/parserpage.ui
    preferences/pathmachinepage.ui
)

if(MMAPPER_IS_DEBUG)
    if(NOT DEFINED CHECK_ODR)
        set(CHECK_ODR YES)
    endif()
    if(NOT DEFINED CHECK_MISSING_INCLUDES)
        set(CHECK_MISSING_INCLUDES YES)
    endif()
endif()

configure_file(global/Version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp)
list(APPEND mmapper_SRCS "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp")

if(CHECK_ODR)
    message(STATUS "Will check headers for ODR violations (slow)")
    # ODR Violation Check
    file(GLOB_RECURSE mmapper_HDRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)
    list(SORT mmapper_HDRS)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/odr.cpp "// This generated file exists to help detect ODR violations.\n"
        "// It will catch duplicate class names in header files,\n"
        "// but it will NOT catch duplicate names in source files.\n\n")
    foreach(header_path ${mmapper_HDRS})
        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/odr.cpp "#include \"${header_path}\"\n")
    endforeach()
    # REVISIT: Find a way to avoid adding this to mmapper_SRCS?
    list(APPEND mmapper_SRCS "${CMAKE_CURRENT_BINARY_DIR}/odr.cpp")
endif()

if(CHECK_MISSING_INCLUDES)
    message(STATUS "Will check headers for missing includes (slow)")
    foreach(header_path ${mmapper_HDRS})
        set(FNAME "${CMAKE_CURRENT_BINARY_DIR}/gen/${header_path}.gen.cpp")
        file(WRITE ${FNAME} "#include \"${header_path}\"\n")
        list(APPEND DUMMY_FILES ${FNAME})
        set(FNAME "")
    endforeach()
    # REVISIT: Find a way to avoid adding this to mmapper_SRCS?
    list(APPEND mmapper_SRCS ${DUMMY_FILES})
endif()

set(mmapper_DATA
    org.mume.MMapper.desktop
    MacOSXBundleInfo.plist.in
    global/Version.cpp.in)

# Build the executable
add_executable(mmapper WIN32 MACOSX_BUNDLE
    ${mmapper_SRCS}
    ${mmapper_UIS}
    ${mmapper_RCS}
    ${mmapper_DATA}
)

target_link_libraries(mmapper PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::OpenGL
)

set_target_properties(
  mmapper PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
  UNITY_BUILD ${USE_UNITY_BUILD}
)

target_include_directories(mmapper SYSTEM PUBLIC ${GLM_INCLUDE_DIR})
add_dependencies(mmapper glm)

if(WIN32)
    target_link_libraries(mmapper PUBLIC ws2_32)
endif()

if(WITH_ZLIB)
    target_include_directories(mmapper SYSTEM PUBLIC ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(mmapper PUBLIC ${ZLIB_LIBRARIES})
    if(NOT ZLIB_FOUND)
        add_dependencies(mmapper zlib)
    endif()
endif()

if(WITH_OPENSSL)
    target_include_directories(mmapper SYSTEM PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(mmapper PUBLIC ${OPENSSL_LIBRARIES})
    if(NOT OPENSSL_FOUND)
        add_dependencies(mmapper openssl)
    endif()
endif()

if(WITH_MINIUPNPC)
    target_include_directories(mmapper SYSTEM PUBLIC ${MINIUPNPC_INCLUDE_DIR})
    target_link_libraries(mmapper PUBLIC ${MINIUPNPC_LIBRARY})
    if(NOT MINIUPNPC_FOUND)
        add_dependencies(mmapper miniupnpc)
    endif()
endif()

if(WITH_DRMINGW)
    target_include_directories(mmapper SYSTEM PUBLIC ${DRMINGW_INCLUDE_DIR})
    target_link_libraries(mmapper PUBLIC ${DRMINGW_LIBRARY})
    add_dependencies(mmapper drmingw)
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
if(GIT_TAG_NAME)
    string(REGEX REPLACE "v(.*)" "\\1" CPACK_PACKAGE_VERSION ${GIT_TAG_NAME})
elseif(GIT_TAG_COMMIT_HASH)
    string(REGEX REPLACE "v(.*)" "\\1" CPACK_PACKAGE_VERSION ${GIT_TAG_COMMIT_HASH})
else()
    set(CPACK_PACKAGE_VERSION ${MMAPPER_VERSION})
endif()
string(REGEX REPLACE "[.-]" " " VERSION_NUMBERS ${MMAPPER_VERSION})
separate_arguments(VERSION_NUMBERS)
list(GET VERSION_NUMBERS 0 CPACK_PACKAGE_VERSION_MAJOR)
list(REMOVE_AT VERSION_NUMBERS 0)
list(GET VERSION_NUMBERS 0 CPACK_PACKAGE_VERSION_MINOR)
list(REMOVE_AT VERSION_NUMBERS 0)
list(GET VERSION_NUMBERS 0 CPACK_PACKAGE_VERSION_PATCH)
string(REGEX REPLACE "Darwin" "Mac" CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-source")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
if(NOT TARGET_ARCHITECTURE)
    if(WIN32)
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(TARGET_ARCHITECTURE "x64")
        else()
            set(TARGET_ARCHITECTURE "x86")
        endif()
    else()
        set(TARGET_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()
message(STATUS "Packaging for target: ${CPACK_SYSTEM_NAME} ${TARGET_ARCHITECTURE}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}-${TARGET_ARCHITECTURE}")

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

    # Linux Desktop Integration
    install(FILES org.mume.MMapper.desktop
            DESTINATION share/applications
            COMPONENT desktopintegration
    )

    # Install icons
    foreach(RES 16 32 48 96 128 256)
        install(
            FILES resources/icons/hi${RES}-app-mmapper-${MMAPPER_BRAND_SUFFIX}.png
            DESTINATION share/icons/hicolor/${RES}x${RES}/apps/
            COMPONENT desktopintegration
            RENAME org.mume.MMapper.png
        )
    endforeach(RES)

    install(
        FILES resources/icons/m-${MMAPPER_BRAND_SUFFIX}.png
        DESTINATION share/icons/hicolor/512x512/apps/
        COMPONENT desktopintegration
        RENAME org.mume.MMapper.png
    )

    set(CPACK_GENERATOR "DEB") # Others: RPM, STGZ
    set(CPACK_SOURCE_GENERATOR "TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nschimme@gmail.com")
    set(CPACK_DEBIAN_PACKAGE_NAME "mmapper")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.15.0), libqt5gui5 (>= 5.15.0), libqt5opengl5 (>= 5.15.0), libqt5network5 (>= 5.15.0)")
    if(WITH_ZLIB)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g")
    endif()
    if(WITH_OPENSSL)
        if (OPENSSL_VERSION VERSION_LESS 3.0.0)
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libssl1.1")
        else()
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libssl3")
        endif()
    endif()
    if(WITH_MINIUPNPC)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libminiupnpc${MINIUPNPC_API_VERSION}")
    endif()
endif(UNIX AND NOT APPLE)

# Windows Install Settings
if(WIN32)
    include(InstallRequiredSystemLibraries)
    set(CMAKE_WIN32_EXECUTABLE ON)

    # Install Executables
    install(
        TARGETS mmapper RUNTIME
        DESTINATION bin
        COMPONENT applications
    )

    # Bundle Library Files
    set(WINDEPLOYQT_ARGS ${WINDEPLOYQT_ARGS} --dir ${CMAKE_CURRENT_BINARY_DIR}/bin --compiler-runtime --no-angle)
    find_program(WINDEPLOYQT_APP windeployqt HINTS ${QTDIR} ENV QTDIR PATH_SUFFIXES bin)
    message(" - windeployqt path: ${WINDEPLOYQT_APP}")
    message(" - windeployqt args: ${WINDEPLOYQT_ARGS}")
    add_custom_command(
            TARGET mmapper
            POST_BUILD
            COMMAND ${WINDEPLOYQT_APP} ${WINDEPLOYQT_ARGS} ${CMAKE_CURRENT_BINARY_DIR}/mmapper.exe
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Finding the Qt framework dependencies"
    )
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/bin
        DESTINATION .
        COMPONENT libraries
    )
    if(MSVC)
        get_filename_component(vcredist_name "${MSVC_REDIST}" NAME)
        install(FILES ${MSVC_REDIST} DESTINATION bin COMPONENT libraries)
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\bin\\\\${vcredist_name}\\\"'")
    endif()

    # ZIP requires 7-zip
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_SOURCE_GENERATOR "ZIP")

    # Windows (NSIS) Settings
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_MENU_LINKS "bin/mmapper.exe;MMapper")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "mmapper.exe")
    set(CPACK_NSIS_HELP_LINK "http://github.com/mume/mmapper")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m-${MMAPPER_BRAND_SUFFIX}.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m-${MMAPPER_BRAND_SUFFIX}.ico")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons\\\\m-${MMAPPER_BRAND_SUFFIX}.png")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m-${MMAPPER_BRAND_SUFFIX}.ico")
    set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "mmapper.exe")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE "ON")
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(CPACK_NSIS_DISPLAY_NAME "MUME Mapper (x64)")
    else()
        set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Caligor soft")
        set(CPACK_NSIS_DISPLAY_NAME "MUME Mapper (x86)")
    endif()
    if(WITH_DRMINGW)
        # Include shortcut to crash logs
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
            CreateShortCut \\\"$SMPROGRAMS\\\\${CPACK_NSIS_DISPLAY_NAME}\\\\Crash Log.lnk\\\" \\\"$PROFILE\\\\AppData\\\\Local\\\\mmappercrash.log\\\"
            ")
        # Delete shortcut and crash log
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
            Delete \\\"$SMPROGRAMS\\\\${CPACK_NSIS_DISPLAY_NAME}\\\\Crash Log.lnk\\\"
            Delete \\\"$PROFILE\\\\AppData\\\\Local\\\\mmappercrash.log\\\"
            ")
    endif()
    # NSIS always displays a license page
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/src/resources/LICENSE.GPL2")
endif(WIN32)

# Apple Install Settings
if(APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
       set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR} CACHE PATH "Mac default path is pwd" FORCE)
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

    set(CMAKE_MACOSX_BUNDLE ON)
    set_source_files_properties(resources/macosx/m-${MMAPPER_BRAND_SUFFIX}.icns PROPERTIES
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

    # Codesign the bundle without a personal certificate
    find_program(CODESIGN_APP codesign)
    message(" - codesign path: ${CODESIGN_APP}")
    add_custom_command(
        TARGET mmapper
        POST_BUILD
        COMMAND ${CODESIGN_APP} --remove-signature ${CMAKE_CURRENT_BINARY_DIR}/mmapper.app
        COMMAND ${CODESIGN_APP} --deep -f -s - ${CMAKE_CURRENT_BINARY_DIR}/mmapper.app -o runtime --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Code signing the bundle"
        )

    # Install Executables
    install(
        TARGETS mmapper RUNTIME BUNDLE
        DESTINATION .
        COMPONENT applications
    )

    # Package Settings
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_DMG_FORMAT UDBZ)

    # Libraries are bundled directly
    set(CPACK_COMPONENT_LIBRARIES_HIDDEN TRUE)

    # Bundle Properties
    set(MACOSX_BUNDLE_BUNDLE_NAME MMapper)
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${MMAPPER_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${MMAPPER_VERSION_STRING})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MMAPPER_VERSION_STRING}")
    set(MACOSX_BUNDLE_ICON_FILE m-${MMAPPER_BRAND_SUFFIX}.icns)
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2019 The MMapper Team")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.mume.mmapper")
endif(APPLE)

# More General CPack Settings
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Nils Schimmelmann")
set(CPACK_PACKAGE_CONTACT "nschimme@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A mud mapper especially written for the mud MUME")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MMapper")

# Components:
if(CMAKE_BUILD_TYPE_UPPER MATCHES "^(DEBUG|RELWITHDEBINFO)$")
    set(CPACK_STRIP_FILES FALSE)
else()
    set(CPACK_STRIP_FILES TRUE)
endif()
# Applications Component
set(CPACK_COMPONENTS_ALL applications libraries)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "MMapper")
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "A mud mapper especially written for the mud MUME")
set(CPACK_COMPONENT_APPLICATIONS_REQUIRED TRUE)

# Libraries Component
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Runtime libraries for running MMapper")
set(CPACK_COMPONENT_LIBRARIES_REQUIRED TRUE)

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};applications;ALL;/")

# Source Package
set(CPACK_SOURCE_IGNORE_FILES
    "~$"
    "/\\\\.git/"
    "/build/"
    "/winbuild/"
)
include(CPack)
