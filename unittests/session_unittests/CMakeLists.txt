add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
include_directories(..)
add_executable(session_unittests
	session_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:session>)
	
target_link_libraries(session_unittests micromock ctest)