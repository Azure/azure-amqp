add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(amqp_frame_codec_unittests
	amqp_frame_codec_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:amqp_frame_codec>)
	
target_link_libraries(amqp_frame_codec_unittests micromock ctest)