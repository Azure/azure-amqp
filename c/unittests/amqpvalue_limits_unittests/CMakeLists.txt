add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(amqpvalue_limits_unittests
	amqpvalue_limits_unittests.cpp
	main.c
	
	#$<TARGET_OBJECTS:amqp_frame_codec>
	)
	
target_link_libraries(amqpvalue_limits_unittests micromock ctest)