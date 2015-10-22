add_definitions(-DUSE_CTEST)
include_directories(../../testtools/testrunnerswitcher/inc)
include_directories(../../testtools/ctest/inc)
include_directories(../../testtools/micromock/inc)
add_executable(frame_codec_unittests
	frame_codec_unittests.cpp
	main.c
	
	$<TARGET_OBJECTS:frame_codec>)
	
target_link_libraries(frame_codec_unittests micromock ctest)