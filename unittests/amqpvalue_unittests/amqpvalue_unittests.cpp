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

		/* amqpvalue_create_ushort */

		/* Tests_SRS_AMQPVALUE_01_038: [amqpvalue_create_ushort shall return a handle to an AMQP_VALUE that stores an uint16_t value.] */
		/* Tests_SRS_AMQPVALUE_01_012: [1.6.4 ushort Integer in the range 0 to 216 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_ushort_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_ushort(0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_038: [amqpvalue_create_ushort shall return a handle to an AMQP_VALUE that stores an uint16_t value.] */
		/* Tests_SRS_AMQPVALUE_01_012: [1.6.4 ushort Integer in the range 0 to 216 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_ushort_65535_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_ushort(65535);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_039: [If allocating the AMQP_VALUE fails then amqpvalue_create_ushort shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_ushort_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_ushort(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_ushort */

		/* Tests_SRS_AMQPVALUE_01_040: [amqpvalue_get_ushort shall fill in the ushort_value argument the uint16_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_041: [On success amqpvalue_get_ushort shall return 0.] */
		TEST_METHOD(amqpvalue_get_ushort_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint16_t ushort_value;
			AMQP_VALUE value = amqpvalue_create_ushort(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ushort(value, &ushort_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, 0, ushort_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_040: [amqpvalue_get_ushort shall fill in the ushort_value argument the uint16_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_041: [On success amqpvalue_get_ushort shall return 0.] */
		TEST_METHOD(amqpvalue_get_ushort_65535_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint16_t ushort_value;
			AMQP_VALUE value = amqpvalue_create_ushort(65535);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ushort(value, &ushort_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, (uint32_t)65535, ushort_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_042: [If any of the arguments is NULL then amqpvalue_get_ushort shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ushort_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint16_t ushort_value;

			// act
			int result = amqpvalue_get_ushort(NULL, &ushort_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_042: [If any of the arguments is NULL then amqpvalue_get_ushort shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ushort_with_a_NULL_ushort_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ushort(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ushort(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_043: [If the type of the value is not ushort (was not created with amqpvalue_create_ushort), then amqpvalue_get_ushort shall return a non-zero value.]  */
		TEST_METHOD(amqpvalue_get_ushort_with_an_amqpvalue_that_is_not_ushort_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint16_t ushort_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ushort(value, &ushort_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_uint */

		/* Tests_SRS_AMQPVALUE_01_044: [amqpvalue_create_uint shall return a handle to an AMQP_VALUE that stores an uint32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_013: [1.6.5 uint Integer in the range 0 to 232 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_uint_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_uint(0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_044: [amqpvalue_create_uint shall return a handle to an AMQP_VALUE that stores an uint32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_013: [1.6.5 uint Integer in the range 0 to 232 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_uint_0xFFFFFFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_uint(0xFFFFFFFF);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_045: [If allocating the AMQP_VALUE fails then amqpvalue_create_uint shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_uint_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_uint(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_uint */

		/* Tests_SRS_AMQPVALUE_01_046: [amqpvalue_get_uint shall fill in the uint_value argument the uint32_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_047: [On success amqpvalue_get_uint shall return 0.] */
		TEST_METHOD(amqpvalue_get_uint_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t uint_value;
			AMQP_VALUE value = amqpvalue_create_uint(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uint(value, &uint_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, 0, uint_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_046: [amqpvalue_get_uint shall fill in the uint_value argument the uint32_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_047: [On success amqpvalue_get_uint shall return 0.] */
		TEST_METHOD(amqpvalue_get_uint_0xFFFFFFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t uint_value;
			AMQP_VALUE value = amqpvalue_create_uint(0xFFFFFFFF);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uint(value, &uint_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0xFFFFFFFF, uint_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_079: [If any of the arguments is NULL then amqpvalue_get_uint shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_uint_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t uint_value;

			// act
			int result = amqpvalue_get_uint(NULL, &uint_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_079: [If any of the arguments is NULL then amqpvalue_get_uint shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_uint_with_a_NULL_uint_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_uint(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uint(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_048: [If the type of the value is not uint (was not created with amqpvalue_create_uint), then amqpvalue_get_uint shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_uint_with_an_amqpvalue_that_is_not_uint_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t uint_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uint(value, &uint_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_ulong */

		/* Tests_SRS_AMQPVALUE_01_049: [amqpvalue_create_ulong shall return a handle to an AMQP_VALUE that stores an uint64_t value.] */
		/* Tests_SRS_AMQPVALUE_01_014: [1.6.6 ulong Integer in the range 0 to 264 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_ulong_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_ulong(0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_049: [amqpvalue_create_ulong shall return a handle to an AMQP_VALUE that stores an uint64_t value.] */
		/* Tests_SRS_AMQPVALUE_01_014: [1.6.6 ulong Integer in the range 0 to 264 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_ulong_0xFFFFFFFFFFFFFFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_ulong(0xFFFFFFFFFFFFFFFF);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_050: [If allocating the AMQP_VALUE fails then amqpvalue_create_ulong shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_ulong_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_ulong(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_ulong */

		/* Tests_SRS_AMQPVALUE_01_051: [amqpvalue_get_ulong shall fill in the ulong_value argument the ulong64_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_052: [On success amqpvalue_get_ulong shall return 0.] */
		TEST_METHOD(amqpvalue_get_ulong_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t ulong_value;
			AMQP_VALUE value = amqpvalue_create_ulong(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ulong(value, &ulong_value);

			// assert
			ASSERT_ARE_EQUAL(uint64_t, 0, ulong_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_051: [amqpvalue_get_ulong shall fill in the ulong_value argument the ulong64_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_052: [On success amqpvalue_get_ulong shall return 0.] */
		TEST_METHOD(amqpvalue_get_ulong_0xFFFFFFFFFFFFFFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t ulong_value;
			AMQP_VALUE value = amqpvalue_create_ulong(0xFFFFFFFFFFFFFFFF);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ulong(value, &ulong_value);

			// assert
			ASSERT_ARE_EQUAL(uint64_t, (uint64_t)0xFFFFFFFFFFFFFFFF, ulong_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_053: [If any of the arguments is NULL then amqpvalue_get_ulong shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ulong_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t ulong_value;

			// act
			int result = amqpvalue_get_ulong(NULL, &ulong_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_053: [If any of the arguments is NULL then amqpvalue_get_ulong shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ulong_with_a_NULL_ulong_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ulong(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ulong(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_054: [If the type of the value is not ulong (was not created with amqpvalue_create_ulong), then amqpvalue_get_ulong shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_ulong_with_an_amqpvalue_that_is_not_ulong_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t ulong_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_ulong(value, &ulong_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_byte */

		/* Tests_SRS_AMQPVALUE_01_055: [amqpvalue_create_byte shall return a handle to an AMQP_VALUE that stores a char value.] */
		/* Tests_SRS_AMQPVALUE_01_015: [1.6.7 byte Integer in the range -(27) to 27 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_byte_minus_128_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_byte(-128);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_055: [amqpvalue_create_byte shall return a handle to an AMQP_VALUE that stores a char value.] */
		/* Tests_SRS_AMQPVALUE_01_015: [1.6.7 byte Integer in the range -(27) to 27 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_byte_127_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_byte(127);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_056: [If allocating the AMQP_VALUE fails then amqpvalue_create_byte shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_byte_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_byte(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_byte */

		/* Tests_SRS_AMQPVALUE_01_057: [amqpvalue_get_byte shall fill in the byte_value argument the char value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_058: [On success amqpvalue_get_byte shall return 0.] */
		TEST_METHOD(amqpvalue_get_byte_minus_127_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			char byte_value;
			AMQP_VALUE value = amqpvalue_create_byte(-128);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_byte(value, &byte_value);

			// assert
			ASSERT_ARE_EQUAL(char, -128, byte_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_057: [amqpvalue_get_byte shall fill in the byte_value argument the char value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_058: [On success amqpvalue_get_byte shall return 0.] */
		TEST_METHOD(amqpvalue_get_byte_127_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			char byte_value;
			AMQP_VALUE value = amqpvalue_create_byte(127);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_byte(value, &byte_value);

			// assert
			ASSERT_ARE_EQUAL(char, 127, byte_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_059: [If any of the arguments is NULL then amqpvalue_get_byte shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_byte_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			char byte_value;

			// act
			int result = amqpvalue_get_byte(NULL, &byte_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_059: [If any of the arguments is NULL then amqpvalue_get_byte shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_byte_with_a_NULL_byte_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_byte(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_byte(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_060: [If the type of the value is not byte (was not created with amqpvalue_create_byte), then amqpvalue_get_byte shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_byte_with_an_amqpvalue_that_is_not_byte_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			char byte_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_byte(value, &byte_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_short */

		/* Tests_SRS_AMQPVALUE_01_055: [amqpvalue_create_short shall return a handle to an AMQP_VALUE that stores a char value.] */
		/* Tests_SRS_AMQPVALUE_01_016: [1.6.8 short Integer in the range -(215) to 215 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_short_minus_32768_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_short(-32768);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_055: [amqpvalue_create_short shall return a handle to an AMQP_VALUE that stores a char value.] */
		/* Tests_SRS_AMQPVALUE_01_016: [1.6.8 short Integer in the range -(215) to 215 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_short_32767_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_short(32767);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_062: [If allocating the AMQP_VALUE fails then amqpvalue_create_short shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_short_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_short(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_short */

		/* Tests_SRS_AMQPVALUE_01_063: [amqpvalue_get_short shall fill in the short_value argument the int16_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_064: [On success amqpvalue_get_short shall return 0.] */
		TEST_METHOD(amqpvalue_get_short_minus_32768_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			int16_t short_value;
			AMQP_VALUE value = amqpvalue_create_short(-32768);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_short(value, &short_value);

			// assert
			ASSERT_ARE_EQUAL(int16_t, -32768, short_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_063: [amqpvalue_get_short shall fill in the short_value argument the int16_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_064: [On success amqpvalue_get_short shall return 0.] */
		TEST_METHOD(amqpvalue_get_short_32767_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			int16_t short_value;
			AMQP_VALUE value = amqpvalue_create_short(32767);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_short(value, &short_value);

			// assert
			ASSERT_ARE_EQUAL(int16_t, 32767, short_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_065: [If any of the arguments is NULL then amqpvalue_get_short shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_short_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			int16_t short_value;

			// act
			int result = amqpvalue_get_short(NULL, &short_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_065: [If any of the arguments is NULL then amqpvalue_get_short shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_short_with_a_NULL_short_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_short(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_short(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_066: [If the type of the value is not short (was not created with amqpvalue_create_short), then amqpvalue_get_short shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_short_with_an_amqpvalue_that_is_not_short_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			int16_t short_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_short(value, &short_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_int */

		/* Tests_SRS_AMQPVALUE_01_067: [amqpvalue_create_int shall return a handle to an AMQP_VALUE that stores an int32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_017: [1.6.9 int Integer in the range -(231) to 231 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_int_minus_2147483648_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_int(-2147483647-1);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_067: [amqpvalue_create_int shall return a handle to an AMQP_VALUE that stores an int32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_017: [1.6.9 int Integer in the range -(231) to 231 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_int_2147483647_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_int(2147483647);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_068: [If allocating the AMQP_VALUE fails then amqpvalue_create_int shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_int_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_int(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_int */

		/* Tests_SRS_AMQPVALUE_01_069: [amqpvalue_get_int shall fill in the int_value argument the int32_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_070: [On success amqpvalue_get_int shall return 0.] */
		TEST_METHOD(amqpvalue_get_int_minus_2147483648_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			int32_t int_value;
			AMQP_VALUE value = amqpvalue_create_int(-2147483647-1);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_int(value, &int_value);

			// assert
			ASSERT_ARE_EQUAL(int32_t, -2147483647-1, int_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_069: [amqpvalue_get_int shall fill in the int_value argument the int32_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_070: [On success amqpvalue_get_int shall return 0.] */
		TEST_METHOD(amqpvalue_get_int_2147483647_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			int32_t int_value;
			AMQP_VALUE value = amqpvalue_create_int(2147483647);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_int(value, &int_value);

			// assert
			ASSERT_ARE_EQUAL(int32_t, 2147483647, int_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_071: [If any of the arguments is NULL then amqpvalue_get_int shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_int_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			int32_t int_value;

			// act
			int result = amqpvalue_get_int(NULL, &int_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_071: [If any of the arguments is NULL then amqpvalue_get_int shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_int_with_a_NULL_int_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_int(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_int(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_072: [If the type of the value is not int (was not created with amqpvalue_create_int), then amqpvalue_get_int shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_int_with_an_amqpvalue_that_is_not_int_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			int32_t int_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_int(value, &int_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_long */

		/* Tests_SRS_AMQPVALUE_01_073: [amqpvalue_create_long shall return a handle to an AMQP_VALUE that stores an int64_t value.] */
		/* Tests_SRS_AMQPVALUE_01_018: [1.6.10 long Integer in the range -(263) to 263 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_long_minus_9223372036854775808_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_long(-9223372036854775807i64-1);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_073: [amqpvalue_create_long shall return a handle to an AMQP_VALUE that stores an int64_t value.] */
		/* Tests_SRS_AMQPVALUE_01_018: [1.6.10 long Integer in the range -(263) to 263 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_long_9223372036854775807_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_long(9223372036854775807);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_074: [If allocating the AMQP_VALUE fails then amqpvalue_create_long shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_long_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_long(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_long */

		/* Tests_SRS_AMQPVALUE_01_075: [amqpvalue_get_long shall fill in the long_value argument the int64_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_076: [On success amqpvalue_get_long shall return 0.] */
		TEST_METHOD(amqpvalue_get_long_minus_9223372036854775808_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			int64_t long_value;
			AMQP_VALUE value = amqpvalue_create_long(-9223372036854775807i64-1);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_long(value, &long_value);

			// assert
			ASSERT_ARE_EQUAL(uint64_t, (uint64_t)(-9223372036854775807i64 - 1), (uint64_t)long_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_075: [amqpvalue_get_long shall fill in the long_value argument the int64_t value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_076: [On success amqpvalue_get_long shall return 0.] */
		TEST_METHOD(amqpvalue_get_long_9223372036854775807_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			int64_t long_value;
			AMQP_VALUE value = amqpvalue_create_long(9223372036854775807);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_long(value, &long_value);

			// assert
			ASSERT_ARE_EQUAL(uint64_t, (uint64_t)9223372036854775807, (uint64_t)long_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_077: [If any of the arguments is NULL then amqpvalue_get_long shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_long_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			int64_t long_value;

			// act
			int result = amqpvalue_get_long(NULL, &long_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_077: [If any of the arguments is NULL then amqpvalue_get_long shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_long_with_a_NULL_long_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_long(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_long(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_078: [If the type of the value is not long (was not created with amqpvalue_create_long), then amqpvalue_get_long shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_long_with_an_amqpvalue_that_is_not_long_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			int64_t long_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_long(value, &long_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_float */

		/* Tests_SRS_AMQPVALUE_01_080: [amqpvalue_create_float shall return a handle to an AMQP_VALUE that stores a float value.] */
		/* Tests_SRS_AMQPVALUE_01_019: [1.6.11 float 32-bit floating point number (IEEE 754-2008 binary32).]  */
		TEST_METHOD(amqpvalue_create_float_minus_one_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_float(-1.0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_080: [amqpvalue_create_float shall return a handle to an AMQP_VALUE that stores a float value.] */
		/* Tests_SRS_AMQPVALUE_01_019: [1.6.11 float 32-bit floating point number (IEEE 754-2008 binary32).]  */
		TEST_METHOD(amqpvalue_create_float_42_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_float(42.0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_081: [If allocating the AMQP_VALUE fails then amqpvalue_create_float shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_float_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_float(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_float */

		/* Tests_SRS_AMQPVALUE_01_082: [amqpvalue_get_float shall fill in the float_value argument the float value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_083: [On success amqpvalue_get_float shall return 0.] */
		TEST_METHOD(amqpvalue_get_float_minus_one_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			float float_value;
			AMQP_VALUE value = amqpvalue_create_float(-1.0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_float(value, &float_value);

			// assert
			ASSERT_ARE_EQUAL(float, -1.0f, float_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_082: [amqpvalue_get_float shall fill in the float_value argument the float value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_083: [On success amqpvalue_get_float shall return 0.] */
		TEST_METHOD(amqpvalue_get_float_42_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			float float_value;
			AMQP_VALUE value = amqpvalue_create_float(42.0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_float(value, &float_value);

			// assert
			ASSERT_ARE_EQUAL(float, 42.0, float_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_084: [If any of the arguments is NULL then amqpvalue_get_float shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_float_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			float float_value;

			// act
			int result = amqpvalue_get_float(NULL, &float_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_084: [If any of the arguments is NULL then amqpvalue_get_float shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_float_with_a_NULL_float_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_float(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_float(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_085: [If the type of the value is not float (was not created with amqpvalue_create_float), then amqpvalue_get_float shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_float_with_an_amqpvalue_that_is_not_float_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			float float_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_float(value, &float_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_double */

		/* Tests_SRS_AMQPVALUE_01_086: [amqpvalue_create_double shall return a handle to an AMQP_VALUE that stores a double value.] */
		/* Tests_SRS_AMQPVALUE_01_020: [1.6.12 double 64-bit floating point number (IEEE 754-2008 binary64).] */
		TEST_METHOD(amqpvalue_create_double_minus_one_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_double(-1.0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_086: [amqpvalue_create_double shall return a handle to an AMQP_VALUE that stores a double value.] */
		/* Tests_SRS_AMQPVALUE_01_020: [1.6.12 double 64-bit floating point number (IEEE 754-2008 binary64).] */
		TEST_METHOD(amqpvalue_create_double_42_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

			// act
			AMQP_VALUE result = amqpvalue_create_double(42.0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_087: [If allocating the AMQP_VALUE fails then amqpvalue_create_double shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_double_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_double(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_double */

		/* Tests_SRS_AMQPVALUE_01_088: [amqpvalue_get_double shall fill in the double_value argument the double value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_089: [On success amqpvalue_get_double shall return 0.] */
		TEST_METHOD(amqpvalue_get_double_minus_one_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			double double_value;
			AMQP_VALUE value = amqpvalue_create_double(-1.0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_double(value, &double_value);

			// assert
			ASSERT_ARE_EQUAL(double, -1.0f, double_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_088: [amqpvalue_get_double shall fill in the double_value argument the double value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_089: [On success amqpvalue_get_double shall return 0.] */
		TEST_METHOD(amqpvalue_get_double_42_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			double double_value;
			AMQP_VALUE value = amqpvalue_create_double(42.0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_double(value, &double_value);

			// assert
			ASSERT_ARE_EQUAL(double, 42.0, double_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_090: [If any of the arguments is NULL then amqpvalue_get_double shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_double_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			double double_value;

			// act
			int result = amqpvalue_get_double(NULL, &double_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_090: [If any of the arguments is NULL then amqpvalue_get_double shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_double_with_a_NULL_double_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_double(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_double(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_091: [If the type of the value is not double (was not created with amqpvalue_create_double), then amqpvalue_get_double shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_double_with_an_amqpvalue_that_is_not_double_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			double double_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_double(value, &double_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}
	};
}
