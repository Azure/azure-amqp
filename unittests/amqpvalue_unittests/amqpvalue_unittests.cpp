#include "testrunnerswitcher.h"
#include "micromock.h"
#include "amqpvalue.h"

bool fail_alloc_calls;

TYPED_MOCK_CLASS(amqpvalue_mocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
		MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(amqpvalue_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqpvalue_mocks, , void, amqpalloc_free, void*, ptr);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

namespace amqpvalue_unittests
{
	TEST_CLASS(amqpvalue_unittests)
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
			fail_alloc_calls = false;
		}

		TEST_METHOD_CLEANUP(method_cleanup)
		{
			if (!MicroMockReleaseMutex(test_serialize_mutex))
			{
				ASSERT_FAIL("Could not release test serialization mutex.");
			}
		}

		/* amqpvalue_create_null */

		/* Tests_SRS_AMQPVALUE_01_001: [amqpvalue_create_null shall return a handle to an AMQP_VALUE that stores a null value.] */
		TEST_METHOD(amqpvalue_create_null_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_null();

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_002: [If allocating the AMQP_VALUE fails then amqpvalue_create_null shall return a non-zero value.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_null_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_null();

			// assert
			ASSERT_IS_NULL(result);
		}
	};
}
