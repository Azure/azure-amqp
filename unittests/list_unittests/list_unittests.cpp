#include "TestRunnerSwitcher.h"
#include "MicroMock.h"
#include "list.h"

TYPED_MOCK_CLASS(list_mocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_1(, void*, amqp_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
};

DECLARE_GLOBAL_MOCK_METHOD_1(list_mocks, , void*, amqp_malloc, size_t, size);

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

namespace amqpvalue_unittests
{
	TEST_CLASS(list_unittests)
	{
	public:
		TEST_CLASS_INITIALIZE(suite_init)
		{
			test_serialize_mutex = MicroMockCreateMutex();
			ASSERT_IS_NOT_NULL(test_serialize_mutex);
		}

		TEST_CLASS_CLEANUP(suite_cleanup)
		{
			MicroMockDestroyMutex(test_serialize_mutex);
		}

		TEST_METHOD_INITIALIZE(method_init)
		{
			if (!MicroMockAcquireMutex(test_serialize_mutex))
			{
				ASSERT_FAIL("Could not acquire test serialization mutex.");
			}
		}

		TEST_METHOD_CLEANUP(method_cleanup)
		{
			if (!MicroMockReleaseMutex(test_serialize_mutex))
			{
				ASSERT_FAIL("Could not acquire test serialization mutex.");
			}
		}

		TEST_METHOD(when_underlying_calls_suceed_list_create_succeeds)
		{
			// arrange
			list_mocks mocks;

			// act
			LIST_HANDLE result = list_create();

			// assert
			ASSERT_IS_NOT_NULL(result);
		}
	};
}
