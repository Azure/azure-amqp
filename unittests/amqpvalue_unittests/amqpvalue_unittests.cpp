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
		/* Tests_SRS_AMQPVALUE_01_003: [1.6.1 null Indicates an empty value.] */
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

		/* Tests_SRS_AMQPVALUE_01_002: [If allocating the AMQP_VALUE fails then amqpvalue_create_null shall return NULL.] */
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

		/* amqpvalue_create_boolean */

		/* Tests_SRS_AMQPVALUE_01_006: [amqpvalue_create_boolean shall return a handle to an AMQP_VALUE that stores a boolean value.] */
		/* Tests_SRS_AMQPVALUE_01_004: [1.6.2 boolean Represents a true or false value.] */
		TEST_METHOD(amqpvalue_create_boolean_true_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_boolean(true);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_006: [amqpvalue_create_boolean shall return a handle to an AMQP_VALUE that stores a boolean value.] */
		/* Tests_SRS_AMQPVALUE_01_004: [1.6.2 boolean Represents a true or false value.] */
		TEST_METHOD(amqpvalue_create_boolean_false_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_boolean(false);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_007: [If allocating the AMQP_VALUE fails then amqpvalue_create_boolean shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_boolean_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_boolean(true);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_boolean */

		/* Tests_SRS_AMQPVALUE_01_008: [amqpvalue_get_boolean shall fill in the bool_value argument the Boolean value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_010: [On success amqpvalue_get_boolean shall return 0.] */
		TEST_METHOD(amqpvalue_get_boolean_true_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			bool bool_value;
			AMQP_VALUE value = amqpvalue_create_boolean(true);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_boolean(value, &bool_value);

			// assert
			ASSERT_ARE_EQUAL(bool, true, bool_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_008: [amqpvalue_get_boolean shall fill in the bool_value argument the Boolean value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_010: [On success amqpvalue_get_boolean shall return 0.] */
		TEST_METHOD(amqpvalue_get_boolean_false_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			bool bool_value;
			AMQP_VALUE value = amqpvalue_create_boolean(false);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_boolean(value, &bool_value);

			// assert
			ASSERT_ARE_EQUAL(bool, false, bool_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_009: [If any of the arguments is NULL then amqpvalue_get_boolean shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_boolean_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			bool bool_value;

			// act
			int result = amqpvalue_get_boolean(NULL, &bool_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_009: [If any of the arguments is NULL then amqpvalue_get_boolean shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_boolean_with_a_NULL_bool_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_boolean(false);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_boolean(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_011: [If the type of the value is not Boolean, then amqpvalue_get_boolean shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_boolean_with_an_amqpvalue_that_is_not_boolean_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			bool bool_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_boolean(value, &bool_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_ubyte */

		/* Tests_SRS_AMQPVALUE_01_032: [amqpvalue_create_ubyte shall return a handle to an AMQP_VALUE that stores a unsigned char value.] */
		/* Tests_SRS_AMQPVALUE_01_005: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_ubyte_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_ubyte(0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_032: [amqpvalue_create_ubyte shall return a handle to an AMQP_VALUE that stores a unsigned char value.] */
		/* Tests_SRS_AMQPVALUE_01_005: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_ubyte_255_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_ubyte(255);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_033: [If allocating the AMQP_VALUE fails then amqpvalue_create_ubyte shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_ubyte_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_ubyte(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_ubyte */

		/* Tests_SRS_AMQPVALUE_01_034: [amqpvalue_get_ubyte shall fill in the ubyte_value argument the unsigned char value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_035: [On success amqpvalue_get_ubyte shall return 0.] */
		TEST_METHOD(amqpvalue_get_ubyte_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char ubyte_value;
			AMQP_VALUE value = amqpvalue_create_ubyte(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ubyte(value, &ubyte_value);

			// assert
			ASSERT_ARE_EQUAL(uint8_t, 0, ubyte_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_034: [amqpvalue_get_ubyte shall fill in the ubyte_value argument the unsigned char value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_035: [On success amqpvalue_get_ubyte shall return 0.] */
		TEST_METHOD(amqpvalue_get_ubyte_255_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char ubyte_value;
			AMQP_VALUE value = amqpvalue_create_ubyte(255);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ubyte(value, &ubyte_value);

			// assert
			ASSERT_ARE_EQUAL(uint8_t, 255, ubyte_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_036: [If any of the arguments is NULL then amqpvalue_get_ubyte shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ubyte_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char ubyte_value;

			// act
			int result = amqpvalue_get_ubyte(NULL, &ubyte_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_036: [If any of the arguments is NULL then amqpvalue_get_ubyte shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ubyte_with_a_NULL_ubyte_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ubyte(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ubyte(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_037: [If the type of the value is not ubyte (was not created with amqpvalue_create_ubyte), then amqpvalue_get_ubyte shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ubyte_with_an_amqpvalue_that_is_not_ubyte_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char ubyte_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ubyte(value, &ubyte_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}
	};
}
