add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
include_directories(..)
add_executable(connection_unittests
	connection_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:connection>)
	
target_link_libraries(connection_unittests micromock ctest)