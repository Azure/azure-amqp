#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(message_unittests, failedTestCount);
    return failedTestCount;
}
