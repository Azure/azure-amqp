#include <stdio.h>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "amqpvalue.h"

/* Requirements satisfied by the current implementation without any code:
Tests_SRS_AMQPVALUE_01_270: [<encoding code="0x56" category="fixed" width="1" label="boolean with the octet 0x00 being false and octet 0x01 being true"/>]
Tests_SRS_AMQPVALUE_01_099: [Represents an approximate point in time using the Unix time t [IEEE1003] encoding of UTC, but with a precision of milliseconds.]
*/

static unsigned char* encoded_bytes;
static size_t encoded_byte_count;
static char actual_stringified_encoded[8192];
static char expected_stringified_encoded[8192];
static int when_shall_encoder_output_fail = 0;
static int encoder_output_call_count;

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

int test_encoder_output(void* context, const void* bytes, size_t length)
{
	unsigned char* new_bytes = (unsigned char*)realloc(encoded_bytes, encoded_byte_count + length);
	if (new_bytes != NULL)
	{
		encoded_bytes = new_bytes;
		(void)memcpy(encoded_bytes + encoded_byte_count, bytes, length);
		encoded_byte_count += length;
	}
	encoder_output_call_count++;
	return 0;
}

static void* test_context = (void*)0x4243;

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
			encoder_output_call_count = 0;
			when_shall_encoder_output_fail = 0;
		}

		TEST_METHOD_CLEANUP(method_cleanup)
		{
			if (encoded_bytes != NULL)
			{
				free(encoded_bytes);
				encoded_bytes = NULL;
			}
			encoded_byte_count = 0;

			if (!MicroMockReleaseMutex(test_serialize_mutex))
			{
				ASSERT_FAIL("Could not release test serialization mutex.");
			}
		}

#if 0
		/* Tests_SRS_AMQPVALUE_01_305: [<encoding name="list32" code="0xd0" category="compound" width="4" label="up to 2^32 - 1 list elements with total size less than 2^32 octets"/>] */
		TEST_METHOD(amqpvalue_encode_list_with_max_items_succeeds)
		{
			AMQP_VALUE source = amqpvalue_create_list();
			AMQP_VALUE item = amqpvalue_create_null();
			long i;
			amqpvalue_set_list_item(source, 2147483647i32 - 1, item);
			for (i = 0; i < 2147483647i32; i++)
			{
				amqpvalue_set_list_item(source, i, item);
			}
			amqpvalue_destroy(item);
			unsigned char expected_bytes[] = { 0xD0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
			stringify_bytes(expected_bytes, sizeof(expected_bytes), expected_stringified_encoded);
			test_amqp_encode(&mocks, source, expected_stringified_encoded);
		}
#endif

END_TEST_SUITE(amqpvalue_unittests)
