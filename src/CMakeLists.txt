set(mmapper_SRCS
    main.cpp
    adventure/adventuresession.cpp
    adventure/adventuresession.h
    adventure/adventuretracker.cpp
    adventure/adventuretracker.h
    adventure/adventurewidget.cpp
    adventure/adventurewidget.h
    adventure/lineparsers.cpp
    adventure/lineparsers.h
    adventure/xpstatuswidget.cpp
    adventure/xpstatuswidget.h
    client/ClientTelnet.cpp
    client/ClientTelnet.h
    client/ClientWidget.cpp
    client/ClientWidget.h
    client/PaletteManager.cpp
    client/PaletteManager.h
    client/displaywidget.cpp
    client/displaywidget.h
    client/inputwidget.cpp
    client/inputwidget.h
    client/stackedinputwidget.cpp
    client/stackedinputwidget.h
    client/PasswordDialog.cpp
    client/PasswordDialog.h
    client/PreviewWidget.cpp
    client/PreviewWidget.h
    clock/mumeclock.cpp
    clock/mumeclock.h
    clock/mumeclockwidget.cpp
    clock/mumeclockwidget.h
    clock/mumemoment.cpp
    clock/mumemoment.h
    configuration/NamedConfig.h
    configuration/PasswordConfig.cpp
    configuration/PasswordConfig.h
    configuration/configuration.cpp
    configuration/configuration.h
    display/CanvasMouseModeEnum.h
    display/Characters.cpp
    display/Characters.h
    display/ConnectionLineBuilder.cpp
    display/ConnectionLineBuilder.h
    display/Connections.cpp
    display/Connections.h
    display/Filenames.cpp
    display/Filenames.h
    display/IMapBatchesFinisher.cpp
    display/IMapBatchesFinisher.h
    display/InfomarkSelection.cpp
    display/InfomarkSelection.h
    display/Infomarks.cpp
    display/Infomarks.h
    display/MapBatches.cpp
    display/MapBatches.h
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
    global/AnsiOstream.cpp
    global/AnsiOstream.h
    global/AnsiTextUtils.cpp
    global/AnsiTextUtils.h
    global/Array.h
    global/AsyncTasks.cpp
    global/AsyncTasks.h
    global/Badge.cpp
    global/Badge.h
    global/CaseUtils.cpp
    global/CaseUtils.h
    global/ChangeMonitor.h
    global/CharUtils.cpp
    global/CharUtils.h
    global/Charset-Utf16.cpp
    global/Charset-Utf8.cpp
    global/Charset.cpp
    global/Charset.h
    global/Color.cpp
    global/Color.h
    global/ConfigConsts-Computed.h
    global/ConfigConsts.h
    global/ConfigEnums.h
    global/Connections.cpp
    global/Connections.h
    global/Consts.cpp
    global/Consts.h
    global/ConversionResult.h
    global/Diff.cpp
    global/Diff.h
    global/EnumIndexedArray.h
    global/Flags.cpp
    global/Flags.h
    global/HideQDebug.cpp
    global/HideQDebug.h
    global/ImmIndexedVector.h
    global/ImmIndexedVectorWithDefault.h
    global/ImmOrderedSet.h
    global/ImmUnorderedMap.h
    global/ImmUnorderedSet.h
    global/IndexedVector.h
    global/IndexedVectorWithDefault.cpp
    global/IndexedVectorWithDefault.h
    global/JsonArray.h
    global/JsonDoc.h
    global/JsonObj.cpp
    global/JsonObj.h
    global/JsonValue.h
    global/JsonValue.cpp
    global/LineUtils.cpp
    global/LineUtils.h
    global/MakeQPointer.h
    global/MmQtHandle.h
    global/NamedColors.cpp
    global/NamedColors.h
    global/NullPointerException.cpp
    global/NullPointerException.h
    global/OrderedMap.cpp
    global/OrderedMap.h
    global/OrderedSet.cpp
    global/OrderedSet.h
    global/PrintUtils.cpp
    global/PrintUtils.h
    global/RAII.cpp
    global/RAII.h
    global/RuleOf5.h
    global/SendToUser.cpp
    global/SendToUser.h
    global/Signal2.h
    global/SignalBlocker.cpp
    global/SignalBlocker.h
    global/StorageUtils.cpp
    global/StorageUtils.h
    global/StringView.cpp
    global/StringView.h
    global/TabUtils.cpp
    global/TabUtils.h
    global/TaggedInt.h
    global/TaggedString.cpp
    global/TaggedString.h
    global/TextBuffer.cpp
    global/TextBuffer.h
    global/TextUtils.cpp
    global/TextUtils.h
    global/Timer.cpp
    global/Timer.h
    global/Version.h
    global/WeakHandle.cpp
    global/WeakHandle.h
    global/WinSock.cpp
    global/WinSock.h
    global/bits.h
    global/cast_error.cpp
    global/cast_error.h
    global/emojis.cpp
    global/emojis.h
    global/entities-xforeach.txt
    global/entities.cpp
    global/entities.h
    global/enums.h
    global/float_cast.cpp
    global/float_cast.h
    global/hash.h
    global/int_cast.cpp
    global/int_cast.h
    global/io.cpp
    global/io.h
    global/logging.cpp
    global/logging.h
    global/macros.h
    global/parserutils.cpp
    global/parserutils.h
    global/progresscounter.cpp
    global/progresscounter.h
    global/random.cpp
    global/random.h
    global/range.h
    global/string_view_utils.cpp
    global/string_view_utils.h
    global/tests.cpp
    global/tests.h
    global/thread_utils.cpp
    global/thread_utils.h
    global/unquote.cpp
    global/unquote.h
    global/utils.cpp
    global/utils.h
    global/window_utils.cpp
    global/window_utils.h
    global/zpipe.cpp
    global/zpipe.h
    group/CGroupChar.cpp
    group/CGroupChar.h
    group/ColorGenerator.cpp
    group/ColorGenerator.h
    group/GroupManagerApi.cpp
    group/GroupManagerApi.h
    group/enums.cpp
    group/enums.h
    group/groupwidget.cpp
    group/groupwidget.h
    group/mmapper2character.cpp
    group/mmapper2character.h
    group/mmapper2group.cpp
    group/mmapper2group.h
    group/tokenmanager.cpp
    group/tokenmanager.h
    logger/autologger.cpp
    logger/autologger.h
    mainwindow/DescriptionWidget.cpp
    mainwindow/DescriptionWidget.h
    mainwindow/MapZoomSlider.cpp
    mainwindow/MapZoomSlider.h
    mainwindow/UpdateDialog.cpp
    mainwindow/UpdateDialog.h
    mainwindow/WinDarkMode.cpp
    mainwindow/WinDarkMode.h
    mainwindow/aboutdialog.cpp
    mainwindow/aboutdialog.h
    mainwindow/findroomsdlg.cpp
    mainwindow/findroomsdlg.h
    mainwindow/infomarkseditdlg.cpp
    mainwindow/infomarkseditdlg.h
    mainwindow/mainwindow-async.cpp
    mainwindow/mainwindow-saveslots.cpp
    mainwindow/mainwindow.cpp
    mainwindow/mainwindow.h
    mainwindow/metatypes.cpp
    mainwindow/metatypes.h
    mainwindow/roomeditattrdlg.cpp
    mainwindow/roomeditattrdlg.h
    mainwindow/utils.cpp
    mainwindow/utils.h
    map/AbstractChangeVisitor.cpp
    map/AbstractChangeVisitor.h
    map/Change.cpp
    map/Change.h
    map/ChangeList.cpp
    map/ChangeList.h
    map/ChangePrinter.cpp
    map/ChangePrinter.h
    map/ChangeTypes.cpp
    map/ChangeTypes.h
    map/Changes.cpp
    map/Changes.h
    map/CommandId.cpp
    map/CommandId.h
    map/Compare.h
    map/ConnectedRoomFlags.h
    map/Crtp.h
    map/Diff-Test.cpp
    map/Diff.cpp
    map/Diff.h
    map/DoorFlags.cpp
    map/DoorFlags.h
    map/ExitDirection.cpp
    map/ExitDirection.h
    map/ExitFieldVariant.cpp
    map/ExitFieldVariant.h
    map/ExitFields.cpp
    map/ExitFields.h
    map/ExitFlags.cpp
    map/ExitFlags.h
    map/ExitsFlags.h
    map/InOutEnum.cpp
    map/InOutEnum.h
    map/InvalidMapOperation.cpp
    map/InvalidMapOperation.h
    map/Map.cpp
    map/Map.h
    map/MapConsistencyError.cpp
    map/MapConsistencyError.h
    map/ParseTree.cpp
    map/ParseTree.h
    map/PromptFlags.cpp
    map/PromptFlags.h
    map/RawExit.cpp
    map/RawExit.h
    map/RawRoom.cpp
    map/RawRoom.h
    map/RawRooms.cpp
    map/RawRooms.h
    map/Remapping.cpp
    map/Remapping.h
    map/RoomFieldVariant.h
    map/RoomFields.cpp
    map/RoomFields.h
    map/RoomHandle.cpp
    map/RoomHandle.h
    map/RoomIdSet.cpp
    map/RoomIdSet.h
    map/RoomRevert.cpp
    map/RoomRevert.h
    map/ServerIdMap.cpp
    map/ServerIdMap.h
    map/SpatialDb.cpp
    map/SpatialDb.h
    map/TinyRoomIdSet.cpp
    map/TinyRoomIdSet.h
    map/World-BaseMap.cpp
    map/World.cpp
    map/World.h
    map/WorldAreaMap.cpp
    map/WorldAreaMap.h
    map/WorldBuilder.cpp
    map/WorldBuilder.h
    map/coordinate.cpp
    map/coordinate.h
    map/enums.cpp
    map/enums.h
    map/exit.h
    map/infomark.cpp
    map/infomark.h
    map/mmapper2room.cpp
    map/mmapper2room.h
    map/parseevent.cpp
    map/parseevent.h
    map/room.cpp
    map/room.h
    map/roomid.cpp
    map/roomid.h
    map/sanitizer.cpp
    map/sanitizer.h
    map/utils.cpp
    map/utils.h
    mapdata/GenericFind.cpp
    mapdata/GenericFind.h
    mapdata/MarkerList.cpp
    mapdata/MarkerList.h
    mapdata/mapdata.cpp
    mapdata/mapdata.h
    mapdata/roomfilter.cpp
    mapdata/roomfilter.h
    mapdata/roomselection.cpp
    mapdata/roomselection.h
    mapdata/shortestpath.cpp
    mapdata/shortestpath.h
    mapfrontend/MapHistory.cpp
    mapfrontend/MapHistory.h
    mapfrontend/mapfrontend.cpp
    mapfrontend/mapfrontend.h
    mapstorage/MmpMapStorage.cpp
    mapstorage/MmpMapStorage.h
    mapstorage/PandoraMapStorage.cpp
    mapstorage/PandoraMapStorage.h
    mapstorage/RawMapData.cpp
    mapstorage/RawMapData.h
    mapstorage/XmlMapStorage.cpp
    mapstorage/XmlMapStorage.h
    mapstorage/abstractmapstorage.cpp
    mapstorage/abstractmapstorage.h
    mapstorage/filesaver.cpp
    mapstorage/filesaver.h
    mapstorage/jsonmapstorage.cpp
    mapstorage/jsonmapstorage.h
    mapstorage/mapstorage.cpp
    mapstorage/mapstorage.h
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
    mpi/gotowidget.cpp
    mpi/gotowidget.h
    mpi/findreplacewidget.cpp
    mpi/findreplacewidget.h
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
    parser/Abbrev.cpp
    parser/Abbrev.h
    parser/AbstractParser-Actions.cpp
    parser/AbstractParser-Commands.cpp
    parser/AbstractParser-Commands.h
    parser/AbstractParser-Config.cpp
    parser/AbstractParser-Group.cpp
    parser/AbstractParser-Mark.cpp
    parser/AbstractParser-Room.cpp
    parser/AbstractParser-Timer.cpp
    parser/AbstractParser-Utils.cpp
    parser/AbstractParser-Utils.h
    parser/Action.cpp
    parser/Action.h
    parser/CommandQueue.cpp
    parser/CommandQueue.h
    parser/DoorAction.cpp
    parser/DoorAction.h
    parser/LineFlags.h
    parser/SendToUserSourceEnum.h
    parser/abstractparser.cpp
    parser/abstractparser.h
    parser/mumexmlparser-gmcp.cpp
    parser/mumexmlparser.cpp
    parser/mumexmlparser.h
    pathmachine/approved.cpp
    pathmachine/approved.h
    pathmachine/crossover.cpp
    pathmachine/crossover.h
    pathmachine/experimenting.cpp
    pathmachine/experimenting.h
    pathmachine/mmapper2pathmachine.cpp
    pathmachine/mmapper2pathmachine.h
    pathmachine/onebyone.cpp
    pathmachine/onebyone.h
    pathmachine/path.cpp
    pathmachine/path.h
    pathmachine/pathmachine.cpp
    pathmachine/pathmachine.h
    pathmachine/pathparameters.h
    pathmachine/pathprocessor.cpp
    pathmachine/pathprocessor.h
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
    preferences/grouppage.cpp
    preferences/grouppage.h
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
    viewers/AnsiViewWindow.cpp
    viewers/AnsiViewWindow.h
    viewers/LaunchAsyncViewer.h
    viewers/TopLevelWindows.cpp
    viewers/TopLevelWindows.h
)

