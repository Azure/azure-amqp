#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(io_unittests, failedTestCount);
    return failedTestCount;
}
