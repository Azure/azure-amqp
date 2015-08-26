#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "session_monitor.h"

TYPED_MOCK_CLASS(session_monitor_mocks, CGlobalMock)
{
public:
	/* connection mocks */
};

extern "C"
{
	extern void consolelogger_log(char* format, ...)
	{
		(void)format;
	}
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(session_monitor_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
	test_serialize_mutex = MicroMockCreateMutex();
	ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
	MicroMockDestroyMutex(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
	if (!MicroMockAcquireMutex(test_serialize_mutex))
	{
		ASSERT_FAIL("Could not acquire test serialization mutex.");
	}
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
	if (!MicroMockReleaseMutex(test_serialize_mutex))
	{
		ASSERT_FAIL("Could not release test serialization mutex.");
	}
}

/* session_monitor_create */

END_TEST_SUITE(session_monitor_unittests)
