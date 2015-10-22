add_definitions(-DUSE_CTEST)
include_directories(../../../testrunnerswitcher/inc)
add_executable(micromocktest
cmockvalueunittests.cpp
micromockcallcomparisonunittests.cpp
micromocktest.cpp
micromockvalidateargumentbufferunittests.cpp
nullargsstringificationunittests.cpp
stdafx.cpp
main.c
#timediscretemicromocktestvoidvoid.cpp
#timediscretemicromocktest_stim.cpp
#timediscretemicromockwithreturnandparameters.cpp

hypothetic_module.h
hypothetic_module_interfaces.h
resource.h
stdafx.h
stdafx_c.h
targetver.h
)

target_link_libraries(micromocktest micromock ctest)