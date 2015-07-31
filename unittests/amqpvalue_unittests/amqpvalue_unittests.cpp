#include <stdio.h>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "amqpvalue.h"

/* Requirements satisfied by the current implementation without any code:
Tests_SRS_AMQPVALUE_01_270: [<encoding code="0x56" category="fixed" width="1" label="boolean with the octet 0x00 being false and octet 0x01 being true"/>]
Tests_SRS_AMQPVALUE_01_099: [Represents an approximate point in time using the Unix time t [IEEE1003] encoding of UTC, but with a precision of milliseconds.]
*/

static int bool_Compare(bool left, bool right)
{
    return left != right;
}

static void bool_ToString(char* string, size_t bufferSize, bool val)
{
    (void)bufferSize;
    (void)strcpy(string, val ? "true" : "false");
}

static unsigned char* encoded_bytes;
static size_t encoded_byte_count;
static char actual_stringified_encoded[8192];
static char expected_stringified_encoded[8192];
static int when_shall_encoder_output_fail = 0;
static int encoder_output_call_count;
static size_t decoded_value_count;
static AMQP_VALUE* decoded_values;

void stringify_bytes(const unsigned char* bytes, size_t byte_count, char* output_string)
{
	size_t i;
	size_t pos = 0;

	output_string[pos++] = '[';
	for (i = 0; i < byte_count; i++)
	{
		(void)sprintf(&output_string[pos], "0x%02X", bytes[i]);
		if (i < byte_count - 1)
		{
			strcat(output_string, ",");
		}
		pos = strlen(output_string);
	}
	output_string[pos++] = ']';
	output_string[pos++] = '\0';
}

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

	MOCK_STATIC_METHOD_3(, int, test_encoder_output, void*, context, const void*, bytes, size_t, length)
		unsigned char* new_bytes = (unsigned char*)realloc(encoded_bytes, encoded_byte_count + length);
		if (new_bytes != NULL)
		{
			encoded_bytes = new_bytes;
			(void)memcpy(encoded_bytes + encoded_byte_count, bytes, length);
			encoded_byte_count += length;
		}
		encoder_output_call_count++;
	MOCK_METHOD_END(int, (encoder_output_call_count == when_shall_encoder_output_fail) ? 1 : 0);

	MOCK_STATIC_METHOD_2(, int, value_decoded_callback, void*, context, AMQP_VALUE, decoded_value)
		AMQP_VALUE* new_values = (AMQP_VALUE*)realloc(decoded_values, sizeof(AMQP_VALUE) * (decoded_value_count + 1));
		if (new_values != NULL)
		{
			decoded_values = new_values;
			new_values[decoded_value_count] = amqpvalue_clone(decoded_value);
			decoded_value_count++;
		}
	MOCK_METHOD_END(int, 0);
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(amqpvalue_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqpvalue_mocks, , void*, amqpalloc_realloc, void*, ptr, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqpvalue_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_3(amqpvalue_mocks, , int, test_encoder_output, void*, context, const void*, bytes, size_t, length);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqpvalue_mocks, , int, value_decoded_callback, void*, context, AMQP_VALUE, decoded_value);
}

