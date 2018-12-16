include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../src/clock
	${CMAKE_CURRENT_SOURCE_DIR}/../src/expandoracommon
	${CMAKE_CURRENT_SOURCE_DIR}/../src/parser
	${CMAKE_CURRENT_SOURCE_DIR}/../src/global
)

# Clock
set(clock_SRCS
    ../src/clock/mumeclock.cpp
    ../src/clock/mumeclock.h
    ../src/clock/mumemoment.cpp
    ../src/clock/mumemoment.h
    )
set(TestClock_SRCS testclock.cpp)
add_executable(TestClock ${TestClock_SRCS} ${clock_SRCS})
target_link_libraries(TestClock Qt5::Test)
add_test(NAME TestClock COMMAND TestClock)

# Expandora
file(GLOB_RECURSE expandoracommon_SRCS
    ../src/expandoracommon/*.cpp
    ../src/parser/CommandId.cpp
    ../src/parser/CommandId.h
    ../src/mapdata/ExitDirection.cpp
    ../src/mapdata/ExitDirection.h
    ../src/global/NullPointerException.cpp
    ../src/global/NullPointerException.h
    ../src/global/random.cpp
    ../src/global/random.h
    ../src/global/utils.cpp
    ../src/global/utils.h
    )
set(TestExpandoraCommon_SRCS testexpandoracommon.cpp)
add_executable(TestExpandoraCommon ${TestExpandoraCommon_SRCS} ${expandoracommon_SRCS})
target_link_libraries(TestExpandoraCommon Qt5::Test)
add_test(NAME TestExpandoraCommon COMMAND TestExpandoraCommon)

# Parser
set(parser_SRCS
    ../src/parser/CommandId.cpp
    ../src/parser/CommandId.h
    ../src/parser/parserutils.cpp
    ../src/expandoracommon/parseevent.cpp
    ../src/expandoracommon/parseevent.h
    ../src/expandoracommon/property.cpp
    ../src/expandoracommon/property.h
    ../src/mapdata/ExitDirection.cpp
    ../src/mapdata/ExitDirection.h
    ../src/global/NullPointerException.cpp
    ../src/global/NullPointerException.h
    ../src/global/random.cpp
    ../src/global/random.h
    )
set(TestParser_SRCS testparser.cpp)
add_executable(TestParser ${TestParser_SRCS} ${parser_SRCS})
target_link_libraries(TestParser Qt5::Test)
add_test(NAME TestParser COMMAND TestParser)
