add_library(ctest 
	src/ctest.c
	
	inc/ctest.h
	inc/ctest_macros.h
	)

include_directories(inc)

add_subdirectory(unittests)