qt_add_resources(mmapper_QRC resources/mmapper2.qrc)

if(MMAPPER_BETA)
    list(APPEND mmapper_RCS "resources/win32/beta.rc")
    set(MMAPPER_BRAND_SUFFIX "beta")
else()
    list(APPEND mmapper_RCS "resources/win32/release.rc")
    set(MMAPPER_BRAND_SUFFIX "release")
endif()
configure_file(resources/build.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/build.qrc)
qt_add_resources(mmapper_QRC ${CMAKE_CURRENT_BINARY_DIR}/build.qrc)
list(APPEND mmapper_RCS "resources/macosx/m-${MMAPPER_BRAND_SUFFIX}.icns")

if(WITH_MAP)
    qt_add_resources(mmapper_QRC "${CMAKE_BINARY_DIR}/map/map.qrc")
endif()

if(WITH_IMAGES)
    qt_add_resources(mmapper_QRC "${CMAKE_BINARY_DIR}/images/images.qrc")
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
    preferences/graphicspage.ui
    preferences/grouppage.ui
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
        set_property(SOURCE ${FNAME} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${header_path})
        set(FNAME "")
    endforeach()
    # REVISIT: Find a way to avoid adding this to mmapper_SRCS?
    list(APPEND mmapper_SRCS ${DUMMY_FILES})
