#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(CMockValue_tests, failedTestCount);
    RUN_TEST_SUITE(MicroMockCallComparisonUnitTests, failedTestCount);
    RUN_TEST_SUITE(MicroMockTest, failedTestCount);
    RUN_TEST_SUITE(MicroMockValidateArgumentBufferTests, failedTestCount);
    RUN_TEST_SUITE(NULLArgsStringificationTests, failedTestCount);
    return failedTestCount;
}
