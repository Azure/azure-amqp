#include "testrunnerswitcher.h"
#include "micromock.h"
#include "amqpvalue.h"

bool fail_alloc_calls;

TYPED_MOCK_CLASS(amqpvalue_mocks, CGlobalMock)
{
public:
	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_2(, void*, amqpalloc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, realloc(ptr, size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(amqpvalue_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqpvalue_mocks, , void*, amqpalloc_realloc, void*, ptr, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqpvalue_mocks, , void, amqpalloc_free, void*, ptr);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(connection_unittests)

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_int(-2147483647 - 1);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_067: [amqpvalue_create_int shall return a handle to an AMQP_VALUE that stores an int32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_017: [1.6.9 int Integer in the range -(231) to 231 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_int_2147483647_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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
			AMQP_VALUE value = amqpvalue_create_int(-2147483647 - 1);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_int(value, &int_value);

			// assert
			ASSERT_ARE_EQUAL(int32_t, -2147483647 - 1, int_value);
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_long(-9223372036854775807i64 - 1);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_073: [amqpvalue_create_long shall return a handle to an AMQP_VALUE that stores an int64_t value.] */
		/* Tests_SRS_AMQPVALUE_01_018: [1.6.10 long Integer in the range -(263) to 263 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_long_9223372036854775807_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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
			AMQP_VALUE value = amqpvalue_create_long(-9223372036854775807i64 - 1);
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

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

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
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

		/* amqpvalue_create_char */

		/* Tests_SRS_AMQPVALUE_01_092: [amqpvalue_create_char shall return a handle to an AMQP_VALUE that stores a single UTF-32 character value.] */
		/* Tests_SRS_AMQPVALUE_01_024: [1.6.16 char A single Unicode character.] */
		TEST_METHOD(amqpvalue_create_char_0x00_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_char(0x00);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_092: [amqpvalue_create_char shall return a handle to an AMQP_VALUE that stores a single UTF-32 character value.] */
		/* Tests_SRS_AMQPVALUE_01_024: [1.6.16 char A single Unicode character.] */
		TEST_METHOD(amqpvalue_create_char_0x10FFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_char(0x10FFFF);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_098: [If the code point value is outside of the allowed range [0, 0x10FFFF] then amqpvalue_create_char shall return NULL.] */
		TEST_METHOD(amqpvalue_create_char_0x110000_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			AMQP_VALUE result = amqpvalue_create_char(0x110000);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_093: [If allocating the AMQP_VALUE fails then amqpvalue_create_char shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_char_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_char(0x0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_char */

		/* Tests_SRS_AMQPVALUE_01_094: [amqpvalue_get_char shall fill in the char_value argument the UTF32 char value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_095: [On success amqpvalue_get_char shall return 0.] */
		TEST_METHOD(amqpvalue_get_char_0x0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t char_value;
			AMQP_VALUE value = amqpvalue_create_char(0x0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_char(value, &char_value);

			// assert
			ASSERT_ARE_EQUAL(char, 0x0, char_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_094: [amqpvalue_get_char shall fill in the char_value argument the UTF32 char value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_095: [On success amqpvalue_get_char shall return 0.] */
		TEST_METHOD(amqpvalue_get_char_0x10FFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t char_value;
			AMQP_VALUE value = amqpvalue_create_char(0x10FFFF);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_char(value, &char_value);

			// assert
			ASSERT_ARE_EQUAL(char, 0x10FFFF, char_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_096: [If any of the arguments is NULL then amqpvalue_get_char shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_char_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t char_value;

			// act
			int result = amqpvalue_get_char(NULL, &char_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_096: [If any of the arguments is NULL then amqpvalue_get_char shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_char_with_a_NULL_char_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_char(0x0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_char(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_097: [If the type of the value is not char (was not created with amqpvalue_create_char), then amqpvalue_get_char shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_char_with_an_amqpvalue_that_is_not_char_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t char_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_char(value, &char_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_timestamp */

		/* Tests_SRS_AMQPVALUE_01_107: [amqpvalue_create_timestamp shall return a handle to an AMQP_VALUE that stores an uint64_t value that represents a millisecond precision Unix time.] */
		/* Tests_SRS_AMQPVALUE_01_025: [1.6.17 timestamp An absolute point in time.] */
		TEST_METHOD(amqpvalue_create_timestamp_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_timestamp(0);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_049: [amqpvalue_create_timestamp shall return a handle to an AMQP_VALUE that stores an uint64_t value.] */
		/* Tests_SRS_AMQPVALUE_01_014: [1.6.6 timestamp Integer in the range 0 to 264 - 1 inclusive.] */
		TEST_METHOD(amqpvalue_create_timestamp_1311704463521_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_timestamp(1311704463521);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_050: [If allocating the AMQP_VALUE fails then amqpvalue_create_timestamp shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_timestamp_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_timestamp(0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_timestamp */

		/* Tests_SRS_AMQPVALUE_01_109: [amqpvalue_get_timestamp shall fill in the timestamp_value argument the timestamp value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_110: [On success amqpvalue_get_timestamp shall return 0.] */
		TEST_METHOD(amqpvalue_get_timestamp_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t timestamp_value;
			AMQP_VALUE value = amqpvalue_create_timestamp(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_timestamp(value, &timestamp_value);

			// assert
			ASSERT_ARE_EQUAL(uint64_t, 0, timestamp_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_109: [amqpvalue_get_timestamp shall fill in the timestamp_value argument the timestamp value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_110: [On success amqpvalue_get_timestamp shall return 0.] */
		TEST_METHOD(amqpvalue_get_timestamp_1311704463521_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t timestamp_value;
			AMQP_VALUE value = amqpvalue_create_timestamp(1311704463521);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_timestamp(value, &timestamp_value);

			// assert
			ASSERT_ARE_EQUAL(uint64_t, (uint64_t)1311704463521, timestamp_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_111: [If any of the arguments is NULL then amqpvalue_get_timestamp shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_timestamp_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t timestamp_value;

			// act
			int result = amqpvalue_get_timestamp(NULL, &timestamp_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_111: [If any of the arguments is NULL then amqpvalue_get_timestamp shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_timestamp_with_a_NULL_timestamp_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_timestamp(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_timestamp(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_112: [If the type of the value is not timestamp (was not created with amqpvalue_create_timestamp), then amqpvalue_get_timestamp shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_timestamp_with_an_amqpvalue_that_is_not_timestamp_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint64_t timestamp_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_timestamp(value, &timestamp_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_uuid */

		/* Tests_SRS_AMQPVALUE_01_113: [amqpvalue_create_uuid shall return a handle to an AMQP_VALUE that stores an amqp_uuid value that represents a unique identifier per RFC-4122 section 4.1.2.] */
		/* Tests_SRS_AMQPVALUE_01_026: [1.6.18 uuid A universally unique identifier as defined by RFC-4122 section 4.1.2 .] */
		TEST_METHOD(amqpvalue_create_uuid_all_zeroes_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0x0 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_uuid(uuid);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_113: [amqpvalue_create_uuid shall return a handle to an AMQP_VALUE that stores an amqp_uuid value that represents a unique identifier per RFC-4122 section 4.1.2.] */
		/* Tests_SRS_AMQPVALUE_01_026: [1.6.18 uuid A universally unique identifier as defined by RFC-4122 section 4.1.2 .] */
		TEST_METHOD(amqpvalue_create_uuid_all_0xFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_uuid(uuid);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_114: [If allocating the AMQP_VALUE fails then amqpvalue_create_uuid shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_uuid_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0x0 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_uuid(uuid);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_uuid */

		/* Tests_SRS_AMQPVALUE_01_115: [amqpvalue_get_uuid shall fill in the uuid_value argument the uuid value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_116: [On success amqpvalue_get_uuid shall return 0.] */
		TEST_METHOD(amqpvalue_get_uuid_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0x0 };
			amqp_uuid uuid_value;
			AMQP_VALUE value = amqpvalue_create_uuid(uuid);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uuid(value, &uuid_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, memcmp(&uuid, &uuid_value, sizeof(uuid)));
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_115: [amqpvalue_get_uuid shall fill in the uuid_value argument the uuid value stored by the AMQP value indicated by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_116: [On success amqpvalue_get_uuid shall return 0.] */
		TEST_METHOD(amqpvalue_get_uuid_1311704463521_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
			amqp_uuid uuid_value;
			AMQP_VALUE value = amqpvalue_create_uuid(uuid);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uuid(value, &uuid_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, memcmp(&uuid, &uuid_value, sizeof(uuid)));
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_117: [If any of the arguments is NULL then amqpvalue_get_uuid shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_uuid_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid_value;

			// act
			int result = amqpvalue_get_uuid(NULL, &uuid_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_117: [If any of the arguments is NULL then amqpvalue_get_uuid shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_uuid_with_a_NULL_uuid_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0x0 };
			AMQP_VALUE value = amqpvalue_create_uuid(uuid);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uuid(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_118: [If the type of the value is not uuid (was not created with amqpvalue_create_uuid), then amqpvalue_get_uuid shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_uuid_with_an_amqpvalue_that_is_not_uuid_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_uuid(value, &uuid_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_binary */

		/* Tests_SRS_AMQPVALUE_01_127: [amqpvalue_create_binary shall return a handle to an AMQP_VALUE that stores a sequence of bytes.] */
		/* Tests_SRS_AMQPVALUE_01_027: [1.6.19 binary A sequence of octets.] */
		TEST_METHOD(amqpvalue_create_binary_with_1_byte_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(1));

			// act
			amqp_binary binary_input = { input, sizeof(input) };
			AMQP_VALUE result = amqpvalue_create_binary(binary_input);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_127: [amqpvalue_create_binary shall return a handle to an AMQP_VALUE that stores a sequence of bytes.] */
		/* Tests_SRS_AMQPVALUE_01_027: [1.6.19 binary A sequence of octets.] */
		TEST_METHOD(amqpvalue_create_binary_with_0_bytes_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			amqp_binary binary_input = { NULL, 0 };
			AMQP_VALUE result = amqpvalue_create_binary(binary_input);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_127: [amqpvalue_create_binary shall return a handle to an AMQP_VALUE that stores a sequence of bytes.] */
		/* Tests_SRS_AMQPVALUE_01_027: [1.6.19 binary A sequence of octets.] */
		TEST_METHOD(amqpvalue_create_binary_with_2_bytes_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0, 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));

			// act
			amqp_binary binary_input = { input, sizeof(input) };
			AMQP_VALUE result = amqpvalue_create_binary(binary_input);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_128: [If allocating the AMQP_VALUE fails then amqpvalue_create_binary shall return NULL.] */
		TEST_METHOD(when_allocating_the_amqp_value_fails_then_amqpvalue_create_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0, 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			amqp_binary binary_input = { input, sizeof(input) };
			AMQP_VALUE result = amqpvalue_create_binary(binary_input);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_128: [If allocating the AMQP_VALUE fails then amqpvalue_create_binary shall return NULL.] */
		TEST_METHOD(when_allocating_the_binary_buffer_fails_then_amqpvalue_create_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0, 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqp_binary binary_input = { input, sizeof(input) };
			AMQP_VALUE result = amqpvalue_create_binary(binary_input);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_129: [If value.data is NULL and value.length is positive then amqpvalue_create_binary shall return NULL.] */
		TEST_METHOD(when_length_is_positive_and_buffer_is_NULL_then_amqpvalue_create_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			amqp_binary binary_input = { NULL, 1 };
			AMQP_VALUE result = amqpvalue_create_binary(binary_input);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_binary */

		/* Tests_SRS_AMQPVALUE_01_131: [amqpvalue_get_binary shall yield a pointer to the sequence of bytes held by the AMQP_VALUE in binary_value.data and fill in the binary_value.length argument the number of bytes held in the binary value.] */
		TEST_METHOD(amqpvalue_get_binary_1_byte_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x42 };
			amqp_binary binary_input = { input, sizeof(input) };
			AMQP_VALUE value = amqpvalue_create_binary(binary_input);
			mocks.ResetAllCalls();

			// act
			amqp_binary binary_value;
			int result = amqpvalue_get_binary(value, &binary_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 1, binary_value.length);
			ASSERT_ARE_EQUAL(int, 0, memcmp(binary_value.bytes, input, sizeof(input)));
		}

		/* Tests_SRS_AMQPVALUE_01_131: [amqpvalue_get_binary shall yield a pointer to the sequence of bytes held by the AMQP_VALUE in binary_value.data and fill in the binary_value.length argument the number of bytes held in the binary value.] */
		TEST_METHOD(amqpvalue_get_binary_0_byte_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x42 };
			amqp_binary binary_input = { NULL, 0 };
			AMQP_VALUE value = amqpvalue_create_binary(binary_input);
			mocks.ResetAllCalls();

			// act
			amqp_binary binary_value;
			int result = amqpvalue_get_binary(value, &binary_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 0, binary_value.length);
		}

		/* Tests_SRS_AMQPVALUE_01_132: [If any of the arguments is NULL then amqpvalue_get_binary shall return NULL.] */
		TEST_METHOD(when_the_value_argument_is_NULL_amqpvalue_get_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			mocks.ResetAllCalls();

			// act
			amqp_binary binary_value;
			int result = amqpvalue_get_binary(NULL, &binary_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_132: [If any of the arguments is NULL then amqpvalue_get_binary shall return NULL.] */
		TEST_METHOD(when_the_binary_value_argument_is_NULL_amqpvalue_get_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x42 };
			amqp_binary binary_input = { NULL, 0 };
			AMQP_VALUE value = amqpvalue_create_binary(binary_input);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_binary(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_133: [If the type of the value is not binary (was not created with amqpvalue_create_binary), then amqpvalue_get_binary shall return NULL.] */
		TEST_METHOD(amqpvalue_get_binary_on_a_null_amqp_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			amqp_binary binary_value;
			int result = amqpvalue_get_binary(value, &binary_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_binary */

		/* Tests_SRS_AMQPVALUE_01_135: [amqpvalue_create_string shall return a handle to an AMQP_VALUE that stores a sequence of Unicode characters.] */
		/* Tests_SRS_AMQPVALUE_01_028: [1.6.20 string A sequence of Unicode characters.] */
		TEST_METHOD(amqpvalue_create_string_with_one_char_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));

			// act
			AMQP_VALUE result = amqpvalue_create_string("a");

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_135: [amqpvalue_create_string shall return a handle to an AMQP_VALUE that stores a sequence of Unicode characters.] */
		/* Tests_SRS_AMQPVALUE_01_028: [1.6.20 string A sequence of Unicode characters.] */
		TEST_METHOD(amqpvalue_create_string_with_0_length_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(1));

			// act
			AMQP_VALUE result = amqpvalue_create_string("");

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_136: [If allocating the AMQP_VALUE fails then amqpvalue_create_string shall return NULL.] */
		TEST_METHOD(when_allocating_the_amqp_value_fails_then_amqpvalue_create_string_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0, 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_string("a");

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_136: [If allocating the AMQP_VALUE fails then amqpvalue_create_string shall return NULL.] */
		TEST_METHOD(when_allocating_the_string_in_the_amqp_value_fails_then_amqpvalue_create_string_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0, 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_string("a");

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_string */

		/* Tests_SRS_AMQPVALUE_01_138: [amqpvalue_get_string shall yield a pointer to the sequence of bytes held by the AMQP_VALUE in string_value.] */
		/* Tests_SRS_AMQPVALUE_01_141: [On success, amqpvalue_get_string shall return 0.] */
		TEST_METHOD(amqpvalue_get_string_1_byte_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_string("a");
			mocks.ResetAllCalls();

			// act
			const char* string_value;
			int result = amqpvalue_get_string(value, &string_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(char_ptr, "a", string_value);
		}

		/* Tests_SRS_AMQPVALUE_01_138: [amqpvalue_get_string shall yield a pointer to the sequence of bytes held by the AMQP_VALUE in string_value.] */
		TEST_METHOD(amqpvalue_get_string_0_length_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_string("");
			mocks.ResetAllCalls();

			// act
			const char* string_value;
			int result = amqpvalue_get_string(value, &string_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(char_ptr, "", string_value);
		}

		/* Tests_SRS_AMQPVALUE_01_139: [If any of the arguments is NULL then amqpvalue_get_string shall return a non-zero value.] */
		TEST_METHOD(when_the_value_argument_is_NULL_amqpvalue_get_string_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			mocks.ResetAllCalls();

			// act
			const char* string_value;
			int result = amqpvalue_get_string(NULL, &string_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_139: [If any of the arguments is NULL then amqpvalue_get_string shall return a non-zero value.] */
		TEST_METHOD(when_the_string_value_argument_is_NULL_amqpvalue_get_string_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_string("a");
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_string(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_140: [If the type of the value is not string (was not created with amqpvalue_create_string), then amqpvalue_get_string shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_string_on_a_null_amqp_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			const char* string_value;
			int result = amqpvalue_get_string(value, &string_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* amqpvalue_create_symbol */

		/* Tests_SRS_AMQPVALUE_01_142: [amqpvalue_create_symbol shall return a handle to an AMQP_VALUE that stores a uint32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_029: [1.6.21 symbol Symbolic values from a constrained domain.] */
		TEST_METHOD(amqpvalue_create_symbol_with_one_char_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_symbol(1);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_142: [amqpvalue_create_symbol shall return a handle to an AMQP_VALUE that stores a uint32_t value.] */
		/* Tests_SRS_AMQPVALUE_01_029: [1.6.21 symbol Symbolic values from a constrained domain.] */
		TEST_METHOD(amqpvalue_create_symbol_value_0xFFFFFFFF_length_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_symbol(0xFFFFFFFF);

			// assert
			ASSERT_IS_NOT_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_143: [If allocating the AMQP_VALUE fails then amqpvalue_create_symbol shall return NULL.] */
		TEST_METHOD(when_allocating_the_amqp_value_fails_then_amqpvalue_create_symbol_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char input[] = { 0x0, 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_symbol(42);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_get_symbol */

		/* Tests_SRS_AMQPVALUE_01_145: [amqpvalue_get_symbol shall fill in the symbol_value the uint32_t symbol value held by the AMQP_VALUE.] */
		/* Tests_SRS_AMQPVALUE_01_146: [On success, amqpvalue_get_symbol shall return 0.] */
		TEST_METHOD(amqpvalue_get_symbol_0_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t symbol_value;
			AMQP_VALUE value = amqpvalue_create_symbol(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_symbol(value, &symbol_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, 0, symbol_value);
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_145: [amqpvalue_get_symbol shall fill in the symbol_value the uint32_t symbol value held by the AMQP_VALUE.] */
		/* Tests_SRS_AMQPVALUE_01_146: [On success, amqpvalue_get_symbol shall return 0.] */
		TEST_METHOD(amqpvalue_get_symbol_0xFFFFFFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t symbol_value;
			AMQP_VALUE value = amqpvalue_create_symbol(0xFFFFFFFF);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_symbol(value, &symbol_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0xFFFFFFFF, symbol_value);
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_147: [If any of the arguments is NULL then amqpvalue_get_symbol shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_symbol_with_a_NULL_amqpvalue_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t symbol_value;

			// act
			int result = amqpvalue_get_symbol(NULL, &symbol_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_147: [If any of the arguments is NULL then amqpvalue_get_symbol shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_symbol_with_a_NULL_symbol_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_symbol(0);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_symbol(value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_148: [If the type of the value is not symbol (was not created with amqpvalue_create_symbol), then amqpvalue_get_string shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_symbol_with_an_amqpvalue_that_is_not_symbol_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t symbol_value;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_symbol(value, &symbol_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value);
		}

		/* amqpvalue_create_list */

		/* Tests_SRS_AMQPVALUE_01_149: [amqpvalue_create_list shall return a handle to an AMQP_VALUE that stores a list.] */
		/* Tests_SRS_AMQPVALUE_01_030: [1.6.22 list A sequence of polymorphic values.] */
		TEST_METHOD(amqpvalue_create_list_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_list();

			// assert
			ASSERT_IS_NOT_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_150: [If allocating the AMQP_VALUE fails then amqpvalue_create_list shall return NULL.] */
		TEST_METHOD(when_allocating_memory_fails_then_amqpvalue_create_list_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_list();

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_set_list_item_count */

		/* Tests_SRS_AMQPVALUE_01_152: [amqpvalue_set_list_item_count shall resize an AMQP list.] */
		/* Tests_SRS_AMQPVALUE_01_153: [On success amqpvalue_set_list_item_count shall return 0.] */
		TEST_METHOD(amqpvalue_set_list_item_count_with_1_count_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			int result = amqpvalue_set_list_item_count(list, 1);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_155: [If the value argument is NULL, amqpvalue_set_list_item_count shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_list_item_count_with_NULL_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			int result = amqpvalue_set_list_item_count(NULL, 1);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_154: [If allocating memory for the list according to the new size fails, then amqpvalue_set_list_item_count shall return a non-zero value, while preserving the existing list contents.] */
		TEST_METHOD(when_reallocating_fails_amqpvalue_set_list_item_count_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			int result = amqpvalue_set_list_item_count(list, 1);
			mocks.AssertActualAndExpectedCalls();

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_156: [If the value is not of type list, then amqpvalue_set_list_item_count shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_list_item_count_with_a_non_list_type_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_list_item_count(null_value, 1);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_156: [If the value is not of type list, then amqpvalue_set_list_item_count shall return a non-zero value.] */
		/* Tests_SRS_AMQPVALUE_01_162: [When a list is grown a null AMQP_VALUE shall be inserted as new list items to fill the list up to the new size.] */
		TEST_METHOD(amqpvalue_set_list_item_count_after_amqpvalue_set_list_item_count_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			int result = amqpvalue_set_list_item_count(list, 2);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_154: [If allocating memory for the list according to the new size fails, then amqpvalue_set_list_item_count shall return a non-zero value, while preserving the existing list contents.] */
		TEST_METHOD(when_allocating_the_new_null_element_fails_amqpvalue_set_list_item_count_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			int result = amqpvalue_set_list_item_count(list, 2);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_154: [If allocating memory for the list according to the new size fails, then amqpvalue_set_list_item_count shall return a non-zero value, while preserving the existing list contents.] */
		TEST_METHOD(when_allocating_the_new_null_element_fails_other_newly_allocated_items_are_rolled_back)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_list_item_count(list, 3);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_156: [If the value is not of type list, then amqpvalue_set_list_item_count shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_list_item_count_with_0_count_does_not_allocate_anything)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_list_item_count(list, 0);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_161: [When the list is shrunk, the extra items shall be freed by using amqp_value_destroy.] */
		TEST_METHOD(shrinking_a_list_by_1_frees_the_extra_value_but_does_not_resize)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 2);
			AMQP_VALUE item_1 = amqpvalue_get_list_item(list, 1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_list_item_count(list, 1);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* amqpvalue_get_list_item_count */

		/* Tests_SRS_AMQPVALUE_01_157: [amqpvalue_get_list_item_count shall fill in the size argument the number of items held by the AMQP list.] */
		/* Tests_SRS_AMQPVALUE_01_158: [On success amqpvalue_get_list_item_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_list_item_count_yields_0_on_an_empty_list)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(list, &item_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 0, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_157: [amqpvalue_get_list_item_count shall fill in the size argument the number of items held by the AMQP list.] */
		/* Tests_SRS_AMQPVALUE_01_158: [On success amqpvalue_get_list_item_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_list_item_count_on_a_list_with_size_1_yields_1)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 1);
			mocks.ResetAllCalls();

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(list, &item_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 1, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_157: [amqpvalue_get_list_item_count shall fill in the size argument the number of items held by the AMQP list.] */
		/* Tests_SRS_AMQPVALUE_01_158: [On success amqpvalue_get_list_item_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_list_item_count_on_a_list_with_size_2_yields_2)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 1);
			(void)amqpvalue_set_list_item_count(list, 2);
			mocks.ResetAllCalls();

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(list, &item_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 2, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_157: [amqpvalue_get_list_item_count shall fill in the size argument the number of items held by the AMQP list.] */
		/* Tests_SRS_AMQPVALUE_01_158: [On success amqpvalue_get_list_item_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_list_item_count_on_a_list_shrunk_to_1_item_yields_1)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 2);
			(void)amqpvalue_set_list_item_count(list, 1);
			mocks.ResetAllCalls();

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(list, &item_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 1, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_157: [amqpvalue_get_list_item_count shall fill in the size argument the number of items held by the AMQP list.] */
		/* Tests_SRS_AMQPVALUE_01_158: [On success amqpvalue_get_list_item_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_list_item_count_on_a_list_shrunk_to_empty_yields_0)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			(void)amqpvalue_set_list_item_count(list, 2);
			(void)amqpvalue_set_list_item_count(list, 0);
			mocks.ResetAllCalls();

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(list, &item_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 0, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_159: [If any of the arguments are NULL, amqpvalue_get_list_item_count shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_list_item_count_with_NULL_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(NULL, &item_count);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_159: [If any of the arguments are NULL, amqpvalue_get_list_item_count shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_list_item_count_with_NULL_item_count_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_list_item_count(list, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_160: [If the AMQP_VALUE is not a list then amqpvalue_get_list_item_count shall return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_list_item_count_on_a_non_list_type_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			uint32_t item_count;
			int result = amqpvalue_get_list_item_count(null_value, &item_count);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
		}

		/* amqpvalue_set_list_item */

		/* Tests_SRS_AMQPVALUE_01_163: [amqpvalue_set_list_item shall replace the item at the 0 based index-th position in the list identified by the value argument with the AMQP_VALUE specified by list_item_value.] */
		/* Tests_SRS_AMQPVALUE_01_164: [On success amqpvalue_set_list_item shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_166: [If index is greater than the current list item count, the list shall be grown to accommodate the new item.] */
		/* Tests_SRS_AMQPVALUE_01_168: [The item stored at the index-th position in the list shall be a clone of list_item_value.] */
		TEST_METHOD(amqpvalue_set_list_item_on_an_empty_list_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 0, null_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_163: [amqpvalue_set_list_item shall replace the item at the 0 based index-th position in the list identified by the value argument with the AMQP_VALUE specified by list_item_value.] */
		/* Tests_SRS_AMQPVALUE_01_164: [On success amqpvalue_set_list_item shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_166: [If index is greater than the current list item count, the list shall be grown to accommodate the new item.] */
		/* Tests_SRS_AMQPVALUE_01_168: [The item stored at the index-th position in the list shall be a clone of list_item_value.] */
		TEST_METHOD(amqpvalue_set_list_item_on_the_2nd_position_in_an_empty_list_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 1, null_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_169: [If cloning the item fails, amqpvalue_set_list_item shall fail and return a non-zero value.] */
		TEST_METHOD(when_cloning_the_item_fails_amqpvalue_set_list_item_fails_too)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			int result = amqpvalue_set_list_item(list, 1, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_172: [If growing the list fails, then amqpvalue_set_list_item shall fail and return a non-zero value.] */
		/* Tests_SRS_AMQPVALUE_01_166: [If index is greater than the current list item count, the list shall be grown to accommodate the new item.] */
		TEST_METHOD(when_allocating_the_filler_value_fails_amqpvalue_set_list_item_fails_too)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 1, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_172: [If growing the list fails, then amqpvalue_set_list_item shall fail and return a non-zero value.] */
		/* Tests_SRS_AMQPVALUE_01_166: [If index is greater than the current list item count, the list shall be grown to accommodate the new item.] */
		TEST_METHOD(when_allocating_the_filler_value_fails_amqpvalue_set_list_item_fails_too_and_frees_previous_filler_values)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

			// 1 cloned item + 2 fillers
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 2, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_172: [If growing the list fails, then amqpvalue_set_list_item shall fail and return a non-zero value.] */
		/* Tests_SRS_AMQPVALUE_01_166: [If index is greater than the current list item count, the list shall be grown to accommodate the new item.] */
		TEST_METHOD(when_reallocating_the_list_fails_amqpvalue_set_list_item_fails_too)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 0, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_163: [amqpvalue_set_list_item shall replace the item at the 0 based index-th position in the list identified by the value argument with the AMQP_VALUE specified by list_item_value.] */
		/* Tests_SRS_AMQPVALUE_01_164: [On success amqpvalue_set_list_item shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_167: [Any previous value stored at the position index in the list shall be freed by using amqpvalue_destroy.] */
		/* Tests_SRS_AMQPVALUE_01_168: [The item stored at the index-th position in the list shall be a clone of list_item_value.] */
		TEST_METHOD(amqpvalue_set_list_item_without_resizing_the_list_frees_the_previous_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			amqpvalue_set_list_item_count(list, 1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 0, null_value);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_165: [If value or list_item_value is NULL, amqpvalue_set_list_item shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_list_item_with_NULL_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_list_item(NULL, 0, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_165: [If value or list_item_value is NULL, amqpvalue_set_list_item shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_list_item_with_NULL_item_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_list_item(list, 0, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_170: [When amqpvalue_set_list_item fails due to not being able to clone the item or grow the list, the list shall not be altered.] */
		TEST_METHOD(when_cloning_fails_amqpvalue_set_list_item_is_not_altered)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			int result = amqpvalue_set_list_item(list, 0, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			uint32_t item_count;
			(void)amqpvalue_get_list_item_count(list, &item_count);
			ASSERT_ARE_EQUAL(uint32_t, 0, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_170: [When amqpvalue_set_list_item fails due to not being able to clone the item or grow the list, the list shall not be altered.] */
		TEST_METHOD(when_growing_fails_amqpvalue_set_list_item_is_not_altered)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_list_item(list, 0, null_value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			uint32_t item_count;
			(void)amqpvalue_get_list_item_count(list, &item_count);
			ASSERT_ARE_EQUAL(uint32_t, 0, item_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(null_value);
		}

		/* amqpvalue_get_list_item */

		/* Tests_SRS_AMQPVALUE_01_173: [amqpvalue_get_list_item shall return a copy of the AMQP_VALUE stored at the 0 based position index in the list identified by value.] */
		TEST_METHOD(amqpvalue_get_list_item_gets_the_first_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE uint_value = amqpvalue_create_uint(42);
			(void)amqpvalue_set_list_item(list, 0, uint_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(list, 0);

			// assert
			ASSERT_IS_NOT_NULL(result);
			uint32_t value = 0;
			(void)amqpvalue_get_uint(result, &value);
			ASSERT_ARE_EQUAL(uint32_t, 42, value);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(uint_value);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_173: [amqpvalue_get_list_item shall return a copy of the AMQP_VALUE stored at the 0 based position index in the list identified by value.] */
		TEST_METHOD(amqpvalue_get_list_item_gets_the_second_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE uint_value = amqpvalue_create_uint(42);
			AMQP_VALUE ulong_value = amqpvalue_create_ulong(43);
			(void)amqpvalue_set_list_item(list, 0, uint_value);
			(void)amqpvalue_set_list_item(list, 1, ulong_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(list, 1);

			// assert
			ASSERT_IS_NOT_NULL(result);
			uint64_t value = 0;
			(void)amqpvalue_get_ulong(result, &value);
			ASSERT_ARE_EQUAL(uint64_t, 43, value);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(uint_value);
			amqpvalue_destroy(ulong_value);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_174: [If the value argument is NULL, amqpvalue_get_list_item shall fail and return NULL.] */
		TEST_METHOD(when_list_handle_is_null_amqpvalue_get_list_item_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(NULL, 0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_175: [If index is greater or equal to the number of items in the list then amqpvalue_get_list_item shall fail and return NULL.] */
		TEST_METHOD(amqpvalue_get_list_item_with_index_too_high_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE uint_value = amqpvalue_create_uint(42);
			(void)amqpvalue_set_list_item(list, 0, uint_value);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(list, 1);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(uint_value);
		}

		/* Tests_SRS_AMQPVALUE_01_175: [If index is greater or equal to the number of items in the list then amqpvalue_get_list_item shall fail and return NULL.] */
		TEST_METHOD(amqpvalue_get_list_item_with_index_0_on_an_empty_list_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(list, 0);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* Tests_SRS_AMQPVALUE_01_176: [If cloning the item at position index fails, then amqpvalue_get_list_item shall fail and return NULL.] */
		TEST_METHOD(when_cloning_the_item_fails_then_amqpvalue_get_list_item_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_list();
			AMQP_VALUE uint_value = amqpvalue_create_uint(42);
			(void)amqpvalue_set_list_item(list, 0, uint_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(list, 0);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
			amqpvalue_destroy(uint_value);
		}

		/* Tests_SRS_AMQPVALUE_01_177: [If value is not a list then amqpvalue_get_list_item shall fail and return NULL.] */
		TEST_METHOD(amqpvalue_get_list_item_called_with_a_non_list_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE list = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(list, 0);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(list);
		}

		/* amqpvalue_create_map */

		/* Tests_SRS_AMQPVALUE_01_178: [amqpvalue_create_map shall create an AMQP value that holds a map and return a handle to it.] */
		/* Tests_SRS_AMQPVALUE_01_031: [1.6.23 map A polymorphic mapping from distinct keys to values.] */
		TEST_METHOD(when_underlying_calls_succeed_amqpvalue_create_map_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_create_map();

			// assert
			ASSERT_IS_NOT_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_179: [If allocating memory for the map fails, then amqpvalue_create_map shall return NULL.] */
		TEST_METHOD(when_allocating_memory_for_the_map_fails_amqpvalue_create_map_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_create_map();

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_180: [The number of key/value pairs in the newly created map shall be zero.] */
		TEST_METHOD(amqpvalue_create_map_creates_a_map_with_no_pairs)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			mocks.ResetAllCalls();

			// act
			uint32_t pair_count;
			(void)amqpvalue_get_map_pair_count(map, &pair_count);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, 0, pair_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
		}

		/* amqpvalue_set_map_value */

		/* Tests_SRS_AMQPVALUE_01_181: [amqpvalue_set_map_value shall set the value in the map identified by the map argument for a key/value pair identified by the key argument.] */
		/* Tests_SRS_AMQPVALUE_01_182: [On success amqpvalue_set_map_value shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_185: [When storing the key or value, their contents shall be cloned.] */
		TEST_METHOD(amqpvalue_set_map_value_adds_one_key_value_pair)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE null = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

			// act
			uint32_t pair_count;
			int result = amqpvalue_set_map_value(map, null, null);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			(void)amqpvalue_get_map_pair_count(map, &pair_count);
			ASSERT_ARE_EQUAL(uint32_t, 1, pair_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(null);
		}

		/* Tests_SRS_AMQPVALUE_01_181: [amqpvalue_set_map_value shall set the value in the map identified by the map argument for a key/value pair identified by the key argument.] */
		/* Tests_SRS_AMQPVALUE_01_182: [On success amqpvalue_set_map_value shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_185: [When storing the key or value, their contents shall be cloned.] */
		TEST_METHOD(amqpvalue_set_map_value_adds_2_key_value_pairs)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, value1, value1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

			// act
			uint32_t pair_count;
			int result = amqpvalue_set_map_value(map, value2, value2);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			(void)amqpvalue_get_map_pair_count(map, &pair_count);
			ASSERT_ARE_EQUAL(uint32_t, 2, pair_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_183: [If any of the arguments are NULL, amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_map_value_with_NULL_map_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_map_value(NULL, value, value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_183: [If any of the arguments are NULL, amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_map_value_with_NULL_key_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_map_value(map, NULL, value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_183: [If any of the arguments are NULL, amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_map_value_with_NULL_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_map_value(map, value, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_184: [If the key already exists in the map, its value shall be replaced with the value provided by the value argument.] */
		/* Tests_SRS_AMQPVALUE_01_185: [When storing the key or value, their contents shall be cloned.] */
		TEST_METHOD(amqpvalue_set_map_value_with_an_already_existing_value_replaces_the_old_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE key = amqpvalue_create_uint(1);
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, key, value1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			int result = amqpvalue_set_map_value(map, key, value2);

			// assert
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, 0, result);
			uint32_t pair_count;
			(void)amqpvalue_get_map_pair_count(map, &pair_count);
			ASSERT_ARE_EQUAL(uint32_t, 1, pair_count);
			AMQP_VALUE result_value = amqpvalue_get_map_value(map, key);
			ASSERT_IS_TRUE(amqpvalue_are_equal(value2, result_value));

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(result_value);
		}

		/* Tests_SRS_AMQPVALUE_01_186: [If allocating memory to hold a new key/value pair fails, amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(when_reallocating_memory_to_hold_the_map_fails_then_amqpvalue_set_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_map_value(map, value, value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_188: [If cloning the value fails, amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(when_cloning_the_value_fails_then_amqpvalue_set_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			int result = amqpvalue_set_map_value(map, value, value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_187: [If cloning the key fails, amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(when_cloning_the_key_fails_then_amqpvalue_set_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			int result = amqpvalue_set_map_value(map, value, value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_196: [If the map argument is not an AMQP value created with the amqpvalue_create_map function than amqpvalue_set_map_value shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_set_map_value_on_a_non_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_set_map_value(null_value, value, value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
			amqpvalue_destroy(value);
		}

		/* amqpvalue_get_map_value */

		/* Tests_SRS_AMQPVALUE_01_189: [amqpvalue_get_map_value shall return the value whose key is identified by the key argument.] */
		/* Tests_SRS_AMQPVALUE_01_192: [The returned value shall be a clone of the actual value stored in the map.] */
		TEST_METHOD(amqpvalue_get_map_value_on_a_map_with_one_pair_returns_the_value_for_the_key)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, value, value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_get_map_value(map, value);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(value, result));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_189: [amqpvalue_get_map_value shall return the value whose key is identified by the key argument.] */
		/* Tests_SRS_AMQPVALUE_01_192: [The returned value shall be a clone of the actual value stored in the map.] */
		TEST_METHOD(amqpvalue_get_map_value_find_second_key_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, value1, value1);
			(void)amqpvalue_set_map_value(map, value2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_get_map_value(map, value2);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(value2, result));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_190: [If any argument is NULL, amqpvalue_get_map_value shall return NULL.] */
		TEST_METHOD(amqpvalue_get_map_value_with_NULL_map_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_map_value(NULL, value);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_190: [If any argument is NULL, amqpvalue_get_map_value shall return NULL.] */
		TEST_METHOD(amqpvalue_get_map_value_with_NULL_key_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_map_value(map, NULL);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
		}

		/* Tests_SRS_AMQPVALUE_01_191: [If the key cannot be found, amqpvalue_get_map_value shall return NULL.] */
		TEST_METHOD(amqpvalue_get_map_value_with_a_key_that_does_not_exist_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, value1, value1);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_map_value(map, value2);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_197: [If the map argument is not an AMQP value created with the amqpvalue_create_map function than amqpvalue_get_map_value shall return NULL.] */
		TEST_METHOD(amqpvalue_get_map_value_for_a_non_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			AMQP_VALUE key = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE result = amqpvalue_get_map_value(null_value, key);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
			amqpvalue_destroy(key);
		}

		/* amqpvalue_get_map_pair_count */

		/* Tests_SRS_AMQPVALUE_01_193: [amqpvalue_get_map_pair_count shall fill in the number of key/value pairs in the map in the pair_count argument.] */
		/* Tests_SRS_AMQPVALUE_01_194: [On success amqpvalue_get_map_pair_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_map_pair_count_yields_0_on_an_empty_map)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			mocks.ResetAllCalls();

			// act
			uint32_t pair_count;
			int result = amqpvalue_get_map_pair_count(map, &pair_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 0, pair_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
		}

		/* Tests_SRS_AMQPVALUE_01_193: [amqpvalue_get_map_pair_count shall fill in the number of key/value pairs in the map in the pair_count argument.] */
		/* Tests_SRS_AMQPVALUE_01_194: [On success amqpvalue_get_map_pair_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_map_pair_count_yields_1_on_a_map_with_1_pair)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, value, value);
			mocks.ResetAllCalls();

			// act
			uint32_t pair_count;
			int result = amqpvalue_get_map_pair_count(map, &pair_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 1, pair_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_193: [amqpvalue_get_map_pair_count shall fill in the number of key/value pairs in the map in the pair_count argument.] */
		/* Tests_SRS_AMQPVALUE_01_194: [On success amqpvalue_get_map_pair_count shall return 0.] */
		TEST_METHOD(amqpvalue_get_map_pair_count_yields_2_on_a_map_with_2_pairs)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, value1, value1);
			(void)amqpvalue_set_map_value(map, value2, value2);
			mocks.ResetAllCalls();

			// act
			uint32_t pair_count;
			int result = amqpvalue_get_map_pair_count(map, &pair_count);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(uint32_t, 2, pair_count);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_195: [If any of the arguments is NULL, amqpvalue_get_map_pair_count shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_pair_with_NULL_map_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			uint32_t pair_count;
			int result = amqpvalue_get_map_pair_count(NULL, &pair_count);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_195: [If any of the arguments is NULL, amqpvalue_get_map_pair_count shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_pair_with_NULL_pair_count_argument_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_map_pair_count(map, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
		}

		/* Tests_SRS_AMQPVALUE_01_198: [If the map argument is not an AMQP value created with the amqpvalue_create_map function then amqpvalue_get_map_pair_count shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_pair_count_on_a_non_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			uint32_t pair_count;
			int result = amqpvalue_get_map_pair_count(null_value, &pair_count);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
		}

		/* amqpvalue_get_map_key_value_pair */

		/* Tests_SRS_AMQPVALUE_01_199: [amqpvalue_get_map_key_value_pair shall fill in the key and value arguments copies of the key/value pair on the 0 based position index in a map.] */
		/* Tests_SRS_AMQPVALUE_01_200: [On success amqpvalue_get_map_key_value_pair shall return 0.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_with_1_element_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, no1, no1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 0, &key, &value);

			// assert
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_IS_NOT_NULL(key);
			ASSERT_IS_NOT_NULL(value);
			ASSERT_IS_TRUE(amqpvalue_are_equal(key, no1));
			ASSERT_IS_TRUE(amqpvalue_are_equal(value, no1));

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
			amqpvalue_destroy(key);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_199: [amqpvalue_get_map_key_value_pair shall fill in the key and value arguments copies of the key/value pair on the 0 based position index in a map.] */
		/* Tests_SRS_AMQPVALUE_01_200: [On success amqpvalue_get_map_key_value_pair shall return 0.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_with_1_element_different_key_and_value_data_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			AMQP_VALUE no2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, no1, no2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 0, &key, &value);

			// assert
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_IS_NOT_NULL(key);
			ASSERT_IS_NOT_NULL(value);
			ASSERT_IS_TRUE(amqpvalue_are_equal(key, no1));
			ASSERT_IS_TRUE(amqpvalue_are_equal(value, no2));

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
			amqpvalue_destroy(no2);
			amqpvalue_destroy(key);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_199: [amqpvalue_get_map_key_value_pair shall fill in the key and value arguments copies of the key/value pair on the 0 based position index in a map.] */
		/* Tests_SRS_AMQPVALUE_01_200: [On success amqpvalue_get_map_key_value_pair shall return 0.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_second_element_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			AMQP_VALUE no2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(map, no1, no1);
			(void)amqpvalue_set_map_value(map, no2, no2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 1, &key, &value);

			// assert
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_IS_NOT_NULL(key);
			ASSERT_IS_NOT_NULL(value);
			ASSERT_IS_TRUE(amqpvalue_are_equal(key, no2));
			ASSERT_IS_TRUE(amqpvalue_are_equal(value, no2));

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
			amqpvalue_destroy(no2);
			amqpvalue_destroy(key);
			amqpvalue_destroy(value);
		}

		/* Tests_SRS_AMQPVALUE_01_201: [If any of the map, key or value arguments is NULL, amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_with_NULL_map_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(NULL, 0, &key, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_201: [If any of the map, key or value arguments is NULL, amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_with_NULL_key_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, no1, no1);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 0, NULL, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
		}

		/* Tests_SRS_AMQPVALUE_01_201: [If any of the map, key or value arguments is NULL, amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_with_NULL_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, no1, no1);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE key;
			int result = amqpvalue_get_map_key_value_pair(map, 0, &key, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
		}

		/* Tests_SRS_AMQPVALUE_01_202: [If cloning the key fails, amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(when_cloning_key_fails_then_amqpvalue_get_map_key_value_pair_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, no1, no1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 0, &key, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
		}

		/* Tests_SRS_AMQPVALUE_01_203: [If cloning the value fails, amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(when_cloning_value_fails_then_amqpvalue_get_map_key_value_pair_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, no1, no1);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 0, &key, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
		}

		/* Tests_SRS_AMQPVALUE_01_204: [If the index argument is greater or equal to the number of key/value pairs in the map then amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_on_an_empty_map_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 0, &key, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
		}

		/* Tests_SRS_AMQPVALUE_01_204: [If the index argument is greater or equal to the number of key/value pairs in the map then amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_with_index_equal_number_of_pairs_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE map = amqpvalue_create_map();
			AMQP_VALUE no1 = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(map, no1, no1);
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(map, 1, &key, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(map);
			amqpvalue_destroy(no1);
		}

		/* Tests_SRS_AMQPVALUE_01_205: [If the map argument is not an AMQP value created with the amqpvalue_create_map function then amqpvalue_get_map_key_value_pair shall fail and return a non-zero value.] */
		TEST_METHOD(amqpvalue_get_map_key_value_pair_on_a_non_map_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE null_value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			AMQP_VALUE key;
			AMQP_VALUE value;
			int result = amqpvalue_get_map_key_value_pair(null_value, 0, &key, &value);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(null_value);
		}

		/* amqpvalue_are_equal */

		/* Tests_SRS_AMQPVALUE_01_207: [If value1 and value2 are NULL, amqpvalue_are_equal shall return true.] */
		TEST_METHOD(amqpvalue_are_equal_with_NULL_values_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			bool result = amqpvalue_are_equal(NULL, NULL);

			// assert
			ASSERT_IS_TRUE(result);
		}

		/* Tests_SRS_AMQPVALUE_01_208: [If one of the arguments is NULL and the other is not, amqpvalue_are_equal shall return false.] */
		TEST_METHOD(when_value2_is_NULL_and_value1_is_not_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, NULL);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
		}

		/* Tests_SRS_AMQPVALUE_01_208: [If one of the arguments is NULL and the other is not, amqpvalue_are_equal shall return false.] */
		TEST_METHOD(when_value1_is_NULL_and_value2_is_not_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value2 = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(NULL, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_209: [If the types for value1 and value2 are different amqpvalue_are_equal shall return false.] */
		TEST_METHOD(when_value1_is_uint_and_value2_is_ulong_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_ulong(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_210: [- null: always equal.] */
		TEST_METHOD(for_2_null_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_null();
			AMQP_VALUE value2 = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_211: [- boolean: compare the bool content.] */
		TEST_METHOD(for_2_equal_boolean_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_boolean(false);
			AMQP_VALUE value2 = amqpvalue_create_boolean(false);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_211: [- boolean: compare the bool content.] */
		TEST_METHOD(for_2_different_boolean_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_boolean(false);
			AMQP_VALUE value2 = amqpvalue_create_boolean(true);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_212: [- ubyte: compare the unsigned char content.] */
		TEST_METHOD(for_2_equal_ubyte_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_ubyte(42);
			AMQP_VALUE value2 = amqpvalue_create_ubyte(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_212: [- ubyte: compare the unsigned char content.] */
		TEST_METHOD(for_2_different_ubyte_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_ubyte(42);
			AMQP_VALUE value2 = amqpvalue_create_ubyte(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_213: [- ushort: compare the uint16_t content.] */
		TEST_METHOD(for_2_equal_ushort_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_ushort(42);
			AMQP_VALUE value2 = amqpvalue_create_ushort(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_212: [- ushort: compare the unsigned char content.] */
		TEST_METHOD(for_2_different_ushort_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_ushort(42);
			AMQP_VALUE value2 = amqpvalue_create_ushort(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_214: [- uint: compare the uint32_t content.] */
		TEST_METHOD(for_2_equal_uint_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_214: [- uint: compare the uint32_t content.] */
		TEST_METHOD(for_2_different_uint_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_uint(42);
			AMQP_VALUE value2 = amqpvalue_create_uint(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_215: [- ulong: compare the uint64_t content.] */
		TEST_METHOD(for_2_equal_ulong_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_ulong(42);
			AMQP_VALUE value2 = amqpvalue_create_ulong(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_215: [- ulong: compare the uint64_t content.] */
		TEST_METHOD(for_2_different_ulong_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_ulong(42);
			AMQP_VALUE value2 = amqpvalue_create_ulong(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_216: [- byte: compare the char content.] */
		TEST_METHOD(for_2_equal_byte_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_byte(42);
			AMQP_VALUE value2 = amqpvalue_create_byte(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_216: [- byte: compare the char content.] */
		TEST_METHOD(for_2_different_byte_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_byte(42);
			AMQP_VALUE value2 = amqpvalue_create_byte(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_217: [- short: compare the int16_t content.] */
		TEST_METHOD(for_2_equal_short_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_short(42);
			AMQP_VALUE value2 = amqpvalue_create_short(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_217: [- short: compare the int16_t content.] */
		TEST_METHOD(for_2_different_short_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_short(42);
			AMQP_VALUE value2 = amqpvalue_create_short(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_218: [- int: compare the int32_t content.] */
		TEST_METHOD(for_2_equal_int_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_int(42);
			AMQP_VALUE value2 = amqpvalue_create_int(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_218: [- int: compare the int32_t content.] */
		TEST_METHOD(for_2_different_int_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_int(42);
			AMQP_VALUE value2 = amqpvalue_create_int(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_219: [- long: compare the int64_t content.] */
		TEST_METHOD(for_2_equal_long_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_long(42);
			AMQP_VALUE value2 = amqpvalue_create_long(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_219: [- long: compare the int64_t content.] */
		TEST_METHOD(for_2_different_long_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_long(42);
			AMQP_VALUE value2 = amqpvalue_create_long(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_224: [- float: compare the float content.] */
		TEST_METHOD(for_2_equal_float_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_float(42);
			AMQP_VALUE value2 = amqpvalue_create_float(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_224: [- float: compare the float content.] */
		TEST_METHOD(for_2_different_float_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_float(42);
			AMQP_VALUE value2 = amqpvalue_create_float(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_225: [- double: compare the double content.] */
		TEST_METHOD(for_2_equal_double_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_double(42);
			AMQP_VALUE value2 = amqpvalue_create_double(42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_225: [- double: compare the double content.] */
		TEST_METHOD(for_2_different_double_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_double(42);
			AMQP_VALUE value2 = amqpvalue_create_double(43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_226: [- char: compare the UNICODE character.] */
		TEST_METHOD(for_2_equal_char_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_char(0x42);
			AMQP_VALUE value2 = amqpvalue_create_char(0x42);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_226: [- char: compare the UNICODE character.] */
		TEST_METHOD(for_2_different_char_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_char(0x42);
			AMQP_VALUE value2 = amqpvalue_create_char(0x43);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_227: [- timestamp: compare the underlying 64 bit integer.] */
		TEST_METHOD(for_2_equal_timestamp_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_timestamp(0x4242424242424242);
			AMQP_VALUE value2 = amqpvalue_create_timestamp(0x4242424242424242);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_227: [- timestamp: compare the underlying 64 bit integer.] */
		TEST_METHOD(for_2_different_timestamp_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_timestamp(0x4242424242424242);
			AMQP_VALUE value2 = amqpvalue_create_timestamp(0x4242424242424243);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_228: [- uuid: compare all uuid bytes.] */
		TEST_METHOD(for_2_equal_uuid_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid bin1 = { 0x42 };
			amqp_uuid bin2 = { 0x42 };
			AMQP_VALUE value1 = amqpvalue_create_uuid(bin1);
			AMQP_VALUE value2 = amqpvalue_create_uuid(bin2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_228: [- uuid: compare all uuid bytes.] */
		TEST_METHOD(for_2_different_uuid_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid bin1 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42 };
			amqp_uuid bin2 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x43 };
			AMQP_VALUE value1 = amqpvalue_create_uuid(bin1);
			AMQP_VALUE value2 = amqpvalue_create_uuid(bin2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_229: [- binary: compare all binary bytes.] */
		TEST_METHOD(for_2_equal_binary_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes1[] = { 0x42 };
			unsigned char bytes2[] = { 0x42 };
			amqp_binary bin1 = { bytes1, sizeof(bytes1) };
			amqp_binary bin2 = { bytes2, sizeof(bytes2) };
			AMQP_VALUE value1 = amqpvalue_create_binary(bin1);
			AMQP_VALUE value2 = amqpvalue_create_binary(bin2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_229: [- binary: compare all binary bytes.] */
		TEST_METHOD(for_2_different_binary_values_with_same_length_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes1[] = { 0x42 };
			unsigned char bytes2[] = { 0x43 };
			amqp_binary bin1 = { bytes1, sizeof(bytes1) };
			amqp_binary bin2 = { bytes2, sizeof(bytes2) };
			AMQP_VALUE value1 = amqpvalue_create_binary(bin1);
			AMQP_VALUE value2 = amqpvalue_create_binary(bin2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_229: [- binary: compare all binary bytes.] */
		TEST_METHOD(for_2_different_binary_values_with_different_length_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes1[] = { 0x42 };
			unsigned char bytes2[] = { 0x42, 0x43 };
			amqp_binary bin1 = { bytes1, sizeof(bytes1) };
			amqp_binary bin2 = { bytes2, sizeof(bytes2) };
			AMQP_VALUE value1 = amqpvalue_create_binary(bin1);
			AMQP_VALUE value2 = amqpvalue_create_binary(bin2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_230: [- string: compare all string characters.] */
		TEST_METHOD(for_2_equal_string_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_string("a");
			AMQP_VALUE value2 = amqpvalue_create_string("a");
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_230: [- string: compare all string characters.] */
		TEST_METHOD(for_2_different_string_values_with_same_length_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_string("a");
			AMQP_VALUE value2 = amqpvalue_create_string("b");
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_230: [- string: compare all string characters.] */
		TEST_METHOD(for_2_different_string_values_with_different_length_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_string("a");
			AMQP_VALUE value2 = amqpvalue_create_string("ab");
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_empty_list_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_lists_with_one_null_item_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value1, 0, null_value);
			(void)amqpvalue_set_list_item(value2, 0, null_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_lists_with_different_number_of_null_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value1, 0, null_value);
			(void)amqpvalue_set_list_item(value2, 0, null_value);
			(void)amqpvalue_set_list_item(value2, 1, null_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_lists_one_empty_and_one_with_a_value_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value1, 0, null_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_lists_with_one_identical_int_value_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE int_value = amqpvalue_create_int(42);
			(void)amqpvalue_set_list_item(value1, 0, int_value);
			(void)amqpvalue_set_list_item(value2, 0, int_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(int_value);
		}

		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_lists_with_2_different_int_values_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE int_value1 = amqpvalue_create_int(42);
			AMQP_VALUE int_value2 = amqpvalue_create_int(43);
			(void)amqpvalue_set_list_item(value1, 0, int_value1);
			(void)amqpvalue_set_list_item(value2, 0, int_value2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(int_value1);
			amqpvalue_destroy(int_value2);
		}

		/* Tests_SRS_AMQPVALUE_01_231: [- list: compare list item count and each element.] */
		TEST_METHOD(for_2_lists_with_different_int_values_at_index_1_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE int_value1 = amqpvalue_create_int(42);
			AMQP_VALUE int_value2 = amqpvalue_create_int(43);
			(void)amqpvalue_set_list_item(value1, 0, int_value1);
			(void)amqpvalue_set_list_item(value2, 0, int_value1);
			(void)amqpvalue_set_list_item(value1, 0, int_value1);
			(void)amqpvalue_set_list_item(value2, 0, int_value2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(int_value1);
			amqpvalue_destroy(int_value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_232: [Nesting shall be considered in comparison.] */
		TEST_METHOD(for_2_lists_each_with_one_empty_list_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE inner_list1 = amqpvalue_create_list();
			AMQP_VALUE inner_list2 = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(value1, 0, inner_list1);
			(void)amqpvalue_set_list_item(value2, 0, inner_list2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(inner_list1);
			amqpvalue_destroy(inner_list2);
		}

		/* Tests_SRS_AMQPVALUE_01_232: [Nesting shall be considered in comparison.] */
		TEST_METHOD(when_inner_lists_have_different_item_count_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE inner_list1 = amqpvalue_create_list();
			AMQP_VALUE inner_list2 = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(inner_list1, 0, null_value);
			(void)amqpvalue_set_list_item(value1, 0, inner_list1);
			(void)amqpvalue_set_list_item(value2, 0, inner_list2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(inner_list1);
			amqpvalue_destroy(inner_list2);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_232: [Nesting shall be considered in comparison.] */
		TEST_METHOD(when_inner_lists_have_each_1_item_count_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE inner_list1 = amqpvalue_create_list();
			AMQP_VALUE inner_list2 = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(inner_list1, 0, null_value);
			(void)amqpvalue_set_list_item(inner_list2, 0, null_value);
			(void)amqpvalue_set_list_item(value1, 0, inner_list1);
			(void)amqpvalue_set_list_item(value2, 0, inner_list2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(inner_list1);
			amqpvalue_destroy(inner_list2);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_232: [Nesting shall be considered in comparison.] */
		TEST_METHOD(when_inner_lists_have_each_1_item_count_but_items_are_different_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_list();
			AMQP_VALUE value2 = amqpvalue_create_list();
			AMQP_VALUE inner_list1 = amqpvalue_create_list();
			AMQP_VALUE inner_list2 = amqpvalue_create_list();
			AMQP_VALUE inner_item1 = amqpvalue_create_uint(42);
			AMQP_VALUE inner_item2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_list_item(inner_list1, 0, inner_item1);
			(void)amqpvalue_set_list_item(inner_list2, 0, inner_item2);
			(void)amqpvalue_set_list_item(value1, 0, inner_list1);
			(void)amqpvalue_set_list_item(value2, 0, inner_list2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(inner_list1);
			amqpvalue_destroy(inner_list2);
			amqpvalue_destroy(inner_item1);
			amqpvalue_destroy(inner_item2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_empty_map_values_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_maps_with_one_null_key_and_null_value_item_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_map_value(value1, null_value, null_value);
			(void)amqpvalue_set_map_value(value2, null_value, null_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(null_value);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_maps_with_one_pair_each_where_key_is_different_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE pair_value = amqpvalue_create_uint(42);
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE key2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(value1, key1, pair_value);
			(void)amqpvalue_set_map_value(value2, key2, pair_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(pair_value);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_maps_with_one_pair_each_where_value_is_different_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE pair_value1 = amqpvalue_create_uint(42);
			AMQP_VALUE pair_value2 = amqpvalue_create_uint(43);
			AMQP_VALUE key = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(value1, key, pair_value1);
			(void)amqpvalue_set_map_value(value2, key, pair_value2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(pair_value1);
			amqpvalue_destroy(pair_value2);
			amqpvalue_destroy(key);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_maps_with_one_pair_each_where_key_and_value_are_equal_amqpvalue_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE pair_value = amqpvalue_create_uint(42);
			AMQP_VALUE key = amqpvalue_create_uint(42);
			(void)amqpvalue_set_map_value(value1, key, pair_value);
			(void)amqpvalue_set_map_value(value2, key, pair_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(pair_value);
			amqpvalue_destroy(key);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_maps_with_different_pair_count_amqpvalue_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE pair_value = amqpvalue_create_uint(42);
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE key2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(value1, key1, pair_value);
			(void)amqpvalue_set_map_value(value2, key1, pair_value);
			(void)amqpvalue_set_map_value(value2, key2, pair_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(pair_value);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		TEST_METHOD(for_2_maps_with_2_equal_pairs_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE pair_value = amqpvalue_create_uint(42);
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE key2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(value1, key1, pair_value);
			(void)amqpvalue_set_map_value(value1, key2, pair_value);
			(void)amqpvalue_set_map_value(value2, key1, pair_value);
			(void)amqpvalue_set_map_value(value2, key2, pair_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(pair_value);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
		}

		/* Tests_SRS_AMQPVALUE_01_206: [amqpvalue_are_equal shall return true if the contents of value1 and value2 are equal.] */
		/* Tests_SRS_AMQPVALUE_01_233: [- map: compare map pair count and each key/value pair.] */
		/* Tests_SRS_AMQPVALUE_01_126: [Unless known to be otherwise, maps MUST be considered to be ordered, that is, the order of the key-value pairs is semantically important and two maps which are different only in the order in which their key-value pairs are encoded are not equal.] */
		TEST_METHOD(for_2_maps_with_2_equal_pairs_out_of_order_are_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE pair_value = amqpvalue_create_uint(42);
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE key2 = amqpvalue_create_uint(43);
			(void)amqpvalue_set_map_value(value1, key1, pair_value);
			(void)amqpvalue_set_map_value(value1, key2, pair_value);
			(void)amqpvalue_set_map_value(value2, key2, pair_value);
			(void)amqpvalue_set_map_value(value2, key1, pair_value);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(pair_value);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
		}

		/* Tests_SRS_AMQPVALUE_01_234: [Nesting shall be considered in comparison.] */
		TEST_METHOD(when_inner_maps_are_equal_are_equal_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE key = amqpvalue_create_uint(42);
			AMQP_VALUE inner_map1 = amqpvalue_create_map();
			AMQP_VALUE inner_map2 = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(value1, key, inner_map1);
			(void)amqpvalue_set_map_value(value2, key, inner_map2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_TRUE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(key);
			amqpvalue_destroy(inner_map1);
			amqpvalue_destroy(inner_map2);
		}

		/* Tests_SRS_AMQPVALUE_01_234: [Nesting shall be considered in comparison.] */
		TEST_METHOD(when_inner_maps_are_equal_not_are_not_equal_returns_false)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value1 = amqpvalue_create_map();
			AMQP_VALUE value2 = amqpvalue_create_map();
			AMQP_VALUE key = amqpvalue_create_uint(42);
			AMQP_VALUE pair_value = amqpvalue_create_uint(43);
			AMQP_VALUE inner_map1 = amqpvalue_create_map();
			AMQP_VALUE inner_map2 = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(inner_map1, key, pair_value);
			(void)amqpvalue_set_map_value(value1, key, inner_map1);
			(void)amqpvalue_set_map_value(value2, key, inner_map2);
			mocks.ResetAllCalls();

			// act
			bool result = amqpvalue_are_equal(value1, value2);

			// assert
			ASSERT_IS_FALSE(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(value1);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(key);
			amqpvalue_destroy(pair_value);
			amqpvalue_destroy(inner_map1);
			amqpvalue_destroy(inner_map2);
		}

END_TEST_SUITE(connection_unittests)