endif()

set(mmapper_DATA
    org.mume.MMapper.desktop
    MacOSXBundleInfo.plist.in
    global/Version.cpp.in)

set(mmapper_map_SRCS ${mmapper_SRCS})
set(mmapper_global_SRCS ${mmapper_SRCS})
list(FILTER mmapper_map_SRCS INCLUDE REGEX "map/")
list(FILTER mmapper_global_SRCS INCLUDE REGEX "global/")
list(FILTER mmapper_SRCS EXCLUDE REGEX "map/")
list(FILTER mmapper_SRCS EXCLUDE REGEX "global/")

message("mmapper_SRCS=[${mmapper_SRCS}]")
message("mmapper_global_SRCS=[${mmapper_global_SRCS}]")
message("mmapper_map_SRCS=[${mmapper_map_SRCS}]")

add_library(mm_global STATIC ${mmapper_global_SRCS})
add_library(mm_map STATIC ${mmapper_map_SRCS})

# Build the executable
add_executable(mmapper WIN32 MACOSX_BUNDLE
    ${mmapper_SRCS}
    ${mmapper_UIS}
    ${mmapper_RCS}
    ${mmapper_QRC}
    ${mmapper_DATA}
    display/GhostRegistry.h
)

set(mm_libs mm_map mm_global)

