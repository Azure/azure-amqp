add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(list_unittests
	list_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:list>)
	
target_link_libraries(list_unittests micromock ctest)