static void* test_context = (void*)0x4243;

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(amqpvalue_unittests)

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
			encoder_output_call_count = 0;
			when_shall_encoder_output_fail = 0;
		}

		TEST_FUNCTION_CLEANUP(method_cleanup)
		{
			amqpvalue_mocks mocks;
			mocks.SetPerformAutomaticCallComparison(AUTOMATIC_CALL_COMPARISON_OFF);

			if (encoded_bytes != NULL)
			{
				free(encoded_bytes);
				encoded_bytes = NULL;
			}
			encoded_byte_count = 0;

			if (decoded_values != NULL)
			{
				size_t i;
				for (i = 0; i < decoded_value_count; i++)
				{
					amqpvalue_destroy(decoded_values[i]);
				}
				free(decoded_values);
				decoded_values = NULL;
			}
			decoded_value_count = 0;

			if (!MicroMockReleaseMutex(test_serialize_mutex))
			{
				ASSERT_FAIL("Could not release test serialization mutex.");
			}
		}

		/* amqpvalue_create_null */

		/* Tests_SRS_AMQPVALUE_01_001: [amqpvalue_create_null shall return a handle to an AMQP_VALUE that stores a null value.] */
		/* Tests_SRS_AMQPVALUE_01_003: [1.6.1 null Indicates an empty value.] */
		TEST_FUNCTION(amqpvalue_create_null_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_null_fails)
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
		TEST_FUNCTION(amqpvalue_create_boolean_true_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_boolean_false_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_boolean_fails)
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
		TEST_FUNCTION(amqpvalue_get_boolean_true_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_boolean_false_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_boolean_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_boolean_with_a_NULL_bool_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_boolean_with_an_amqpvalue_that_is_not_boolean_fails)
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
		TEST_FUNCTION(amqpvalue_create_ubyte_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_ubyte_255_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_ubyte_fails)
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
		TEST_FUNCTION(amqpvalue_get_ubyte_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_ubyte_255_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_ubyte_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_ubyte_with_a_NULL_ubyte_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_ubyte_with_an_amqpvalue_that_is_not_ubyte_fails)
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
		TEST_FUNCTION(amqpvalue_create_ushort_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_ushort_65535_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_ushort_fails)
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
		TEST_FUNCTION(amqpvalue_get_ushort_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_ushort_65535_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_ushort_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_ushort_with_a_NULL_ushort_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_ushort_with_an_amqpvalue_that_is_not_ushort_fails)
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
		TEST_FUNCTION(amqpvalue_create_uint_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_uint_0xFFFFFFFF_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_uint_fails)
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
		TEST_FUNCTION(amqpvalue_get_uint_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_uint_0xFFFFFFFF_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_uint_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_uint_with_a_NULL_uint_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_uint_with_an_amqpvalue_that_is_not_uint_fails)
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
		TEST_FUNCTION(amqpvalue_create_ulong_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_ulong_0xFFFFFFFFFFFFFFFF_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_ulong_fails)
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
		TEST_FUNCTION(amqpvalue_get_ulong_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_ulong_0xFFFFFFFFFFFFFFFF_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_ulong_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_ulong_with_a_NULL_ulong_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_ulong_with_an_amqpvalue_that_is_not_ulong_fails)
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
		TEST_FUNCTION(amqpvalue_create_byte_minus_128_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_byte_127_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_byte_fails)
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
		TEST_FUNCTION(amqpvalue_get_byte_minus_127_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_byte_127_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_byte_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_byte_with_a_NULL_byte_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_byte_with_an_amqpvalue_that_is_not_byte_fails)
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
		TEST_FUNCTION(amqpvalue_create_short_minus_32768_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_short_32767_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_short_fails)
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
		TEST_FUNCTION(amqpvalue_get_short_minus_32768_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_short_32767_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_short_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_short_with_a_NULL_short_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_short_with_an_amqpvalue_that_is_not_short_fails)
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
		TEST_FUNCTION(amqpvalue_create_int_minus_2147483648_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_int_2147483647_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_int_fails)
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
		TEST_FUNCTION(amqpvalue_get_int_minus_2147483648_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_int_2147483647_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_int_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_int_with_a_NULL_int_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_int_with_an_amqpvalue_that_is_not_int_fails)
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
		TEST_FUNCTION(amqpvalue_create_long_minus_9223372036854775808_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_long_9223372036854775807_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_long_fails)
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
		TEST_FUNCTION(amqpvalue_get_long_minus_9223372036854775808_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_long_9223372036854775807_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_long_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_long_with_a_NULL_long_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_long_with_an_amqpvalue_that_is_not_long_fails)
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
		TEST_FUNCTION(amqpvalue_create_float_minus_one_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_float_42_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_float_fails)
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
		TEST_FUNCTION(amqpvalue_get_float_minus_one_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_float_42_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_float_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_float_with_a_NULL_float_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_float_with_an_amqpvalue_that_is_not_float_fails)
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
		TEST_FUNCTION(amqpvalue_create_double_minus_one_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_double_42_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_double_fails)
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
		TEST_FUNCTION(amqpvalue_get_double_minus_one_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_double_42_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_double_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_double_with_a_NULL_double_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_double_with_an_amqpvalue_that_is_not_double_fails)
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
		TEST_FUNCTION(amqpvalue_create_char_0x00_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_char_0x10FFFF_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_char_0x110000_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			AMQP_VALUE result = amqpvalue_create_char(0x110000);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_093: [If allocating the AMQP_VALUE fails then amqpvalue_create_char shall return NULL.] */
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_char_fails)
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
		TEST_FUNCTION(amqpvalue_get_char_0x0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_char_0x10FFFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			uint32_t char_value;
			AMQP_VALUE value = amqpvalue_create_char(0x10FFFF);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_get_char(value, &char_value);

			// assert
			ASSERT_ARE_EQUAL(uint32_t, 0x10FFFF, char_value);
			ASSERT_ARE_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_096: [If any of the arguments is NULL then amqpvalue_get_char shall return a non-zero value.] */
		TEST_FUNCTION(amqpvalue_get_char_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_char_with_a_NULL_char_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_char_with_an_amqpvalue_that_is_not_char_fails)
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
		TEST_FUNCTION(amqpvalue_create_timestamp_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_timestamp_1311704463521_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_timestamp_fails)
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
		TEST_FUNCTION(amqpvalue_get_timestamp_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_timestamp_1311704463521_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_timestamp_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_timestamp_with_a_NULL_timestamp_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_timestamp_with_an_amqpvalue_that_is_not_timestamp_fails)
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
		TEST_FUNCTION(amqpvalue_create_uuid_all_zeroes_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_uuid_all_0xFF_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_uuid_fails)
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
		TEST_FUNCTION(amqpvalue_get_uuid_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_uuid_1311704463521_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_uuid_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_uuid_with_a_NULL_uuid_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_uuid_with_an_amqpvalue_that_is_not_uuid_fails)
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
		TEST_FUNCTION(amqpvalue_create_binary_with_1_byte_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_binary_with_0_bytes_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_binary_with_2_bytes_succeeds)
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
		TEST_FUNCTION(when_allocating_the_amqp_value_fails_then_amqpvalue_create_binary_fails)
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
		TEST_FUNCTION(when_allocating_the_binary_buffer_fails_then_amqpvalue_create_binary_fails)
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
		TEST_FUNCTION(when_length_is_positive_and_buffer_is_NULL_then_amqpvalue_create_binary_fails)
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
		TEST_FUNCTION(amqpvalue_get_binary_1_byte_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_binary_0_byte_succeeds)
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
		TEST_FUNCTION(when_the_value_argument_is_NULL_amqpvalue_get_binary_fails)
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
		TEST_FUNCTION(when_the_binary_value_argument_is_NULL_amqpvalue_get_binary_fails)
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
		TEST_FUNCTION(amqpvalue_get_binary_on_a_null_amqp_value_fails)
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
		TEST_FUNCTION(amqpvalue_create_string_with_one_char_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_string_with_0_length_succeeds)
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
		TEST_FUNCTION(when_allocating_the_amqp_value_fails_then_amqpvalue_create_string_fails)
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
		TEST_FUNCTION(when_allocating_the_string_in_the_amqp_value_fails_then_amqpvalue_create_string_fails)
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
		TEST_FUNCTION(amqpvalue_get_string_1_byte_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_string_0_length_succeeds)
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
		TEST_FUNCTION(when_the_value_argument_is_NULL_amqpvalue_get_string_fails)
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
		TEST_FUNCTION(when_the_string_value_argument_is_NULL_amqpvalue_get_string_fails)
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
		TEST_FUNCTION(amqpvalue_get_string_on_a_null_amqp_value_fails)
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
		TEST_FUNCTION(amqpvalue_create_symbol_with_one_char_succeeds)
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
		TEST_FUNCTION(amqpvalue_create_symbol_value_0xFFFFFFFF_length_succeeds)
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
		TEST_FUNCTION(when_allocating_the_amqp_value_fails_then_amqpvalue_create_symbol_fails)
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
		TEST_FUNCTION(amqpvalue_get_symbol_0_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_symbol_0xFFFFFFFF_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_symbol_with_a_NULL_amqpvalue_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_symbol_with_a_NULL_symbol_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_symbol_with_an_amqpvalue_that_is_not_symbol_fails)
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
		TEST_FUNCTION(amqpvalue_create_list_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_fails_then_amqpvalue_create_list_fails)
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
		TEST_FUNCTION(amqpvalue_set_list_item_count_with_1_count_succeeds)
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
		TEST_FUNCTION(amqpvalue_set_list_item_count_with_NULL_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			int result = amqpvalue_set_list_item_count(NULL, 1);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_154: [If allocating memory for the list according to the new size fails, then amqpvalue_set_list_item_count shall return a non-zero value, while preserving the existing list contents.] */
		TEST_FUNCTION(when_reallocating_fails_amqpvalue_set_list_item_count_fails)
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
		TEST_FUNCTION(amqpvalue_set_list_item_count_with_a_non_list_type_fails)
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
		TEST_FUNCTION(amqpvalue_set_list_item_count_after_amqpvalue_set_list_item_count_succeeds)
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
		TEST_FUNCTION(when_allocating_the_new_null_element_fails_amqpvalue_set_list_item_count_fails)
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
		TEST_FUNCTION(when_allocating_the_new_null_element_fails_other_newly_allocated_items_are_rolled_back)
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
		TEST_FUNCTION(amqpvalue_set_list_item_count_with_0_count_does_not_allocate_anything)
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
		TEST_FUNCTION(shrinking_a_list_by_1_frees_the_extra_value_but_does_not_resize)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_yields_0_on_an_empty_list)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_on_a_list_with_size_1_yields_1)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_on_a_list_with_size_2_yields_2)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_on_a_list_shrunk_to_1_item_yields_1)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_on_a_list_shrunk_to_empty_yields_0)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_with_NULL_handle_fails)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_with_NULL_item_count_fails)
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
		TEST_FUNCTION(amqpvalue_get_list_item_count_on_a_non_list_type_fails)
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
		TEST_FUNCTION(amqpvalue_set_list_item_on_an_empty_list_succeeds)
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
		TEST_FUNCTION(amqpvalue_set_list_item_on_the_2nd_position_in_an_empty_list_succeeds)
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
		TEST_FUNCTION(when_cloning_the_item_fails_amqpvalue_set_list_item_fails_too)
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
		TEST_FUNCTION(when_allocating_the_filler_value_fails_amqpvalue_set_list_item_fails_too)
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
		TEST_FUNCTION(when_allocating_the_filler_value_fails_amqpvalue_set_list_item_fails_too_and_frees_previous_filler_values)
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
		TEST_FUNCTION(when_reallocating_the_list_fails_amqpvalue_set_list_item_fails_too)
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
		TEST_FUNCTION(amqpvalue_set_list_item_without_resizing_the_list_frees_the_previous_item)
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
		TEST_FUNCTION(amqpvalue_set_list_item_with_NULL_handle_fails)
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
		TEST_FUNCTION(amqpvalue_set_list_item_with_NULL_item_fails)
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
		TEST_FUNCTION(when_cloning_fails_amqpvalue_set_list_item_is_not_altered)
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
		TEST_FUNCTION(when_growing_fails_amqpvalue_set_list_item_is_not_altered)
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
		TEST_FUNCTION(amqpvalue_get_list_item_gets_the_first_item)
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
		TEST_FUNCTION(amqpvalue_get_list_item_gets_the_second_item)
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
		TEST_FUNCTION(when_list_handle_is_null_amqpvalue_get_list_item_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			AMQP_VALUE result = amqpvalue_get_list_item(NULL, 0);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_175: [If index is greater or equal to the number of items in the list then amqpvalue_get_list_item shall fail and return NULL.] */
		TEST_FUNCTION(amqpvalue_get_list_item_with_index_too_high_fails)
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
		TEST_FUNCTION(amqpvalue_get_list_item_with_index_0_on_an_empty_list_fails)
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
		TEST_FUNCTION(when_cloning_the_item_fails_then_amqpvalue_get_list_item_fails)
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
		TEST_FUNCTION(amqpvalue_get_list_item_called_with_a_non_list_handle_fails)
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
		TEST_FUNCTION(when_underlying_calls_succeed_amqpvalue_create_map_succeeds)
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
		TEST_FUNCTION(when_allocating_memory_for_the_map_fails_amqpvalue_create_map_fails)
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
		TEST_FUNCTION(amqpvalue_create_map_creates_a_map_with_no_pairs)
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
		TEST_FUNCTION(amqpvalue_set_map_value_adds_one_key_value_pair)
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
		TEST_FUNCTION(amqpvalue_set_map_value_adds_2_key_value_pairs)
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
		TEST_FUNCTION(amqpvalue_set_map_value_with_NULL_map_fails)
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
		TEST_FUNCTION(amqpvalue_set_map_value_with_NULL_key_fails)
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
		TEST_FUNCTION(amqpvalue_set_map_value_with_NULL_value_fails)
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
        /* Tests_SRS_AMQPVALUE_01_125: [A map in which there exist two identical key values is invalid.] */
		TEST_FUNCTION(amqpvalue_set_map_value_with_an_already_existing_value_replaces_the_old_value)
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
		TEST_FUNCTION(when_reallocating_memory_to_hold_the_map_fails_then_amqpvalue_set_map_value_fails)
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
		TEST_FUNCTION(when_cloning_the_value_fails_then_amqpvalue_set_map_value_fails)
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
		TEST_FUNCTION(when_cloning_the_key_fails_then_amqpvalue_set_map_value_fails)
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
		TEST_FUNCTION(amqpvalue_set_map_value_on_a_non_map_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_value_on_a_map_with_one_pair_returns_the_value_for_the_key)
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
		TEST_FUNCTION(amqpvalue_get_map_value_find_second_key_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_map_value_with_NULL_map_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_value_with_NULL_key_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_value_with_a_key_that_does_not_exist_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_value_for_a_non_map_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_pair_count_yields_0_on_an_empty_map)
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
		TEST_FUNCTION(amqpvalue_get_map_pair_count_yields_1_on_a_map_with_1_pair)
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
		TEST_FUNCTION(amqpvalue_get_map_pair_count_yields_2_on_a_map_with_2_pairs)
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
		TEST_FUNCTION(amqpvalue_get_map_pair_with_NULL_map_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_pair_with_NULL_pair_count_argument_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_pair_count_on_a_non_map_value_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_with_1_element_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_with_1_element_different_key_and_value_data_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_second_element_succeeds)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_with_NULL_map_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_with_NULL_key_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_with_NULL_value_fails)
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
		TEST_FUNCTION(when_cloning_key_fails_then_amqpvalue_get_map_key_value_pair_fails)
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
		TEST_FUNCTION(when_cloning_value_fails_then_amqpvalue_get_map_key_value_pair_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_on_an_empty_map_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_with_index_equal_number_of_pairs_fails)
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
		TEST_FUNCTION(amqpvalue_get_map_key_value_pair_on_a_non_map_value_fails)
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
		TEST_FUNCTION(amqpvalue_are_equal_with_NULL_values_returns_true)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			bool result = amqpvalue_are_equal(NULL, NULL);

			// assert
			ASSERT_IS_TRUE(result);
		}

		/* Tests_SRS_AMQPVALUE_01_208: [If one of the arguments is NULL and the other is not, amqpvalue_are_equal shall return false.] */
		TEST_FUNCTION(when_value2_is_NULL_and_value1_is_not_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(when_value1_is_NULL_and_value2_is_not_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(when_value1_is_uint_and_value2_is_ulong_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_null_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_equal_boolean_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_boolean_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_ubyte_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_ubyte_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_ushort_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_ushort_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_uint_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_uint_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_ulong_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_ulong_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_byte_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_byte_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_short_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_short_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_int_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_int_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_long_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_long_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_float_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_float_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_double_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_double_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_char_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_char_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_timestamp_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_timestamp_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_uuid_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_uuid_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_binary_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_binary_values_with_same_length_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_different_binary_values_with_different_length_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_equal_string_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_different_string_values_with_same_length_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_different_string_values_with_different_length_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_empty_list_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_lists_with_one_null_item_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_lists_with_different_number_of_null_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_lists_one_empty_and_one_with_a_value_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_lists_with_one_identical_int_value_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_lists_with_2_different_int_values_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_lists_with_different_int_values_at_index_1_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_lists_each_with_one_empty_list_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(when_inner_lists_have_different_item_count_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(when_inner_lists_have_each_1_item_count_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(when_inner_lists_have_each_1_item_count_but_items_are_different_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_empty_map_values_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_maps_with_one_null_key_and_null_value_item_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_maps_with_one_pair_each_where_key_is_different_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_maps_with_one_pair_each_where_value_is_different_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_maps_with_one_pair_each_where_key_and_value_are_equal_amqpvalue_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_maps_with_different_pair_count_amqpvalue_are_equal_returns_false)
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
		TEST_FUNCTION(for_2_maps_with_2_equal_pairs_are_equal_returns_true)
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
		TEST_FUNCTION(for_2_maps_with_2_equal_pairs_out_of_order_are_equal_returns_false)
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
		TEST_FUNCTION(when_inner_maps_are_equal_are_equal_returns_true)
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
		TEST_FUNCTION(when_inner_maps_are_equal_not_are_not_equal_returns_false)
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

		/* amqpvalue_clone */

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_237: [null] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_null_succesfully)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_237: [null] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_null_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_238: [boolean] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_boolean_succesfully_false_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(false);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_238: [boolean] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_boolean_succesfully_true_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(true);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_238: [boolean] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_boolean_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(true);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_239: [ubyte] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_ubyte_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_239: [ubyte] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_ubyte_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_239: [ubyte] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_ubyte_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_240: [ushort] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_ushort_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_240: [ushort] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_ushort_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_240: [ushort] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_ushort_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_241: [uint] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_uint_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_241: [uint] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_uint_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_241: [uint] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_uint_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_242: [ulong] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_ulong_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_242: [ulong] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_ulong_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_242: [ulong] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_ulong_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_243: [byte] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_byte_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_243: [byte] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_byte_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_243: [byte] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_byte_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_244: [short] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_short_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_244: [short] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_short_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_244: [short] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_short_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_245: [int] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_int_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_245: [int] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_int_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_245: [int] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_int_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_246: [long] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_long_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_246: [long] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_long_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_246: [long] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_long_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_247: [float] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_float_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_float(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_247: [float] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_float_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_float(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_247: [float] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_float_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_float(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_248: [double] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_double_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_double(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_248: [double] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_double_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_double(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_248: [double] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_double_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_double(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_252: [char] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_char_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_char(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_252: [char] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_char_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_char(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_252: [char] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_char_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_char(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_253: [timestamp] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_timestamp_succesfully_value_42)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(42);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_253: [timestamp] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_timestamp_succesfully_value_43)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_253: [timestamp] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_timestamp_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(43);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_254: [uuid] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_uuid_succesfully_first_byte_non_zero)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid_value = { 0x42 };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_254: [uuid] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_uuid_succesfully_2_non_zero_bytes)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid_value = { 0x42, 0x43 };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_254: [uuid] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_uuid_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid_value = { 0x42, 0x43 };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_255: [binary] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_binary_succesfully_1_byte)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char buffer[] = { 0x42 };
			amqp_binary binary_value = { buffer, sizeof(buffer) };
			AMQP_VALUE source = amqpvalue_create_binary(binary_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(sizeof(buffer)));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_255: [binary] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_binary_succesfully_2_bytes)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char buffer[] = { 0x42, 0x43 };
			amqp_binary binary_value = { buffer, sizeof(buffer) };
			AMQP_VALUE source = amqpvalue_create_binary(binary_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(sizeof(buffer)));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_255: [binary] */
		TEST_FUNCTION(when_allocating_cloned_value_fails_amqpvalue_clone_for_a_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char buffer[] = { 0x42, 0x43 };
			amqp_binary binary_value = { buffer, sizeof(buffer) };
			AMQP_VALUE source = amqpvalue_create_binary(binary_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_255: [binary] */
		TEST_FUNCTION(when_allocating_the_underlying_buffer_amqpvalue_clone_for_a_binary_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char buffer[] = { 0x42, 0x43 };
			amqp_binary binary_value = { buffer, sizeof(buffer) };
			AMQP_VALUE source = amqpvalue_create_binary(binary_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(sizeof(buffer)))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_256: [string] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_string_succesfully_a)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("a");
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_256: [string] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_string_succesfully_abcd)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("abcd");
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_255: [string] */
		TEST_FUNCTION(when_allocating_the_cloned_value_amqpvalue_clone_for_a_string_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("abcd");
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_255: [string] */
		TEST_FUNCTION(when_allocating_the_underlying_buffer_amqpvalue_clone_for_a_string_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("abcd");
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(amqpvalue_clone_clones_an_empty_list)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_list_with_one_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE item = amqpvalue_create_uint(42);
			AMQP_VALUE source = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(source, 0, item);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(item);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_list_with_2_items)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE item1 = amqpvalue_create_uint(42);
			AMQP_VALUE item2 = amqpvalue_create_uint(43);
			AMQP_VALUE source = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(source, 0, item1);
			(void)amqpvalue_set_list_item(source, 1, item2);
			mocks.ResetAllCalls();

			/* 4 = 1 for source, 1 for the array of items + 2 items */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.ExpectedTimesExactly(4);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(item1);
			amqpvalue_destroy(item2);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(when_allocating_the_cloned_value_amqpvalue_clone_for_a_list_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE item1 = amqpvalue_create_uint(42);
			AMQP_VALUE item2 = amqpvalue_create_uint(43);
			AMQP_VALUE source = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(source, 0, item1);
			(void)amqpvalue_set_list_item(source, 1, item2);
			mocks.ResetAllCalls();

			/* 4 = 1 for source, 1 for the array of items + 2 items */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(item1);
			amqpvalue_destroy(item2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(when_allocating_the_underlying_list_amqpvalue_clone_for_a_list_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE item1 = amqpvalue_create_uint(42);
			AMQP_VALUE item2 = amqpvalue_create_uint(43);
			AMQP_VALUE source = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(source, 0, item1);
			(void)amqpvalue_set_list_item(source, 1, item2);
			mocks.ResetAllCalls();

			/* 4 = 1 for source, 1 for the array of items + 2 items */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(item1);
			amqpvalue_destroy(item2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(when_allocating_the_first_cloned_value_amqpvalue_clone_for_a_list_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE item1 = amqpvalue_create_uint(42);
			AMQP_VALUE item2 = amqpvalue_create_uint(43);
			AMQP_VALUE source = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(source, 0, item1);
			(void)amqpvalue_set_list_item(source, 1, item2);
			mocks.ResetAllCalls();

			/* 4 = 1 for source, 1 for the array of items + 2 items */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(item1);
			amqpvalue_destroy(item2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_258: [list] */
		TEST_FUNCTION(when_allocating_the_second_cloned_value_amqpvalue_clone_for_a_list_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE item1 = amqpvalue_create_uint(42);
			AMQP_VALUE item2 = amqpvalue_create_uint(43);
			AMQP_VALUE source = amqpvalue_create_list();
			(void)amqpvalue_set_list_item(source, 0, item1);
			(void)amqpvalue_set_list_item(source, 1, item2);
			mocks.ResetAllCalls();

			/* 4 = 1 for source, 1 for the array of items + 2 items */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(item1);
			amqpvalue_destroy(item2);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(amqpvalue_clone_clones_an_empty_map)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_map_with_one_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key = amqpvalue_create_uint(42);
			AMQP_VALUE value = amqpvalue_create_uint(43);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key, value);
			mocks.ResetAllCalls();

			/* the cloned map */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			/* the cloned map array */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			/* the cloned key */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			/* the cloned value */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key);
			amqpvalue_destroy(value);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_235: [amqpvalue_clone shall clone the value passed as argument and return a new non-NULL handle to the cloned AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(amqpvalue_clone_clones_a_map_with_2_items)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			/* 4 = 1 for source, 1 for the array of items + 4 items */
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.ExpectedTimesExactly(6);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NOT_NULL(result);
			ASSERT_IS_TRUE(amqpvalue_are_equal(result, source));
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
			amqpvalue_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(when_allocating_the_cloned_value_fails_amqpvalue_clone_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(when_allocating_the_underlying_map_array_fails_amqpvalue_clone_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(when_allocating_the_first_cloned_key_fails_amqpvalue_clone_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(when_allocating_the_first_cloned_value_fails_amqpvalue_clone_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(when_allocating_the_second_cloned_key_fails_amqpvalue_clone_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
		}

		/* Tests_SRS_AMQPVALUE_01_236: [If creating the cloned value fails, amqpvalue_clone shall return NULL.] */
		/* Tests_SRS_AMQPVALUE_01_259: [map] */
		TEST_FUNCTION(when_allocating_the_second_cloned_value_fails_amqpvalue_clone_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE key1 = amqpvalue_create_uint(42);
			AMQP_VALUE value1 = amqpvalue_create_uint(43);
			AMQP_VALUE key2 = amqpvalue_create_uint(44);
			AMQP_VALUE value2 = amqpvalue_create_uint(45);
			AMQP_VALUE source = amqpvalue_create_map();
			(void)amqpvalue_set_map_value(source, key1, value1);
			(void)amqpvalue_set_map_value(source, key2, value2);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQP_VALUE result = amqpvalue_clone(source);

			// assert
			ASSERT_IS_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(value1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(value2);
		}

		/* amqpvalue_encode */

		static void test_amqpvalue_encode(amqpvalue_mocks* mocks, AMQP_VALUE source, const char* expected_stringified_bytes)
		{
			// arrange
			mocks->ResetAllCalls();

			EXPECTED_CALL((*mocks), test_encoder_output(NULL, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.ExpectedAtLeastTimes(1);
			EXPECTED_CALL((*mocks), test_encoder_output(NULL, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.IgnoreAllCalls();

			// act
			int result = amqpvalue_encode(source, test_encoder_output, NULL);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			stringify_bytes(encoded_bytes, encoded_byte_count, actual_stringified_encoded);
			ASSERT_ARE_EQUAL(char_ptr, expected_stringified_bytes, actual_stringified_encoded);
			mocks->AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		static void test_amqpvalue_encode_failure(amqpvalue_mocks* mocks, AMQP_VALUE source)
		{
			(void)amqpvalue_encode(source, test_encoder_output, NULL);
			mocks->ResetAllCalls();

			int encoder_calls = encoder_output_call_count;
			int i;
			for (i = 0; i < 1; i++)
			{
				mocks->ResetAllCalls();

				encoder_output_call_count = 0;
				when_shall_encoder_output_fail = i + 1;
				EXPECTED_CALL((*mocks), test_encoder_output(NULL, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
					.ValidateArgument(1).ExpectedTimesExactly(i + 1);

				// act
				int result = amqpvalue_encode(source, test_encoder_output, NULL);

				// assert
				ASSERT_ARE_NOT_EQUAL(int, 0, result);
				mocks->AssertActualAndExpectedCalls();
			}

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_265: [amqpvalue_encode shall encode the value per the ISO.] */
		/* Tests_SRS_AMQPVALUE_01_266: [On success amqpvalue_encode shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_267: [amqpvalue_encode shall pass the encoded bytes to the encoder_output function.] */
		/* Tests_SRS_AMQPVALUE_01_268: [On each call to the encoder_output function, amqpvalue_encode shall also pass the context argument.] */
		/* Tests_SRS_AMQPVALUE_01_264: [<encoding code="0x40" category="fixed" width="0" label="the null value"/>] */
		TEST_FUNCTION(amqpvalue_encode_for_a_null_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, test_encoder_output(test_context, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.ValidateArgument(1).ExpectedAtLeastTimes(1);

			// act
			int result = amqpvalue_encode(source, test_encoder_output, test_context);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			stringify_bytes(encoded_bytes, encoded_byte_count, actual_stringified_encoded);
			ASSERT_ARE_EQUAL(char_ptr, "[0x40]", actual_stringified_encoded);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_265: [amqpvalue_encode shall encode the value per the ISO.] */
		/* Tests_SRS_AMQPVALUE_01_266: [On success amqpvalue_encode shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_267: [amqpvalue_encode shall pass the encoded bytes to the encoder_output function.] */
		/* Tests_SRS_AMQPVALUE_01_268: [On each call to the encoder_output function, amqpvalue_encode shall also pass the context argument.] */
		/* Tests_SRS_AMQPVALUE_01_264: [<encoding code="0x40" category="fixed" width="0" label="the null value"/>] */
		TEST_FUNCTION(amqpvalue_encode_with_NULL_context_is_allowed)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_null();
			test_amqpvalue_encode(&mocks, source, "[0x40]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, test_encoder_output(NULL, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
				.ValidateArgument(1).ExpectedAtLeastTimes(1).SetReturn(1);

			// act
			int result = amqpvalue_encode(source, test_encoder_output, NULL);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_269: [If value or encoder_output are NULL, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(amqpvalue_encode_with_NULL_value_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			int result = amqpvalue_encode(NULL, test_encoder_output, test_context);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_269: [If value or encoder_output are NULL, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(amqpvalue_encode_with_NULL_encoder_output_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_null();
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_encode(source, NULL, test_context);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

		/* Tests_SRS_AMQPVALUE_01_272: [<encoding name="true" code="0x41" category="fixed" width="0" label="the boolean value true"/>] */
		TEST_FUNCTION(amqpvalue_encode_boolean_true_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(true);
			test_amqpvalue_encode(&mocks, source, "[0x41]");
		}

		/* Tests_SRS_AMQPVALUE_01_273: [<encoding name="false" code="0x42" category="fixed" width="0" label="the boolean value false"/>] */
		TEST_FUNCTION(amqpvalue_encode_boolean_false_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(false);
			test_amqpvalue_encode(&mocks, source, "[0x42]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_boolean_false_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(false);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_boolean_true_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(true);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_275: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_ubyte_0x00_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(0x0);
			test_amqpvalue_encode(&mocks, source, "[0x50,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_275: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_ubyte_0xFF_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(0xFF);
			test_amqpvalue_encode(&mocks, source, "[0x50,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_ubyte_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(0xFF);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_276: [<encoding code="0x60" category="fixed" width="2" label="16-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_ushort_0x0_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(0x0);
			test_amqpvalue_encode(&mocks, source, "[0x60,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_276: [<encoding code="0x60" category="fixed" width="2" label="16-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_ushort_0x04243_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(0x4243);
			test_amqpvalue_encode(&mocks, source, "[0x60,0x42,0x43]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_ushort_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(0x4243);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_277: [<encoding code="0x70" category="fixed" width="4" label="32-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_uint_0xFFFFFFFF_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0xFFFFFFFF);
			test_amqpvalue_encode(&mocks, source, "[0x70,0xFF,0xFF,0xFF,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_277: [<encoding code="0x70" category="fixed" width="4" label="32-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_uint_0x042434445_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0x42434445);
			test_amqpvalue_encode(&mocks, source, "[0x70,0x42,0x43,0x44,0x45]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_uint_0x42434445_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0x42434445);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_278: [<encoding name="smalluint" code="0x52" category="fixed" width="1" label="unsigned integer value in the range 0 to 255 inclusive"/>] */
		TEST_FUNCTION(amqpvalue_encode_uint_0x42_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0x42);
			test_amqpvalue_encode(&mocks, source, "[0x52,0x42]");
		}

		/* Tests_SRS_AMQPVALUE_01_278: [<encoding name="smalluint" code="0x52" category="fixed" width="1" label="unsigned integer value in the range 0 to 255 inclusive"/>] */
		TEST_FUNCTION(amqpvalue_encode_uint_0xFF_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0xFF);
			test_amqpvalue_encode(&mocks, source, "[0x52,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_uint_0xFF_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0xFF);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_279: [<encoding name="uint0" code="0x43" category="fixed" width="0" label="the uint value 0"/>] */
		TEST_FUNCTION(amqpvalue_encode_uint_0x00_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0x00);
			test_amqpvalue_encode(&mocks, source, "[0x43]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_uint_0x00_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0x00);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_280: [<encoding code="0x80" category="fixed" width="8" label="64-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_ulong_0x4243444546474849_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0x4243444546474849);
			test_amqpvalue_encode(&mocks, source, "[0x80,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49]");
		}

		/* Tests_SRS_AMQPVALUE_01_280: [<encoding code="0x80" category="fixed" width="8" label="64-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_ulong_0xFFFFFFFFFFFFFFFF_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0xFFFFFFFFFFFFFFFF);
			test_amqpvalue_encode(&mocks, source, "[0x80,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_ulong_0xFFFFFFFFFFFFFFFF_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0xFFFFFFFFFFFFFFFF);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_281: [<encoding name="smallulong" code="0x53" category="fixed" width="1" label="unsigned long value in the range 0 to 255 inclusive"/>] */
		TEST_FUNCTION(amqpvalue_encode_ulong_0x42_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0x42);
			test_amqpvalue_encode(&mocks, source, "[0x53,0x42]");
		}

		/* Tests_SRS_AMQPVALUE_01_281: [<encoding name="smallulong" code="0x53" category="fixed" width="1" label="unsigned long value in the range 0 to 255 inclusive"/>] */
		TEST_FUNCTION(amqpvalue_encode_ulong_0xFF_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0xFF);
			test_amqpvalue_encode(&mocks, source, "[0x53,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_ulong_0xFF_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0xFF);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_282: [<encoding name="ulong0" code="0x44" category="fixed" width="0" label="the ulong value 0"/>] */
		TEST_FUNCTION(amqpvalue_encode_ulong_0x00_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0x00);
			test_amqpvalue_encode(&mocks, source, "[0x44]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_ulong_0x00_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0x00);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_283: [<encoding code="0x51" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_byte_minus128_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(-128);
			test_amqpvalue_encode(&mocks, source, "[0x51,0x80]");
		}

		/* Tests_SRS_AMQPVALUE_01_283: [<encoding code="0x51" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_byte_0_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(0);
			test_amqpvalue_encode(&mocks, source, "[0x51,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_283: [<encoding code="0x51" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_byte_127_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(127);
			test_amqpvalue_encode(&mocks, source, "[0x51,0x7F]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_byte_127_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(127);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_284: [<encoding code="0x61" category="fixed" width="2" label="16-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_short_minus32768_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(-32768);
			test_amqpvalue_encode(&mocks, source, "[0x61,0x80,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_284: [<encoding code="0x61" category="fixed" width="2" label="16-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_short_0_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(0);
			test_amqpvalue_encode(&mocks, source, "[0x61,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_284: [<encoding code="0x61" category="fixed" width="2" label="16-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_short_32767_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(32767);
			test_amqpvalue_encode(&mocks, source, "[0x61,0x7F,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_short_32767_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(32767);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_285: [<encoding code="0x71" category="fixed" width="4" label="32-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_int_minus2147483648_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(-2147483647-1);
			test_amqpvalue_encode(&mocks, source, "[0x71,0x80,0x00,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_285: [<encoding code="0x71" category="fixed" width="4" label="32-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_int_0x42434445_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(0x42434445);
			test_amqpvalue_encode(&mocks, source, "[0x71,0x42,0x43,0x44,0x45]");
		}

		/* Tests_SRS_AMQPVALUE_01_285: [<encoding code="0x71" category="fixed" width="4" label="32-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_int_2147483647_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(2147483647);
			test_amqpvalue_encode(&mocks, source, "[0x71,0x7F,0xFF,0xFF,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_int_2147483647_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(2147483647);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_286: [<encoding name="smallint" code="0x54" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_int_minus128_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(-128);
			test_amqpvalue_encode(&mocks, source, "[0x54,0x80]");
		}

		/* Tests_SRS_AMQPVALUE_01_286: [<encoding name="smallint" code="0x54" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_int_0_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(0);
			test_amqpvalue_encode(&mocks, source, "[0x54,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_286: [<encoding name="smallint" code="0x54" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_encode_int_127_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(127);
			test_amqpvalue_encode(&mocks, source, "[0x54,0x7F]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_int_127_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(127);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_287: [<encoding code="0x81" category="fixed" width="8" label="64-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_long_minus9223372036854775808_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(-9223372036854775807L - 1);
			test_amqpvalue_encode(&mocks, source, "[0x81,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_287: [<encoding code="0x81" category="fixed" width="8" label="64-bit two's-complement longeger in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_long_0x4243444546474849_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(0x4243444546474849L);
			test_amqpvalue_encode(&mocks, source, "[0x81,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49]");
		}

		/* Tests_SRS_AMQPVALUE_01_287: [<encoding code="0x81" category="fixed" width="8" label="64-bit two's-complement longeger in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_encode_long_9223372036854775807_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(9223372036854775807L);
			test_amqpvalue_encode(&mocks, source, "[0x81,0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_long_9223372036854775807_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(9223372036854775807L);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_288: [<encoding name="smalllong" code="0x55" category="fixed" width="1" label="8-bit two's-complement longeger"/>] */
		TEST_FUNCTION(amqpvalue_encode_long_minus128_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(-128);
			test_amqpvalue_encode(&mocks, source, "[0x55,0x80]");
		}

		/* Tests_SRS_AMQPVALUE_01_288: [<encoding name="smalllong" code="0x55" category="fixed" width="1" label="8-bit two's-complement longeger"/>] */
		TEST_FUNCTION(amqpvalue_encode_long_0_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(0);
			test_amqpvalue_encode(&mocks, source, "[0x55,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_288: [<encoding name="smalllong" code="0x55" category="fixed" width="1" label="8-bit two's-complement longeger"/>] */
		TEST_FUNCTION(amqpvalue_encode_long_127_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(127);
			test_amqpvalue_encode(&mocks, source, "[0x55,0x7F]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_long_127_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(127);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_295: [<encoding name="ms64" code="0x83" category="fixed" width="8" label="64-bit two's-complement integer representing milliseconds since the unix epoch"/>] */
		TEST_FUNCTION(amqpvalue_encode_timestamp_minus9223372036854775808_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(-9223372036854775807L - 1);
			test_amqpvalue_encode(&mocks, source, "[0x83,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_295: [<encoding name="ms64" code="0x83" category="fixed" width="8" label="64-bit two's-complement integer representing milliseconds since the unix epoch"/>] */
		TEST_FUNCTION(amqpvalue_encode_timestamp_0_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(0);
			test_amqpvalue_encode(&mocks, source, "[0x83,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_295: [<encoding name="ms64" code="0x83" category="fixed" width="8" label="64-bit two's-complement integer representing milliseconds since the unix epoch"/>] */
		TEST_FUNCTION(amqpvalue_encode_timestamp_9223372036854775807_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(9223372036854775807L);
			test_amqpvalue_encode(&mocks, source, "[0x83,0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_timestamp_127_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(127);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_296: [<encoding code="0x98" category="fixed" width="16" label="UUID as defined in section 4.1.2 of RFC-4122"/>] */
		TEST_FUNCTION(amqpvalue_encode_uuid_all_zeroes_succeeds)
		{
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0 };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid);
			test_amqpvalue_encode(&mocks, source, "[0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_296: [<encoding code="0x98" category="fixed" width="16" label="UUID as defined in section 4.1.2 of RFC-4122"/>] */
		TEST_FUNCTION(amqpvalue_encode_uuid_succeeds)
		{
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
				0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid);
			test_amqpvalue_encode(&mocks, source, "[0x98,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_uuid_fails)
		{
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
				0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_297: [<encoding name="vbin8" code="0xa0" category="variable" width="1" label="up to 2^8 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_encode_binary_zero_bytes_succeeds)
		{
			amqpvalue_mocks mocks;
			unsigned char bytes[] = { 0x00 };
			amqp_binary binary = { &bytes, 0 };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_encode(&mocks, source, "[0xA0,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_297: [<encoding name="vbin8" code="0xa0" category="variable" width="1" label="up to 2^8 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_encode_binary_one_byte_succeeds)
		{
			amqpvalue_mocks mocks;
			unsigned char bytes[] = { 0x42 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_encode(&mocks, source, "[0xA0,0x01,0x42]");
		}

		/* Tests_SRS_AMQPVALUE_01_297: [<encoding name="vbin8" code="0xa0" category="variable" width="1" label="up to 2^8 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_encode_binary_255_bytes_succeeds)
		{
			amqpvalue_mocks mocks;
			unsigned char bytes[255];
			int i;
			for (i = 0; i < 255; i++)
			{
				bytes[i] = i;
			}
			unsigned char expected_bytes[257] = { 0xA0, 0xFF };
			for (i = 0; i < 255; i++)
			{
				expected_bytes[i + 2] = i;
			}
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_binary_255_bytes_fails)
		{
			amqpvalue_mocks mocks;
			unsigned char bytes[255];
			int i;
			for (i = 0; i < 255; i++)
			{
				bytes[i] = i;
			}
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_298: [<encoding name="vbin32" code="0xb0" category="variable" width="4" label="up to 2^32 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_encode_binary_256_bytes_succeeds)
		{
			amqpvalue_mocks mocks;
			unsigned char bytes[256];
			int i;
			for (i = 0; i < 256; i++)
			{
				bytes[i] = i;
			}
			unsigned char expected_bytes[261] = { 0xB0, 0x00, 0x00, 0x01, 0x00 };
			for (i = 0; i < 256; i++)
			{
				expected_bytes[i + 5] = i;
			}
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_binary_256_bytes_fails)
		{
			amqpvalue_mocks mocks;
			unsigned char bytes[256];
			int i;
			for (i = 0; i < 256; i++)
			{
				bytes[i] = i;
			}
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_299: [<encoding name="str8-utf8" code="0xa1" category="variable" width="1" label="up to 2^8 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_encode_string_with_empty_string_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("");
			test_amqpvalue_encode(&mocks, source, "[0xA1,0x00]");
		}

		/* Tests_SRS_AMQPVALUE_01_299: [<encoding name="str8-utf8" code="0xa1" category="variable" width="1" label="up to 2^8 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_encode_string_with_char_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("a");
			test_amqpvalue_encode(&mocks, source, "[0xA1,0x01,0x61]");
		}

		/* Tests_SRS_AMQPVALUE_01_299: [<encoding name="str8-utf8" code="0xa1" category="variable" width="1" label="up to 2^8 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_encode_string_with_255_chars_succeeds)
		{
			amqpvalue_mocks mocks;
			char chars[256];
			int i;
			for (i = 0; i < 255; i++)
			{
				chars[i] = 'a';
			}
			chars[255] = '\0';
			unsigned char expected_bytes[257] = { 0xA1, 0xFF };
			for (i = 0; i < 255; i++)
			{
				expected_bytes[i + 2] = 'a';
			}
			AMQP_VALUE source = amqpvalue_create_string(chars);
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_string_with_255_chars_fails)
		{
			amqpvalue_mocks mocks;
			char chars[256];
			int i;
			for (i = 0; i < 255; i++)
			{
				chars[i] = 'a';
			}
			chars[255] = '\0';
			AMQP_VALUE source = amqpvalue_create_string(chars);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_300: [<encoding name="str32-utf8" code="0xb1" category="variable" width="4" label="up to 2^32 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_encode_string_with_256_chars_succeeds)
		{
			amqpvalue_mocks mocks;
			char chars[257];
			int i;
			for (i = 0; i < 256; i++)
			{
				chars[i] = 'a';
			}
			chars[256] = '\0';
			unsigned char expected_bytes[261] = { 0xB1, 0x00, 0x00, 0x01, 0x00 };
			for (i = 0; i < 256; i++)
			{
				expected_bytes[i + 5] = 'a';
			}
			AMQP_VALUE source = amqpvalue_create_string(chars);
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_string_with_256_chars_fails)
		{
			amqpvalue_mocks mocks;
			char chars[257];
			int i;
			for (i = 0; i < 256; i++)
			{
				chars[i] = 'a';
			}
			chars[256] = '\0';
			AMQP_VALUE source = amqpvalue_create_string(chars);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_303: [<encoding name="list0" code="0x45" category="fixed" width="0" label="the empty list (i.e. the list with no elements)"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_empty_list_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			test_amqpvalue_encode(&mocks, source, "[0x45]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_list_empty_list_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_with_one_null_item_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			test_amqpvalue_encode(&mocks, source, "[0xC0,0x01,0x01,0x40]");
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_list_with_one_null_item_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_with_2_null_item_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_set_list_item(source, 1, item);
			amqpvalue_destroy(item);
			test_amqpvalue_encode(&mocks, source, "[0xC0,0x02,0x02,0x40,0x40]");
		}

		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_with_255_null_items_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			int i;
			for (i = 0; i < 255; i++)
			{
				amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			unsigned char expected_bytes[258] = { 0xC0, 0xFF, 0xFF };
			for (i = 0; i < 255; i++)
			{
				expected_bytes[i + 3] = 0x40;
			}
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_then_amqpvalue_encode_list_with_255_null_items_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			int i;
			for (i = 0; i < 255; i++)
			{
				amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_1_item_with_255_bytes_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			unsigned char bytes[253] = { 0 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE item = amqpvalue_create_binary(binary);
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			unsigned char expected_bytes[258] = { 0xC0, 0xFF, 0x01, 0xA0, 0xFD };
			int i;
			for (i = 0; i < 253; i++)
			{
				expected_bytes[i + 5] = 0;
			}
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_305: [<encoding name="list32" code="0xd0" category="compound" width="4" label="up to 2^32 - 1 list elements with total size less than 2^32 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_1_item_with_256_bytes_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			unsigned char bytes[254] = { 0 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE item = amqpvalue_create_binary(binary);
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			unsigned char expected_bytes[254 + 11] = { 0xD0, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0xA0, 0xFE };
			int i;
			for (i = 0; i < 254; i++)
			{
				expected_bytes[i + 11] = 0;
			}
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_list_1_item_with_256_bytes_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			unsigned char bytes[254] = { 0 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE item = amqpvalue_create_binary(binary);
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_305: [<encoding name="list32" code="0xd0" category="compound" width="4" label="up to 2^32 - 1 list elements with total size less than 2^32 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_256_null_items_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			int i;
			for (i = 0; i < 256; i++)
			{
				amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			unsigned char expected_bytes[256 + 9] = { 0xD0, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00 };
			for (i = 0; i < 256; i++)
			{
				expected_bytes[i + 9] = 0x40;
			}
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
		TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_list_256_null_items_fails)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			int i;
			for (i = 0; i < 256; i++)
			{
				amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			test_amqpvalue_encode_failure(&mocks, source);
		}

		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_encode_list_with_2_different_items_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			unsigned char bytes[] = { 0x42 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE item = amqpvalue_create_binary(binary);
			amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			item = amqpvalue_create_null();
			amqpvalue_set_list_item(source, 1, item);
			amqpvalue_destroy(item);
			unsigned char expected_bytes[] = { 0xC0, 0x04, 0x02, 0xA0, 0x01, 0x42, 0x40 };
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

		/* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
		TEST_FUNCTION(amqpvalue_encode_empty_map_succeeds)
		{
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			unsigned char expected_bytes[] = { 0xC1, 0x00, 0x00 };
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
		}

        /* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
        TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_empty_map_fails)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            test_amqpvalue_encode_failure(&mocks, source);
        }

        /* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
        /* Tests_SRS_AMQPVALUE_01_123: [A map is encoded as a compound value where the constituent elements form alternating key value pairs.] */
        /* Tests_SRS_AMQPVALUE_01_124: [Map encodings MUST contain an even number of items (i.e. an equal number of keys and values).] */
        TEST_FUNCTION(amqpvalue_encode_a_map_with_a_null_key_and_null_value_succeeds)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            AMQP_VALUE key = amqpvalue_create_null();
            AMQP_VALUE value = amqpvalue_create_null();
            amqpvalue_set_map_value(source, key, value);
            amqpvalue_destroy(key);
            amqpvalue_destroy(value);
            unsigned char expected_bytes[] = { 0xC1, 0x02, 0x02, 0x40, 0x40 };
            stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
            test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
        }

        /* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
        TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_a_map_with_a_null_key_and_null_value_fails)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            AMQP_VALUE key = amqpvalue_create_null();
            AMQP_VALUE value = amqpvalue_create_null();
            amqpvalue_set_map_value(source, key, value);
            amqpvalue_destroy(key);
            amqpvalue_destroy(value);
            test_amqpvalue_encode_failure(&mocks, source);
        }

        /* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
        /* Tests_SRS_AMQPVALUE_01_123: [A map is encoded as a compound value where the constituent elements form alternating key value pairs.] */
        /* Tests_SRS_AMQPVALUE_01_124: [Map encodings MUST contain an even number of items (i.e. an equal number of keys and values).] */
        TEST_FUNCTION(amqpvalue_encode_a_map_with_a_uint_key_and_uint_value_succeeds)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            AMQP_VALUE key = amqpvalue_create_uint(0x42);
            AMQP_VALUE value = amqpvalue_create_uint(0x43);
            amqpvalue_set_map_value(source, key, value);
            amqpvalue_destroy(key);
            amqpvalue_destroy(value);
            unsigned char expected_bytes[] = { 0xC1, 0x04, 0x02, 0x52, 0x42, 0x52, 0x43 };
            stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
            test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
        }

        /* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
        TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_a_map_with_a_uint_key_and_uint_value_fails)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            AMQP_VALUE key = amqpvalue_create_uint(0x42);
            AMQP_VALUE value = amqpvalue_create_uint(0x43);
            amqpvalue_set_map_value(source, key, value);
            amqpvalue_destroy(key);
            amqpvalue_destroy(value);
            test_amqpvalue_encode_failure(&mocks, source);
        }

        /* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
        /* Tests_SRS_AMQPVALUE_01_123: [A map is encoded as a compound value where the constituent elements form alternating key value pairs.] */
        /* Tests_SRS_AMQPVALUE_01_124: [Map encodings MUST contain an even number of items (i.e. an equal number of keys and values).] */
        TEST_FUNCTION(amqpvalue_encode_85_key_value_pairs_succeeds)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            int i;
            for (i = 0; i < 85; i++)
            {
                AMQP_VALUE key = amqpvalue_create_uint(i + 1);
                AMQP_VALUE value = amqpvalue_create_null();
                amqpvalue_set_map_value(source, key, value);
                amqpvalue_destroy(key);
                amqpvalue_destroy(value);
            }
            unsigned char expected_bytes[3 + 255] = { 0xC1, 0xFF, 0xAA };
            for (i = 0; i < 85; i++)
            {
                expected_bytes[3 + (i * 3)] = 0x52;
                expected_bytes[3 + (i * 3) + 1] = i + 1;
                expected_bytes[3 + (i * 3) + 2] = 0x40;
            }
            stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
            test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
        }

        /* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
        TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_85_key_value_pairs_fails)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            int i;
            for (i = 0; i < 85; i++)
            {
                AMQP_VALUE key = amqpvalue_create_uint(i + 1);
                AMQP_VALUE value = amqpvalue_create_null();
                amqpvalue_set_map_value(source, key, value);
                amqpvalue_destroy(key);
                amqpvalue_destroy(value);
            }
            test_amqpvalue_encode_failure(&mocks, source);
        }

        /* Tests_SRS_AMQPVALUE_01_307: [<encoding name="map32" code="0xd1" category="compound" width="4" label="up to 2^32 - 1 octets of encoded map data"/>]  */
        /* Tests_SRS_AMQPVALUE_01_123: [A map is encoded as a compound value where the constituent elements form alternating key value pairs.] */
        /* Tests_SRS_AMQPVALUE_01_124: [Map encodings MUST contain an even number of items (i.e. an equal number of keys and values).] */
        TEST_FUNCTION(amqpvalue_encode_86_key_value_pairs_succeeds)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            int i;

            AMQP_VALUE key = amqpvalue_create_uint(0xFF);
            AMQP_VALUE value = amqpvalue_create_uint(0xFF);
            amqpvalue_set_map_value(source, key, value);
            amqpvalue_destroy(key);
            amqpvalue_destroy(value);

            for (i = 1; i < 85; i++)
            {
                AMQP_VALUE key = amqpvalue_create_uint(i);
                AMQP_VALUE value = amqpvalue_create_null();
                amqpvalue_set_map_value(source, key, value);
                amqpvalue_destroy(key);
                amqpvalue_destroy(value);
            }
            unsigned char expected_bytes[9 + 256] = { 0xD0, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x52, 0xFF, 0x52, 0xFF };
            for (i = 1; i < 85; i++)
            {
                expected_bytes[10 + (i * 3)] = 0x52;
                expected_bytes[10 + (i * 3) + 1] = i;
                expected_bytes[10 + (i * 3) + 2] = 0x40;
            }
            stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
            test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
        }

        /* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
        TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_86_key_value_pairs_fails)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            int i;

            AMQP_VALUE key = amqpvalue_create_uint(0xFF);
            AMQP_VALUE value = amqpvalue_create_uint(0xFF);
            amqpvalue_set_map_value(source, key, value);
            amqpvalue_destroy(key);
            amqpvalue_destroy(value);

            for (i = 1; i < 85; i++)
            {
                AMQP_VALUE key = amqpvalue_create_uint(i);
                AMQP_VALUE value = amqpvalue_create_null();
                amqpvalue_set_map_value(source, key, value);
                amqpvalue_destroy(key);
                amqpvalue_destroy(value);
            }
            test_amqpvalue_encode_failure(&mocks, source);
        }

        /* Tests_SRS_AMQPVALUE_01_307: [<encoding name="map32" code="0xd1" category="compound" width="4" label="up to 2^32 - 1 octets of encoded map data"/>]  */
        /* Tests_SRS_AMQPVALUE_01_123: [A map is encoded as a compound value where the constituent elements form alternating key value pairs.] */
        /* Tests_SRS_AMQPVALUE_01_124: [Map encodings MUST contain an even number of items (i.e. an equal number of keys and values).] */
        TEST_FUNCTION(amqpvalue_encode_128_key_value_pairs_succeeds)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            int i;
            for (i = 0; i < 128; i++)
            {
                AMQP_VALUE key = amqpvalue_create_uint(i + 1);
                AMQP_VALUE value = amqpvalue_create_null();
                amqpvalue_set_map_value(source, key, value);
                amqpvalue_destroy(key);
                amqpvalue_destroy(value);
            }
            unsigned char expected_bytes[9 + 384] = { 0xD0, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x00 };
            for (i = 0; i < 128; i++)
            {
                expected_bytes[9 + (i * 3)] = 0x52;
                expected_bytes[9 + (i * 3) + 1] = i + 1;
                expected_bytes[9 + (i * 3) + 2] = 0x40;
            }
            stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
            test_amqpvalue_encode(&mocks, source, expected_stringified_encoded);
        }

        /* Tests_SRS_AMQPVALUE_01_274: [When the encoder output function fails, amqpvalue_encode shall fail and return a non-zero value.] */
        TEST_FUNCTION(when_encoder_output_fails_amqpvalue_encode_128_key_value_pairs_fails)
        {
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_map();
            int i;
            for (i = 0; i < 128; i++)
            {
                AMQP_VALUE key = amqpvalue_create_uint(i + 1);
                AMQP_VALUE value = amqpvalue_create_null();
                amqpvalue_set_map_value(source, key, value);
                amqpvalue_destroy(key);
                amqpvalue_destroy(value);
            }
            test_amqpvalue_encode_failure(&mocks, source);
        }

        /* amqpvalue_get_encoded_size */

        /* Tests_SRS_AMQPVALUE_01_309: [If any argument is NULL, amqpvalue_get_encoded_size shall return a non-zero value.] */
        TEST_FUNCTION(amqpvalue_get_encoded_size_with_NULL_value_argument_fails)
        {
            // arrange
            amqpvalue_mocks mocks;

            // act
            size_t encoded_size;
            int result = amqpvalue_get_encoded_size(NULL, &encoded_size);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }

        /* Tests_SRS_AMQPVALUE_01_309: [If any argument is NULL, amqpvalue_get_encoded_size shall return a non-zero value.] */
        TEST_FUNCTION(amqpvalue_get_encoded_size_with_NULL_encoded_size_argument_fails)
        {
            // arrange
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_null();
            mocks.ResetAllCalls();

            // act
            int result = amqpvalue_get_encoded_size(source, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
            mocks.AssertActualAndExpectedCalls();

            // cleanup
            amqpvalue_destroy(source);
        }

		static void test_amqpvalue_get_encoded_size(amqpvalue_mocks* mocks, AMQP_VALUE source, size_t expected_encoded_size)
		{
			// arrange
			mocks->ResetAllCalls();

			// act
			size_t encoded_size;
			int result = amqpvalue_get_encoded_size(source, &encoded_size);

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			ASSERT_ARE_EQUAL(size_t, expected_encoded_size, encoded_size);
			mocks->AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_destroy(source);
		}

        /* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
        /* Tests_SRS_AMQPVALUE_01_264: [<encoding code="0x40" category="fixed" width="0" label="the null value"/>] */
        TEST_FUNCTION(amqpvalue_get_encoded_size_with_null_value_succeeds)
        {
            // arrange
            amqpvalue_mocks mocks;
            AMQP_VALUE source = amqpvalue_create_null();
			test_amqpvalue_get_encoded_size(&mocks, source, 1);
        }

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_272: [<encoding name="true" code="0x41" category="fixed" width="0" label="the boolean value true"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_true_bool_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(true);
			test_amqpvalue_get_encoded_size(&mocks, source, 1);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_273: [<encoding name="false" code="0x42" category="fixed" width="0" label="the boolean value false"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_false_bool_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_boolean(false);
			test_amqpvalue_get_encoded_size(&mocks, source, 1);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_275: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ubyte_0x0_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(0x0);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_275: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ubyte_0xFF_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ubyte(0xFF);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_276: [<encoding code="0x60" category="fixed" width="2" label="16-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ushort_0x0_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(0x0);
			test_amqpvalue_get_encoded_size(&mocks, source, 3);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_276: [<encoding code="0x60" category="fixed" width="2" label="16-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ushort_0xFFFF_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ushort(0xFFFF);
			test_amqpvalue_get_encoded_size(&mocks, source, 3);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_277: [<encoding code="0x70" category="fixed" width="4" label="32-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_uint_0xFFFFFFFF_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0xFFFFFFFF);
			test_amqpvalue_get_encoded_size(&mocks, source, 5);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_278: [<encoding name="smalluint" code="0x52" category="fixed" width="1" label="unsigned integer value in the range 0 to 255 inclusive"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_uint_0xFF_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0xFF);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_279: [<encoding name="uint0" code="0x43" category="fixed" width="0" label="the uint value 0"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_uint_0x0_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_uint(0x0);
			test_amqpvalue_get_encoded_size(&mocks, source, 1);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_280: [<encoding code="0x80" category="fixed" width="8" label="64-bit unsigned integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ulong_0xFFFFFFFFFFFFFFFF_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0xFFFFFFFFFFFFFFFF);
			test_amqpvalue_get_encoded_size(&mocks, source, 9);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_281: [<encoding name="smallulong" code="0x53" category="fixed" width="1" label="unsigned long value in the range 0 to 255 inclusive"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ulong_0xFF_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0xFF);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_282: [<encoding name="ulong0" code="0x44" category="fixed" width="0" label="the ulong value 0"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_ulong_0x0_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_ulong(0x0);
			test_amqpvalue_get_encoded_size(&mocks, source, 1);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_283: [<encoding code="0x51" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_byte_minus_128_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(-128);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_283: [<encoding code="0x51" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_byte_127_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_byte(127);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_284: [<encoding code="0x61" category="fixed" width="2" label="16-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_short_minus_32768_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_short(-32768);
			test_amqpvalue_get_encoded_size(&mocks, source, 3);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_285: [<encoding code="0x71" category="fixed" width="4" label="32-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_int_2147483647_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(2147483647);
			test_amqpvalue_get_encoded_size(&mocks, source, 5);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_285: [<encoding code="0x71" category="fixed" width="4" label="32-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_int_minus_2147483648_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(-2147483647-1);
			test_amqpvalue_get_encoded_size(&mocks, source, 5);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_286: [<encoding name="smallint" code="0x54" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_int_127_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(127);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_286: [<encoding name="smallint" code="0x54" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_int_minus_128_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_int(-128);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_287: [<encoding code="0x81" category="fixed" width="8" label="64-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_long_9223372036854775807_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(9223372036854775807i64);
			test_amqpvalue_get_encoded_size(&mocks, source, 9);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_287: [<encoding code="0x81" category="fixed" width="8" label="64-bit two's-complement integer in network byte order"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_long_minus_9223372036854775808_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(-9223372036854775807i64 - 1);
			test_amqpvalue_get_encoded_size(&mocks, source, 9);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_288: [<encoding name="smalllong" code="0x55" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_long_127_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(127);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_288: [<encoding name="smalllong" code="0x55" category="fixed" width="1" label="8-bit two's-complement integer"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_long_minus_128_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_long(-128);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_295: [<encoding name="ms64" code="0x83" category="fixed" width="8" label="64-bit two's-complement integer representing milliseconds since the unix epoch"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_timestamp_9223372036854775807_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(9223372036854775807i64);
			test_amqpvalue_get_encoded_size(&mocks, source, 9);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_295: [<encoding name="ms64" code="0x83" category="fixed" width="8" label="64-bit two's-complement integer representing milliseconds since the unix epoch"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_timestamp_minus_9223372036854775808_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_timestamp(-9223372036854775807i64 - 1);
			test_amqpvalue_get_encoded_size(&mocks, source, 9);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_296: [<encoding code="0x98" category="fixed" width="16" label="UUID as defined in section 4.1.2 of RFC-4122"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_uuid_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0 };
			AMQP_VALUE source = amqpvalue_create_uuid(uuid);
			test_amqpvalue_get_encoded_size(&mocks, source, 17);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_297: [<encoding name="vbin8" code="0xa0" category="variable" width="1" label="up to 2^8 - 1 octets of binary data"/>]  */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_binary_zero_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_binary binary = { NULL, 0 };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_297: [<encoding name="vbin8" code="0xa0" category="variable" width="1" label="up to 2^8 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_binary_1_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes[] = { 0x42 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_get_encoded_size(&mocks, source, 3);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_297: [<encoding name="vbin8" code="0xa0" category="variable" width="1" label="up to 2^8 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_binary_255_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes[255] = { 0x42 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_get_encoded_size(&mocks, source, 257);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_298: [<encoding name="vbin32" code="0xb0" category="variable" width="4" label="up to 2^32 - 1 octets of binary data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_binary_256_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes[256] = { 0x42 };
			amqp_binary binary = { &bytes, sizeof(bytes) };
			AMQP_VALUE source = amqpvalue_create_binary(binary);
			test_amqpvalue_get_encoded_size(&mocks, source, 261);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_299: [<encoding name="str8-utf8" code="0xa1" category="variable" width="1" label="up to 2^8 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_string_0_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("");
			test_amqpvalue_get_encoded_size(&mocks, source, 2);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_299: [<encoding name="str8-utf8" code="0xa1" category="variable" width="1" label="up to 2^8 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_string_1_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_string("a");
			test_amqpvalue_get_encoded_size(&mocks, source, 3);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_299: [<encoding name="str8-utf8" code="0xa1" category="variable" width="1" label="up to 2^8 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_string_255_chars_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			char string[256];
			memset(string, 'a', 255);
			string[255] = 0;
			AMQP_VALUE source = amqpvalue_create_string(string);
			test_amqpvalue_get_encoded_size(&mocks, source, 257);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_300: [<encoding name="str32-utf8" code="0xb1" category="variable" width="4" label="up to 2^32 - 1 octets worth of UTF-8 Unicode (with no byte order mark)"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_string_256_chars_length_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			char string[257];
			memset(string, 'a', 256);
			string[256] = 0;
			AMQP_VALUE source = amqpvalue_create_string(string);
			test_amqpvalue_get_encoded_size(&mocks, source, 261);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_303: [<encoding name="list0" code="0x45" category="fixed" width="0" label="the empty list (i.e. the list with no elements)"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_empty_list_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			test_amqpvalue_get_encoded_size(&mocks, source, 1);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_list_value_with_1_item_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 4);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_list_value_with_2_items_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(source, 0, item);
			(void)amqpvalue_set_list_item(source, 1, item);
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 5);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_list_value_with_1_string_item_with_5_chars_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_string("fluff");
			(void)amqpvalue_set_list_item(source, 0, item);
			amqpvalue_destroy(item);
			/* 1 - list descriptor +
			   1 - list size
			   1 - list item count
			   1 - string descriptor
			   1 - string length
			   5 - string chars */
			test_amqpvalue_get_encoded_size(&mocks, source, 10);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_304: [<encoding name="list8" code="0xc0" category="compound" width="1" label="up to 2^8 - 1 list elements with total size less than 2^8 octets"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_list_value_with_255_null_items_succeeds)
		{
			// arrange
			uint32_t i;
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			for (i = 0; i < 255; i++)
			{
				(void)amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 258);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_305: [<encoding name="list32" code="0xd0" category="compound" width="4" label="up to 2^32 - 1 list elements with total size less than 2^32 octets"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_list_value_with_256_null_items_succeeds)
		{
			// arrange
			uint32_t i;
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			for (i = 0; i < 256; i++)
			{
				(void)amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 265);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_empty_map_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			test_amqpvalue_get_encoded_size(&mocks, source, 3);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_map_value_with_1_null_key_and_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			AMQP_VALUE item = amqpvalue_create_null();
			amqpvalue_set_map_value(source, item, item);
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 5);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_map_value_with_2_keys_and_values_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			AMQP_VALUE key1 = amqpvalue_create_uint(1);
			AMQP_VALUE key2 = amqpvalue_create_uint(2);
			AMQP_VALUE item = amqpvalue_create_null();
			amqpvalue_set_map_value(source, key1, item);
			amqpvalue_set_map_value(source, key2, item);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 9);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_306: [<encoding name="map8" code="0xc1" category="compound" width="1" label="up to 2^8 - 1 octets of encoded map data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_map_value_with_1_key_and_value_255_bytes_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			AMQP_VALUE key1 = amqpvalue_create_null();
			char string[253];
			memset(string, 'a', 252);
			string[252] = 0;
			AMQP_VALUE item = amqpvalue_create_string(string);
			amqpvalue_set_map_value(source, key1, item);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 258);
		}

		/* Tests_SRS_AMQPVALUE_01_308: [amqpvalue_get_encoded_size shall fill in the encoded_size argument the number of bytes required to encode the given AMQP value.] */
		/* Tests_SRS_AMQPVALUE_01_307: [<encoding name="map32" code="0xd1" category="compound" width="4" label="up to 2^32 - 1 octets of encoded map data"/>] */
		TEST_FUNCTION(amqpvalue_get_encoded_size_with_map_value_with_1_key_and_value_256_bytes_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE source = amqpvalue_create_map();
			AMQP_VALUE key1 = amqpvalue_create_null();
			char string[254];
			memset(string, 'a', 253);
			string[253] = 0;
			AMQP_VALUE item = amqpvalue_create_string(string);
			amqpvalue_set_map_value(source, key1, item);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(item);
			test_amqpvalue_get_encoded_size(&mocks, source, 265);
		}

		/* amqpvalue_destroy */

		/* Tests_SRS_AMQPVALUE_01_315: [If the value argument is NULL, amqpvalue_destroy shall do nothing.] */
		TEST_FUNCTION(amqpvalue_decoder_destroy_with_NULL_does_nothing)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			amqpvalue_destroy(NULL);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_null_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_null();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_ubyte_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ubyte(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_ushort_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ushort(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_uint_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_uint(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_ulong_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ulong(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_byte_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_byte(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_short_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_short(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_int_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_int(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_long_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_long(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_float_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_float(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_double_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_double(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_char_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_char(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_timestamp_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_timestamp(0);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_uuid_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0 };
			AMQP_VALUE value = amqpvalue_create_uuid(uuid);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_binary_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes[] = { 0x42 };
			amqp_binary binary = { bytes, sizeof(bytes) };
			AMQP_VALUE value = amqpvalue_create_binary(binary);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_string_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_string("test");
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_empty_list_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_list();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_list_value_with_1_null_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value, 0, null_value);
			amqpvalue_destroy(null_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_list_value_with_2_null_items)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value, 0, null_value);
			(void)amqpvalue_set_list_item(value, 1, null_value);
			amqpvalue_destroy(null_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the 2 null items */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_empty_map_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_map();
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_map_value_with_1_key_value_pair)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_map();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_map_value(value, null_value, null_value);
			amqpvalue_destroy(null_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the key and value */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_map_value_with_2_key_value_pairs)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_map();
			AMQP_VALUE key1 = amqpvalue_create_ubyte(1);
			AMQP_VALUE key2 = amqpvalue_create_uint(2);
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_map_value(value, key1, null_value);
			(void)amqpvalue_set_map_value(value, key2, null_value);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(null_value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the first key and value */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the first second and value */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_null_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_null();
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_ubyte_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ubyte(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_ushort_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ushort(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_uint_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_uint(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_ulong_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_ulong(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_byte_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_byte(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_short_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_short(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_int_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_int(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_long_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_long(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_float_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_float(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_double_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_double(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_char_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_char(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_timestamp_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_timestamp(0);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_uuid_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			amqp_uuid uuid = { 0 };
			AMQP_VALUE value = amqpvalue_create_uuid(uuid);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_binary_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes[] = { 0x42 };
			amqp_binary binary = { bytes, sizeof(bytes) };
			AMQP_VALUE value = amqpvalue_create_binary(binary);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_string_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_string("test");
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_empty_list_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_list();
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_list_cloned_value_with_1_null_item)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value, 0, null_value);
			amqpvalue_destroy(null_value);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_list_cloned_value_with_2_null_items)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_list();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_list_item(value, 0, null_value);
			(void)amqpvalue_set_list_item(value, 1, null_value);
			amqpvalue_destroy(null_value);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the 2 null items */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_empty_map_cloned_value)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_map();
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_map_cloned_value_with_1_key_value_pair)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_map();
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_map_value(value, null_value, null_value);
			amqpvalue_destroy(null_value);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the key and value */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_314: [amqpvalue_destroy shall free all resources allocated by any of the amqpvalue_create_xxx functions or amqpvalue_clone.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_the_memory_for_map_cloned_value_with_2_key_value_pairs)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQP_VALUE value = amqpvalue_create_map();
			AMQP_VALUE key1 = amqpvalue_create_ubyte(1);
			AMQP_VALUE key2 = amqpvalue_create_uint(2);
			AMQP_VALUE null_value = amqpvalue_create_null();
			(void)amqpvalue_set_map_value(value, key1, null_value);
			(void)amqpvalue_set_map_value(value, key2, null_value);
			amqpvalue_destroy(key1);
			amqpvalue_destroy(key2);
			amqpvalue_destroy(null_value);
			AMQP_VALUE cloned_value = amqpvalue_clone(value);
			amqpvalue_destroy(value);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the first key and value */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			/* this is for the first second and value */
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			amqpvalue_destroy(cloned_value);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* amqpvalue_decoder_create */

		/* Tests_SRS_AMQPVALUE_01_311: [amqpvalue_decoder_create shall create a new amqp value decoder and return a non-NULL handle to it.] */
		TEST_FUNCTION(amqpvalue_decoder_create_returns_a_non_NULL_handle)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();

			// act
			AMQPVALUE_DECODER_HANDLE result = amqpvalue_decoder_create(value_decoded_callback, test_context);

			// assert
			ASSERT_IS_NOT_NULL(result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_decoder_destroy(result);
		}

		/* Tests_SRS_AMQPVALUE_01_312: [If the value_decoded_callback argument is NULL, amqpvalue_decoder_create shall return NULL.] */
		TEST_FUNCTION(amqpvalue_decoder_create_with_NULL_callback_returns_NULL)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			AMQPVALUE_DECODER_HANDLE result = amqpvalue_decoder_create(NULL, test_context);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_313: [If creating the decoder fails, amqpvalue_decoder_create shall return NULL.] */
		TEST_FUNCTION(when_allocating_the_decoder_fails_amqpvalue_decoder_create_fails)
		{
			// arrange
			amqpvalue_mocks mocks;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			// act
			AMQPVALUE_DECODER_HANDLE result = amqpvalue_decoder_create(value_decoded_callback, test_context);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_313: [If creating the decoder fails, amqpvalue_decoder_create shall return NULL.] */
		TEST_FUNCTION(when_allocating_the_initial_decode_value_fails_amqpvalue_decoder_create_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQPVALUE_DECODER_HANDLE result = amqpvalue_decoder_create(value_decoded_callback, test_context);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* Tests_SRS_AMQPVALUE_01_313: [If creating the decoder fails, amqpvalue_decoder_create shall return NULL.] */
		TEST_FUNCTION(when_allocating_memoory_fails_amqpvalue_decoder_create_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.SetReturn((void*)NULL);

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

			// act
			AMQPVALUE_DECODER_HANDLE result = amqpvalue_decoder_create(value_decoded_callback, test_context);

			// assert
			ASSERT_IS_NULL(result);
		}

		/* amqpvalue_decoder_destroy */

		/* Tests_SRS_AMQPVALUE_01_316: [amqpvalue_decoder_destroy shall free all resources associated with the amqpvalue_decoder.] */
		TEST_FUNCTION(amqpvalue_destroy_frees_underlying_allocated_chunks)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();

			EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG))
				.ExpectedAtLeastTimes(3);

			// act
			amqpvalue_decoder_destroy(amqpvalue_decoder);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* Tests_SRS_AMQPVALUE_01_317: [If handle is NULL, amqpvalue_decoder_destroy shall do nothing.] */
		TEST_FUNCTION(amqpvalue_destroy_with_NULL_handle_does_not_free_anything)
		{
			// arrange
			amqpvalue_mocks mocks;

			// act
			amqpvalue_decoder_destroy(NULL);

			// assert
			// no explicit assert, uMock checks the calls
		}

		/* amqpvalue_decode_bytes */

		/* Tests_SRS_AMQPVALUE_01_320: [If handle or buffer are NULL, amqpvalue_decode_bytes shall return a non-zero value.] */
		TEST_FUNCTION(amqpvalue_decode_bytes_with_NULL_handle_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			unsigned char bytes[] = { 0x40 };

			// act
			int result = amqpvalue_decode_bytes(NULL, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
		}

		/* Tests_SRS_AMQPVALUE_01_320: [If handle or buffer are NULL, amqpvalue_decode_bytes shall return a non-zero value.] */
		TEST_FUNCTION(amqpvalue_decode_bytes_with_NULL_buffer_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, NULL, 1);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_321: [If size is 0, amqpvalue_decode_bytes shall return a non-zero value.] */
		TEST_FUNCTION(amqpvalue_decode_bytes_with_0_size_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x40 };

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, 0);

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_318: [amqpvalue_decode_bytes shall decode size bytes that are passed in the buffer argument.] */
		/* Tests_SRS_AMQPVALUE_01_319: [On success, amqpvalue_decode_bytes shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_322: [amqpvalue_decode_bytes shall process the bytes byte by byte, as a stream.] */
		/* Tests_SRS_AMQPVALUE_01_323: [When enough bytes have been processed for a valid amqp value, the value_decoded_callback passed in amqpvalue_decoder_create shall be called.] */
		/* Tests_SRS_AMQPVALUE_01_324: [The decoded amqp value shall be passed to value_decoded_callback.] */
		/* Tests_SRS_AMQPVALUE_01_325: [Also the context stored in amqpvalue_decoder_create shall be passed to the value_decoded_callback callback.] */
		/* Tests_SRS_AMQPVALUE_01_328: [1.6.1 null Indicates an empty value.] */
		/* Tests_SRS_AMQPVALUE_01_329: [<encoding code="0x40" category="fixed" width="0" label="the null value"/>] */
		TEST_FUNCTION(amqpvalue_decode_1_amqp_null_value_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x40 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_NULL, (int)amqpvalue_get_type(decoded_values[0]));

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_318: [amqpvalue_decode_bytes shall decode size bytes that are passed in the buffer argument.] */
		/* Tests_SRS_AMQPVALUE_01_319: [On success, amqpvalue_decode_bytes shall return 0.] */
		/* Tests_SRS_AMQPVALUE_01_322: [amqpvalue_decode_bytes shall process the bytes byte by byte, as a stream.] */
		/* Tests_SRS_AMQPVALUE_01_323: [When enough bytes have been processed for a valid amqp value, the value_decoded_callback passed in amqpvalue_decoder_create shall be called.] */
		/* Tests_SRS_AMQPVALUE_01_324: [The decoded amqp value shall be passed to value_decoded_callback.] */
		/* Tests_SRS_AMQPVALUE_01_325: [Also the context stored in amqpvalue_decoder_create shall be passed to the value_decoded_callback callback.] */
		/* Tests_SRS_AMQPVALUE_01_328: [1.6.1 null Indicates an empty value.] */
		/* Tests_SRS_AMQPVALUE_01_329: [<encoding code="0x40" category="fixed" width="0" label="the null value"/>] */
		TEST_FUNCTION(amqpvalue_decode_2_amqp_null_values_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x40, 0x40 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_NULL, (int)amqpvalue_get_type(decoded_values[0]));
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_NULL, (int)amqpvalue_get_type(decoded_values[1]));

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_331: [<encoding code="0x56" category="fixed" width="1" label="boolean with the octet 0x00 being false and octet 0x01 being true"/>] */
		TEST_FUNCTION(amqpvalue_decode_boolean_false_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x56, 0x00 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_BOOL, (int)amqpvalue_get_type(decoded_values[0]));
			bool actual_value = true;
			amqpvalue_get_boolean(decoded_values[0], &actual_value);
			ASSERT_IS_FALSE(actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_331: [<encoding code="0x56" category="fixed" width="1" label="boolean with the octet 0x00 being false and octet 0x01 being true"/>] */
		TEST_FUNCTION(amqpvalue_decode_boolean_true_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x56, 0x01 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_BOOL, (int)amqpvalue_get_type(decoded_values[0]));
			bool actual_value = false;
			amqpvalue_get_boolean(decoded_values[0], &actual_value);
			ASSERT_IS_TRUE(actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_331: [<encoding code="0x56" category="fixed" width="1" label="boolean with the octet 0x00 being false and octet 0x01 being true"/>] */
		TEST_FUNCTION(amqpvalue_decode_boolean_true_byte_by_byte_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x56, 0x01 };
			int i;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			for (i = 0; i < sizeof(bytes); i++)
			{
				int result = amqpvalue_decode_bytes(amqpvalue_decoder, &bytes[i], 1);
				ASSERT_ARE_EQUAL(int, 0, result);
			}

			// assert
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_BOOL, (int)amqpvalue_get_type(decoded_values[0]));
			bool actual_value = false;
			amqpvalue_get_boolean(decoded_values[0], &actual_value);
			ASSERT_IS_TRUE(actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_327: [If not enough bytes have accumulated to decode a value, the value_decoded_callback shall not be called.] */
		TEST_FUNCTION(amqpvalue_decode_boolean_0x56_only_one_byte_succeeds_but_does_not_trigger_callback)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x56 };

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_331: [<encoding code="0x56" category="fixed" width="1" label="boolean with the octet 0x00 being false and octet 0x01 being true"/>] */
		TEST_FUNCTION(amqpvalue_decode_boolean_with_0x56_payload_greater_than_1_fails)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x56, 0x02 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_NOT_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_332: [<encoding name="true" code="0x41" category="fixed" width="0" label="the boolean value true"/>] */
		TEST_FUNCTION(amqpvalue_decode_0x41_true_boolean_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x41 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_BOOL, (int)amqpvalue_get_type(decoded_values[0]));
			bool actual_value = false;
			amqpvalue_get_boolean(decoded_values[0], &actual_value);
			ASSERT_IS_TRUE(actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_330: [1.6.2 boolean Represents a true or false value.] */
		/* Tests_SRS_AMQPVALUE_01_333: [<encoding name="false" code="0x42" category="fixed" width="0" label="the boolean value false"/>] */
		TEST_FUNCTION(amqpvalue_decode_0x42_false_boolean_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x42 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_BOOL, (int)amqpvalue_get_type(decoded_values[0]));
			bool actual_value = true;
			amqpvalue_get_boolean(decoded_values[0], &actual_value);
			ASSERT_IS_FALSE(actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_334: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
		/* Tests_SRS_AMQPVALUE_01_335: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_decode_ubyte_0x00_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x50, 0x00 };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_UBYTE, (int)amqpvalue_get_type(decoded_values[0]));
			unsigned char actual_value = 1;
			amqpvalue_get_ubyte(decoded_values[0], &actual_value);
			ASSERT_ARE_EQUAL(uint8_t, 0, actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_334: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
		/* Tests_SRS_AMQPVALUE_01_335: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_decode_ubyte_0xFF_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x50, 0xFF };

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_UBYTE, (int)amqpvalue_get_type(decoded_values[0]));
			unsigned char actual_value = 0;
			amqpvalue_get_ubyte(decoded_values[0], &actual_value);
			ASSERT_ARE_EQUAL(uint8_t, 0xFF, actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_334: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
		/* Tests_SRS_AMQPVALUE_01_335: [<encoding code="0x50" category="fixed" width="1" label="8-bit unsigned integer"/>] */
		TEST_FUNCTION(amqpvalue_decode_ubyte_0xFF_given_byte_by_byte_succeeds)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x50, 0xFF };
			int i;

			EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
				.IgnoreAllCalls();
			STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
				.IgnoreArgument(2);

			// act
			for (i = 0; i < sizeof(bytes); i++)
			{
				int result = amqpvalue_decode_bytes(amqpvalue_decoder, &bytes[i], 1);
				ASSERT_ARE_EQUAL(int, 0, result);
			}

			// assert
			mocks.AssertActualAndExpectedCalls();
			ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_UBYTE, (int)amqpvalue_get_type(decoded_values[0]));
			unsigned char actual_value = 0;
			amqpvalue_get_ubyte(decoded_values[0], &actual_value);
			ASSERT_ARE_EQUAL(uint8_t, 0xFF, actual_value);

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

		/* Tests_SRS_AMQPVALUE_01_334: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
		/* Tests_SRS_AMQPVALUE_01_327: [If not enough bytes have accumulated to decode a value, the value_decoded_callback shall not be called.] */
		TEST_FUNCTION(amqpvalue_decode_ubyte_with_only_one_byte_succeds_but_no_callback_triggered)
		{
			// arrange
			amqpvalue_mocks mocks;
			AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
			mocks.ResetAllCalls();
			unsigned char bytes[] = { 0x50 };

			// act
			int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

			// assert
			ASSERT_ARE_EQUAL(int, 0, result);
			mocks.AssertActualAndExpectedCalls();

			// cleanup
			amqpvalue_decoder_destroy(amqpvalue_decoder);
		}

        /* Tests_SRS_AMQPVALUE_01_336: [1.6.4 ushort Integer in the range 0 to 216 - 1 inclusive.] */
        /* Tests_SRS_AMQPVALUE_01_337: [<encoding code="0x60" category="fixed" width="2" label="16-bit unsigned integer in network byte order"/>] */
        TEST_FUNCTION(amqpvalue_decode_ushort_0x0_succeeds)
        {
            // arrange
            amqpvalue_mocks mocks;
            AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
            mocks.ResetAllCalls();
            unsigned char bytes[] = { 0x60, 0x0, 0x0 };

            EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
                .IgnoreAllCalls();
            STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
                .IgnoreArgument(2);

            // act
            int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            mocks.AssertActualAndExpectedCalls();
            ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_USHORT, (int)amqpvalue_get_type(decoded_values[0]));
            uint16_t actual_value = 1;
            amqpvalue_get_ushort(decoded_values[0], &actual_value);
            ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0, (uint32_t)actual_value);

            // cleanup
            amqpvalue_decoder_destroy(amqpvalue_decoder);
        }

        /* Tests_SRS_AMQPVALUE_01_336: [1.6.4 ushort Integer in the range 0 to 216 - 1 inclusive.] */
        /* Tests_SRS_AMQPVALUE_01_337: [<encoding code="0x60" category="fixed" width="2" label="16-bit unsigned integer in network byte order"/>] */
        TEST_FUNCTION(amqpvalue_decode_ushort_0xFFFF_succeeds)
        {
            // arrange
            amqpvalue_mocks mocks;
            AMQPVALUE_DECODER_HANDLE amqpvalue_decoder = amqpvalue_decoder_create(value_decoded_callback, test_context);
            mocks.ResetAllCalls();
            unsigned char bytes[] = { 0x60, 0xFF, 0xFF };

            EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
                .IgnoreAllCalls();
            STRICT_EXPECTED_CALL(mocks, value_decoded_callback(test_context, IGNORED_PTR_ARG))
                .IgnoreArgument(2);

            // act
            int result = amqpvalue_decode_bytes(amqpvalue_decoder, bytes, sizeof(bytes));

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            mocks.AssertActualAndExpectedCalls();
            ASSERT_ARE_EQUAL(int, (int)AMQP_TYPE_USHORT, (int)amqpvalue_get_type(decoded_values[0]));
            uint16_t actual_value = 1;
            amqpvalue_get_ushort(decoded_values[0], &actual_value);
            ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0xFFFF, (uint32_t)actual_value);

            // cleanup
            amqpvalue_decoder_destroy(amqpvalue_decoder);
        }

END_TEST_SUITE(amqpvalue_unittests)
