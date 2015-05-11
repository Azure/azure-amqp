#include "TestRunnerSwitcher.h"
#include "MicroMock.h"
#include "list.h"

bool fail_alloc_calls;

TYPED_MOCK_CLASS(list_mocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_1(, void*, amqp_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqp_free, void*, ptr)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(list_mocks, , void*, amqp_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(list_mocks, , void, amqp_free, void*, ptr);
}

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
			fail_alloc_calls = false;
		}

		TEST_METHOD_CLEANUP(method_cleanup)
		{
			if (!MicroMockReleaseMutex(test_serialize_mutex))
			{
				ASSERT_FAIL("Could not release test serialization mutex.");
			}
		}

		/* list_create */

		/* Tests_SRS_LIST_01_001: [list_create shall create a new list and return a non-NULL handle on success.] */
		TEST_METHOD(when_underlying_calls_suceed_list_create_succeeds)
		{
			// arrange
			list_mocks mocks;

			EXPECTED_CALL(mocks, amqp_malloc(IGNORE));

			// act
			LIST_HANDLE result = list_create();

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_LIST_01_002: [If any error occurs during the list creation, list_create shall return NULL.] */
		TEST_METHOD(when_underlying_malloc_fails_list_create_fails)
		{
			// arrange
			list_mocks mocks;

			EXPECTED_CALL(mocks, amqp_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			LIST_HANDLE result = list_create();

			// assert
			ASSERT_IS_NULL(result);
		}

		/* list_destroy */

		/* Tests_SRS_LIST_01_003: [list_destroy shall free all resources associated with the list identified by the handle argument.] */
		TEST_METHOD(list_destroy_on_a_non_null_handle_frees_resources)
		{
			// arrange
			list_mocks mocks;
			LIST_HANDLE handle = list_create();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqp_free(IGNORED_PTR_ARG));

			// act
			list_destroy(handle);

			// assert
			// uMock checks the calls
		}

		/* Tests_SRS_LIST_01_004: [If the handle argument is NULL, no freeing of resources shall occur.] */
		TEST_METHOD(list_destroy_on_a_null_handle_frees_nothing)
		{
			// arrange
			list_mocks mocks;

			// act
			list_destroy(NULL);

			// assert
			// uMock checks the calls
		}
	};
}
