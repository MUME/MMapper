include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../src/clock
	${CMAKE_CURRENT_SOURCE_DIR}/../src/expandoracommon
	${CMAKE_CURRENT_SOURCE_DIR}/../src/parser
)

# Clock
set(clock_SRCS ../src/clock/mumeclock.cpp ../src/clock/mumemoment.cpp)
set(TestClock_SRCS testclock.cpp)
add_executable(TestClock ${TestClock_SRCS} ${clock_SRCS})
target_link_libraries(TestClock Qt5::Test)
add_test(NAME TestClock COMMAND TestClock)

# Expandora
file(GLOB_RECURSE expandoracommon_SRCS ../src/expandoracommon/*.cpp)
set(TestExpandoraCommon_SRCS testexpandoracommon.cpp)
add_executable(TestExpandoraCommon ${TestExpandoraCommon_SRCS} ${expandoracommon_SRCS})
target_link_libraries(TestExpandoraCommon Qt5::Test)
add_test(NAME TestExpandoraCommon COMMAND TestExpandoraCommon)

# Parser
set(parser_SRCS ../src/parser/parserutils.cpp)
set(TestParser_SRCS testparser.cpp)
add_executable(TestParser ${TestParser_SRCS} ${parser_SRCS})
target_link_libraries(TestParser Qt5::Test)
add_test(NAME TestParser COMMAND TestParser)
