#include "TestRunnerSwitcher.h"
int main(void)
{
	RUN_TEST_SUITE(CMockValue_tests);
	RUN_TEST_SUITE(MicroMockCallComparisonUnitTests);
	RUN_TEST_SUITE(MicroMockTest);
	RUN_TEST_SUITE(MicroMockValidateArgumentBufferTests);
	RUN_TEST_SUITE(NULLArgsStringificationTests);
	RUN_TEST_SUITE(MicroMockVoidVoidTest);
	RUN_TEST_SUITE(MicroMockStimTest);
	RUN_TEST_SUITE(MicroMockTestWithReturnAndParameters);
	return 0;
}

