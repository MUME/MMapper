include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

include_directories(SYSTEM ${GLM_INCLUDE_DIR})

# Clock
set(clock_SRCS
    ../src/clock/mumeclock.cpp
    ../src/clock/mumeclock.h
    ../src/clock/mumemoment.cpp
    ../src/clock/mumemoment.h
    ../src/global/TextUtils.cpp
    ../src/global/TextUtils.h
    ../src/observer/gameobserver.cpp
    ../src/observer/gameobserver.h
    ../src/global/parserutils.cpp
    ../src/proxy/GmcpMessage.cpp
    ../src/proxy/GmcpMessage.h
    )
set(TestClock_SRCS testclock.cpp)
add_executable(TestClock ${TestClock_SRCS} ${clock_SRCS})
add_dependencies(TestClock glm)
target_link_libraries(TestClock Qt5::Test coverage_config)
set_target_properties(
  TestClock PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestClock COMMAND TestClock)

# Expandora
file(GLOB_RECURSE expandoracommon_SRCS
    ../src/expandoracommon/*.cpp
    ../src/global/NullPointerException.cpp
    ../src/global/NullPointerException.h
    ../src/global/StringView.cpp
    ../src/global/StringView.h
    ../src/global/TextUtils.cpp
    ../src/global/TextUtils.h
    ../src/global/random.cpp
    ../src/global/random.h
    ../src/global/string_view_utils.cpp
    ../src/global/string_view_utils.h
    ../src/global/utils.cpp
    ../src/global/utils.h
    ../src/mapdata/ExitDirection.cpp
    ../src/mapdata/ExitDirection.h
    ../src/mapdata/ExitFieldVariant.cpp
    ../src/mapdata/ExitFieldVariant.h
    ../src/parser/CommandId.cpp
    ../src/parser/CommandId.h
    ../src/global/parserutils.cpp
    ../src/global/parserutils.h
    )
set(TestExpandoraCommon_SRCS testexpandoracommon.cpp)
add_executable(TestExpandoraCommon ${TestExpandoraCommon_SRCS} ${expandoracommon_SRCS})
add_dependencies(TestExpandoraCommon glm)
target_link_libraries(TestExpandoraCommon Qt5::Test coverage_config)
set_target_properties(
  TestExpandoraCommon PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestExpandoraCommon COMMAND TestExpandoraCommon)

# Parser
set(parser_SRCS
    ../src/expandoracommon/parseevent.cpp
    ../src/expandoracommon/parseevent.h
    ../src/expandoracommon/property.cpp
    ../src/expandoracommon/property.h
    ../src/global/NullPointerException.cpp
    ../src/global/NullPointerException.h
    ../src/global/TextUtils.cpp
    ../src/global/TextUtils.h
    ../src/global/random.cpp
    ../src/global/random.h
    ../src/global/string_view_utils.cpp
    ../src/global/string_view_utils.h
    ../src/mapdata/ExitDirection.cpp
    ../src/mapdata/ExitDirection.h
    ../src/parser/CommandId.cpp
    ../src/parser/CommandId.h
    ../src/global/parserutils.cpp
    ../src/global/parserutils.h
    )
set(TestParser_SRCS testparser.cpp)
add_executable(TestParser ${TestParser_SRCS} ${parser_SRCS})
add_dependencies(TestParser glm)
target_link_libraries(TestParser Qt5::Test coverage_config)
set_target_properties(
  TestParser PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestParser COMMAND TestParser)

# Proxy
set(proxy_SRCS
    ../src/proxy/GmcpMessage.cpp
    ../src/proxy/GmcpMessage.h
    ../src/proxy/GmcpModule.cpp
    ../src/proxy/GmcpModule.h
    ../src/proxy/GmcpUtils.cpp
    ../src/proxy/GmcpUtils.h
    ../src/global/TextUtils.cpp
    ../src/global/TextUtils.h
    )
set(TestProxy_SRCS TestProxy.cpp)
add_executable(TestProxy ${TestProxy_SRCS} ${proxy_SRCS})
add_dependencies(TestProxy glm)
target_link_libraries(TestProxy Qt5::Test coverage_config)
set_target_properties(
  TestProxy PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestProxy COMMAND TestProxy)

# MainWindow
set(mainwindow_SRCS
    ../src/global/Version.h
    ../src/mainwindow/UpdateDialog.cpp
    ../src/mainwindow/UpdateDialog.h
    )
set(TestMainWindow_SRCS TestMainWindow.cpp)
add_executable(TestMainWindow ${TestMainWindow_SRCS} ${mainwindow_SRCS})
add_dependencies(TestMainWindow glm)
target_link_libraries(TestMainWindow Qt5::Widgets Qt5::Network Qt5::Test coverage_config)
set_target_properties(
  TestMainWindow PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestMainWindow COMMAND TestMainWindow)

# Global
set(global_SRCS
    ../src/global/AnsiColor.h
    ../src/global/StringView.cpp
    ../src/global/StringView.h
    ../src/global/TextUtils.cpp
    ../src/global/TextUtils.h
    ../src/global/string_view_utils.cpp
    ../src/global/string_view_utils.h
    ../src/global/unquote.cpp
    ../src/global/unquote.h
    )
set(TestGlobal_SRCS TestGlobal.cpp)
add_executable(TestGlobal ${TestGlobal_SRCS} ${global_SRCS})
add_dependencies(TestGlobal glm)
target_link_libraries(TestGlobal Qt5::Widgets Qt5::Test coverage_config)
set_target_properties(
  TestGlobal PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestGlobal COMMAND TestGlobal)

# Adventure
set(adventure_SRCS
        ../src/adventure/adventuresession.cpp
        ../src/adventure/adventuresession.h
        ../src/adventure/adventuretracker.cpp
        ../src/adventure/adventuretracker.h
        ../src/adventure/lineparsers.cpp
        ../src/adventure/lineparsers.h
        ../src/configuration/configobserver.h
        ../src/configuration/configuration.cpp
        ../src/configuration/configuration.h
        ../src/global/Color.cpp
        ../src/global/Color.h
        ../src/global/NamedColors.cpp
        ../src/global/NamedColors.h
        ../src/global/utils.cpp
        ../src/global/utils.h
        ../src/observer/gameobserver.cpp
        ../src/observer/gameobserver.h
        ../src/global/parserutils.cpp
        ../src/global/parserutils.h
        )
set(TestAdventure_SRCS testadventure.cpp testadventure.h)
add_executable(TestAdventure ${TestAdventure_SRCS} ${adventure_SRCS})
add_dependencies(TestAdventure glm)
target_link_libraries(TestAdventure Qt5::Widgets Qt5::Network Qt5::Test coverage_config)
set_target_properties(
        TestAdventure PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestAdventure COMMAND TestAdventure)

# Timer
set(timer_test
        ../src/timers/CTimers.cpp
        ../src/timers/CTimers.h
)
set(TestTimer TestCTimers.cpp)
add_executable(TestTimer ${timer_test} ${TestTimer})
add_dependencies(TestTimer glm)
target_link_libraries(TestTimer Qt5::Test coverage_config)
set_target_properties(
        TestTimer PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestTimer COMMAND TestTimer)

# Room panel mob
set(room_mob
        ../src/roompanel/RoomMob.h
        ../src/roompanel/RoomMob.cpp
)
set(RoomMob roompanel/TestRoomMob.cpp)
add_executable(TestRoomMob ${room_mob} ${RoomMob})
add_dependencies(TestRoomMob glm)
target_link_libraries(TestRoomMob Qt5::Test Qt5::Gui Qt5::Widgets coverage_config)
set_target_properties(
        TestRoomMob PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestRoomMob COMMAND TestRoomMob)


# Room panel mobs
set(room_mobs
        ../src/roompanel/RoomMobs.h
        ../src/roompanel/RoomMobs.cpp
        ../src/roompanel/RoomMob.h
        ../src/roompanel/RoomMob.cpp
)
set(RoomMobs roompanel/TestRoomMobs.cpp)
add_executable(TestRoomMobs ${room_mobs} ${RoomMobs})
add_dependencies(TestRoomMobs glm)
target_link_libraries(TestRoomMobs Qt5::Test Qt5::Gui Qt5::Widgets coverage_config)
set_target_properties(
        TestRoomMobs PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestRoomMobs COMMAND TestRoomMobs)

# Room panel manager
set(room_manager
        ../src/roompanel/RoomMobs.h
        ../src/roompanel/RoomMobs.cpp
        ../src/roompanel/RoomMob.h
        ../src/roompanel/RoomMob.cpp
        ../src/roompanel/RoomManager.cpp
        ../src/roompanel/RoomManager.h
        ../src/global/TextUtils.cpp
        ../src/proxy/GmcpMessage.cpp
)
set(RoomManager roompanel/TestRoomManager.cpp)
add_executable(TestRoomManager ${room_manager} ${RoomManager})
add_dependencies(TestRoomManager glm)
target_link_libraries(TestRoomManager Qt5::Test Qt5::Gui Qt5::Widgets coverage_config)
set_target_properties(
        TestRoomManager PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_FLAGS "${WARNING_FLAGS}"
)
add_test(NAME TestRoomManager COMMAND TestRoomManager)
