add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(io_unittests
	io_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:io>)
	
target_link_libraries(io_unittests micromock ctest)