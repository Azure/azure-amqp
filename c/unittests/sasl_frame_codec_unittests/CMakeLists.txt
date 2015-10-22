add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(amqp_frame_codec_unittests
	sasl_frame_codec_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:sasl_frame_codec>)
	
target_link_libraries(sasl_frame_codec_unittests micromock ctest)