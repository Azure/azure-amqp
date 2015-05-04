CC=g++
CCC=gcc
CFLAGS=-c -Wall -DUSE_CTEST -DDISABLE_DETOURS
LDFLAGS=
INCLUDE=-I../SAL -I../TestRunnerSwitcher -I../CTest -I./MicroMock

SOURCES_CPP=MicroMock/MockValueBase.cpp MicroMock/MicroMockCharStarArenullTerminatedStrings.cpp MicroMock/TimeDiscreteMicroMock.cpp MicroMock/MockCallRecorder.cpp MicroMock/DetoursHookClass.cpp MicroMock/MockMethodCallBase.cpp MicroMock/MicromockException.cpp 
OBJECTS_CPP=$(SOURCES_CPP:.cpp=.o)

SOURCES_TEST_CPP=MicroMockTest/CMockValueUnitTests.cpp MicroMockTest/MicroMockCallComparisonUnitTests.cpp MicroMockTest/MicroMockTest.cpp MicroMockTest/MicroMockValidateArgumentBufferUnitTests.cpp MicroMockTest/NULLArgsStringificationUnitTests.cpp MicroMockTest/TimeDiscreteMicroMockTestVoidVoid.cpp MicroMockTest/TimeDiscreteMicroMockTest_Stim.cpp MicroMockTest/TimeDiscreteMicroMockWithReturnAndParameters.cpp 
OBJECTS_TEST_CPP=$(SOURCES_TEST_CPP:.cpp=.o)

SOURCES_C=../CTest/CTest.c ./MicroMockTest/MicroMockTest_with_CTest.c
OBJECTS_C=$(SOURCES_C:.c=.o)
EXECUTABLE=micromock_unitTests

all: $(SOURCES_CPP) $(SOURCES_TEST_CPP) $(SOURCES_C) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS_CPP) $(OBJECTS_TEST_CPP) $(OBJECTS_C)
	$(CC) $(LDFLAGS) $(OBJECTS_CPP) $(OBJECTS_TEST_CPP) $(OBJECTS_C) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

.c.o:
	$(CCC) $(CFLAGS) $(INCLUDE) $< -o $@
	
	
#DEPS = MicroMock/DetoursHookClass.h MicroMock/StrictOrderedCallComparer.h MicroMock/MockCallComparer.h MicroMock/TimeDiscreteMicroMock.h MicroMock/MicroMockTestMutex.h MicroMock/RuntimeMock.h MicroMock/stdafx.h MicroMock/MockCallArgument.h MicroMock/MicroMock.h MicroMock/MockResultValue.h MicroMock/ThreadSafeGlobalMock.h MicroMock/MicroMockException.h MicroMock/MicroMockEnumToString.h MicroMock/NiceCallComparer.h MicroMock/Mock.h MicroMock/MicroMockCommon.h MicroMock/TimeDiscreteMicroMockCallMacros.h MicroMock/MockValueBase.h MicroMock/MicroMockTestRunnerHooks.h MicroMock/MockMethodCallBase.h MicroMock/StrictUnorderedCallComparer.h MicroMock/MicroMockCharStarArenullTerminatedStrings.h MicroMock/GlobalMock.h MicroMock/MockCallRecorder.h MicroMock/MicroMockCallMacros.h MicroMock/targetver.h MicroMock/MicroMock/MockMethodCall.h  MicroMock/MockValue.h  MicroMock/MockCallArgumentBase.h  