target_link_libraries(mmapper PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    # note: link order matters on some platforms. include ${mm_libs} twice if you have a link failure.
    ${mm_libs}
    ${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(
  mmapper PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
  INSTALL_RPATH "${QTKEYCHAIN_LIBRARY_DIR}"
  BUILD_WITH_INSTALL_RPATH ON
)

set_target_properties(
  mm_global PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)

set_target_properties(
  mm_map PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)

target_include_directories(mmapper SYSTEM PUBLIC ${GLM_INCLUDE_DIR})
target_include_directories(mmapper SYSTEM PUBLIC ${IMMER_INCLUDE_DIR})
target_include_directories(mm_global SYSTEM PUBLIC
        ${GLM_INCLUDE_DIR}
        ${IMMER_INCLUDE_DIR}
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        #${Qt6_Network_INCLUDE_DIRS}
        #${Qt6_OpenGL_INCLUDE_DIRS}
        #${Qt6OpenGLWidgets_INCLUDE_DIRS}
)
target_include_directories(mm_map SYSTEM PUBLIC
        ${GLM_INCLUDE_DIR}
        ${IMMER_INCLUDE_DIR}
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        #${Qt6_Network_INCLUDE_DIRS}
        #${Qt6_OpenGL_INCLUDE_DIRS}
        #${Qt6OpenGLWidgets_INCLUDE_DIRS}
)
target_link_libraries(mm_global PUBLIC
    Qt6::Core
    Qt6::Widgets
)
target_link_libraries(mm_map PUBLIC
    Qt6::Core
    Qt6::Widgets
)
add_dependencies(mmapper mm_global mm_map)
add_dependencies(mm_map mm_global)

if(NOT GLM_FOUND)
    add_dependencies(mmapper glm)
    add_dependencies(mm_map glm)
    add_dependencies(mm_global glm)
endif()
if(NOT IMMER_FOUND)
    add_dependencies(mmapper immer)
    add_dependencies(mm_map immer)
    add_dependencies(mm_global immer)
endif()

if(WIN32)
    target_link_libraries(mmapper PRIVATE ws2_32)
    target_link_libraries(mmapper PRIVATE dwmapi)
endif()

if(WITH_MAP)
    add_dependencies(mmapper map_qrc)
endif()

if(WITH_IMAGES)
    add_dependencies(mmapper images_qrc)
endif()

if(WITH_ZLIB)
    target_include_directories(mmapper SYSTEM PUBLIC ${ZLIB_INCLUDE_DIRS})
    target_include_directories(mm_global SYSTEM PUBLIC ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(mmapper PUBLIC ${ZLIB_LIBRARIES})
    target_link_libraries(mm_global PUBLIC ${ZLIB_LIBRARIES})
    if(NOT ZLIB_FOUND)
        add_dependencies(mmapper zlib)
        add_dependencies(mm_global zlib)
    endif()
endif()

if(WITH_OPENSSL)
    target_include_directories(mmapper SYSTEM PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(mmapper PUBLIC ${OPENSSL_LIBRARIES})
    if(NOT OPENSSL_FOUND)
        add_dependencies(mmapper openssl)
    endif()
endif()

if(WITH_WEBSOCKET)
    target_link_libraries(mmapper PUBLIC Qt6::WebSockets)
endif()

if(WITH_QTKEYCHAIN)
    target_include_directories(mmapper SYSTEM PUBLIC ${QTKEYCHAIN_INCLUDE_DIR})
    target_link_libraries(mmapper PUBLIC ${QTKEYCHAIN_LIBRARY})
    if(NOT QTKEYCHAIN_FOUND)
        add_dependencies(mmapper qtkeychain)
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


# Begin CPack Settings
if(GIT_TAG_NAME)
    string(REGEX REPLACE "v(.*)" "\\1" CPACK_PACKAGE_VERSION ${GIT_TAG_NAME})
elseif(GIT_TAG_COMMIT_HASH)
    string(REGEX REPLACE "v(.*)" "\\1" CPACK_PACKAGE_VERSION ${GIT_TAG_COMMIT_HASH})
else()
    set(CPACK_PACKAGE_VERSION ${MMAPPER_VERSION})
endif()
# Parse CPACK_PACKAGE_VERSION to get individual components
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]+)?.*$" _ "${CPACK_PACKAGE_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
set(CPACK_PACKAGE_VERSION_TWEAK ${CMAKE_MATCH_4})
if(NOT CPACK_PACKAGE_VERSION_TWEAK)
    # Set to 0 if the commit count is missing
    set(CPACK_PACKAGE_VERSION_TWEAK 0)
endif()

string(REGEX REPLACE "Darwin" "Mac" CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-source")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
if(NOT TARGET_ARCHITECTURE)
    if(WIN32)
        set(TARGET_ARCHITECTURE "x64")
    else()
        set(TARGET_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()
message(STATUS "Packaging for target: ${CPACK_SYSTEM_NAME} ${TARGET_ARCHITECTURE}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}-${TARGET_ARCHITECTURE}")

# Components:
if(CMAKE_BUILD_TYPE_UPPER MATCHES "^(DEBUG|RELWITHDEBINFO)$")
    set(CPACK_STRIP_FILES FALSE)
else()
    set(CPACK_STRIP_FILES TRUE)
endif()

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
    set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6 (>= 6.2.0), libqt6gui6 (>= 6.2.0), libqt6opengl6 (>= 6.2.0), libqt6network6 (>= 6.2.0)")
    if(WITH_ZLIB)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, zlib1g")
    endif()
    if(WITH_OPENSSL)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libssl3")
    endif()
    if(WITH_WEBSOCKET)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libqt6websockets6 (>= 6.2.0)")
    endif()
    if(WITH_QTKEYCHAIN)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libqt6keychain1")
    endif()
endif(UNIX AND NOT APPLE)

# Windows Install Settings
if(WIN32)
    if(MMAPPER_IS_DEBUG)
        set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
    endif()
    include(InstallRequiredSystemLibraries)
    message(STATUS "System runtime libs: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")
    set(CMAKE_WIN32_EXECUTABLE ON)

    # Install Executables
    install(
        TARGETS mmapper RUNTIME
        DESTINATION .
        COMPONENT applications
    )

    # Debug symbols
    set(WINDEPLOYQT_STAGING ${CMAKE_CURRENT_BINARY_DIR}/bin)
    if(MSVC AND NOT CPACK_STRIP_FILES)
        add_custom_command(TARGET mmapper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDEPLOYQT_STAGING}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:mmapper>/mmapper.pdb"
                    "${WINDEPLOYQT_STAGING}/mmapper.pdb"
            COMMENT "Copy debug symbols"
            VERBATIM
        )
    endif()

    # Bundle Library Files
    set(WINDEPLOYQT_ARGS ${WINDEPLOYQT_ARGS} --compiler-runtime --no-translations --no-system-d3d-compiler --no-opengl-sw --verbose 1)
    find_program(WINDEPLOYQT_APP windeployqt HINTS ${QTDIR} ENV QTDIR PATH_SUFFIXES .)
    message(" - windeployqt path: ${WINDEPLOYQT_APP}")
    message(" - windeployqt args: ${WINDEPLOYQT_ARGS}")
    add_custom_command(
            TARGET mmapper
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDEPLOYQT_STAGING}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mmapper>" ${WINDEPLOYQT_STAGING}
            COMMAND "${WINDEPLOYQT_APP}" ${WINDEPLOYQT_ARGS} "${WINDEPLOYQT_STAGING}/mmapper.exe"
            WORKING_DIRECTORY "${WINDEPLOYQT_STAGING}"
            COMMENT "Finding the Qt framework dependencies"
            VERBATIM
    )
    install(DIRECTORY
        "${WINDEPLOYQT_STAGING}/"
        DESTINATION .
        COMPONENT libraries
        USE_SOURCE_PERMISSIONS
        PATTERN "*.exe" EXCLUDE
    )
    if(MSVC)
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
            ExecWait '\\$INSTDIR\\\\vc_redist.x64.exe /q /norestart'
            Delete '\\$INSTDIR\\\\vc_redist.x64.exe'
            ")
    endif()

    set(CPACK_SOURCE_GENERATOR "ZIP")

    if(WITH_UPDATER)
        set(CPACK_GENERATOR "NSIS")

        # Windows (NSIS) Settings
        set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
        set(CPACK_NSIS_MENU_LINKS "mmapper.exe;MMapper")
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
        set(CPACK_NSIS_DISPLAY_NAME "MUME Mapper (x64)")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
            Push \$0
            mmapper_check:
              ExecWait 'cmd /C tasklist /FI \\\"IMAGENAME eq mmapper.exe\\\" | findstr /I \\\"mmapper.exe\\\" >nul' \$0
              StrCmp \$0 0 mmapper_found mmapper_done
            mmapper_found:
              MessageBox MB_OKCANCEL|MB_ICONQUESTION \\\"MMapper is running. Please close it to continue.\\\" IDOK mmapper_check
              Abort
            mmapper_done:
              Pop \$0
            ")
        if(WITH_DRMINGW)
            # Include shortcut to crash logs
            set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
                CreateShortCut \\\"$SMPROGRAMS\\\\${CPACK_NSIS_DISPLAY_NAME}\\\\Crash Log.lnk\\\" \\\"$PROFILE\\\\AppData\\\\Local\\\\mmappercrash.log\\\"
                ")
            # Delete shortcut and crash log
            set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
                Delete \\\"$SMPROGRAMS\\\\${CPACK_NSIS_DISPLAY_NAME}\\\\Crash Log.lnk\\\"
                Delete \\\"$PROFILE\\\\AppData\\\\Local\\\\mmappercrash.log\\\"
                ")
        endif()
        # NSIS always displays a license page
        set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING.txt")
    else()
        # Windows AppX Settings
        set(CPACK_GENERATOR "External")
        set(APPX_NAME "${PROJECT_NAME}")

        # Set AppX specific version variables and remove leading zeros
        string(REGEX REPLACE "^0*([0-9]+)$" "\\1" APPX_VERSION_MAJOR "${CPACK_PACKAGE_VERSION_MAJOR}")
        string(REGEX REPLACE "^0*([0-9]+)$" "\\1" APPX_VERSION_MINOR "${CPACK_PACKAGE_VERSION_MINOR}")
        string(REGEX REPLACE "^0*([0-9]+)$" "\\1" APPX_VERSION_PATCH "${CPACK_PACKAGE_VERSION_PATCH}")
        string(REGEX REPLACE "^0*([0-9]+)$" "\\1" APPX_VERSION_REVISION "${CPACK_PACKAGE_VERSION_TWEAK}")
        set(APPX_MANIFEST_VERSION "${APPX_VERSION_MAJOR}.${APPX_VERSION_MINOR}.${APPX_VERSION_PATCH}.0")
        set(APPX_VERSION "${APPX_VERSION_MAJOR}_${APPX_VERSION_MINOR}_${APPX_VERSION_PATCH}_${APPX_VERSION_REVISION}")

        # Install AppManifest and Assets
        configure_file(
            "${PROJECT_SOURCE_DIR}/cmake/AppxManifest.xml.in"
            "${CMAKE_BINARY_DIR}/AppxManifest.xml"
            @ONLY
        )
        install(
            FILES "${CMAKE_BINARY_DIR}/AppxManifest.xml"
            DESTINATION .
            COMPONENT applications
        )
        install(
            FILES "${CMAKE_SOURCE_DIR}/src/resources/icons/hi150-app-mmapper-release.png"
            DESTINATION "Assets"
            RENAME "Square150x150Logo.png"
            COMPONENT applications
        )
        install(
            FILES "${CMAKE_SOURCE_DIR}/src/resources/icons/hi44-app-mmapper-release.png"
            DESTINATION "Assets"
            RENAME "Square44x44Logo.png"
            COMPONENT applications
        )
        install(
            FILES "${CMAKE_SOURCE_DIR}/src/resources/icons/hi50-app-mmapper-release.png"
            DESTINATION "Assets"
            RENAME "StoreLogo.png"
            COMPONENT applications
        )

        # Set the AppX external script
        set(CPACK_EXTERNAL_ENABLE_STAGING TRUE)
        configure_file(
            "${PROJECT_SOURCE_DIR}/cmake/MakeAppX.cmake.in"
            "${CMAKE_BINARY_DIR}/MakeAppX.cmake"
            @ONLY
        )
        set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_BINARY_DIR}/MakeAppX.cmake")
    endif()
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
        COMMAND ${MACDEPLOYQT_APP} ${CMAKE_CURRENT_BINARY_DIR}/mmapper.app -libpath ${QTKEYCHAIN_LIBRARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Deploying the Qt Framework onto the bundle"
        VERBATIM
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
        VERBATIM
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
