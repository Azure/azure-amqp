add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(amqpvalue_unittests
	amqpvalue_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:amqpvalue>)
	
target_link_libraries(amqpvalue_unittests micromock ctest)