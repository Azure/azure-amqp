#include <cstdint>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "sasl_frame_codec.h"
#include "frame_codec.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"
#include "amqp_definitions_mocks.h"

std::ostream& operator<<(std::ostream& left, const delivery_tag& delivery)
{
	std::ios::fmtflags f(left.flags());
	left << std::hex;
	for (size_t i = 0; i < delivery.length; i++)
	{
		left << ((const unsigned char*)delivery.bytes)[i];
	}
	left.flags(f);
	return left;
}

static bool operator==(const delivery_tag& left, const delivery_tag& right)
{
	if (left.length != right.length)
	{
		return false;
	}
	else
	{
		return memcmp(left.bytes, right.bytes, left.length) == 0;
	}
}

#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4242
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4243
#define TEST_DECODER_HANDLE				(AMQPVALUE_DECODER_HANDLE)0x4244
#define TEST_ENCODER_HANDLE				(ENCODER_HANDLE)0x4245
#define TEST_AMQP_VALUE					(AMQP_VALUE)0x4246
#define TEST_CONTEXT					(void*)0x4247

#define TEST_MIX_MAX_FRAME_SIZE			512

static const unsigned char default_test_encoded_bytes[2] = { 0x42, 0x43 };
static const unsigned char* test_encoded_bytes;
static size_t test_encoded_bytes_size;

static FRAME_RECEIVED_CALLBACK saved_frame_begin_callback;
static void* saved_callback_context;

static VALUE_DECODED_CALLBACK saved_value_decoded_callback;
static void* saved_value_decoded_callback_context;
static size_t total_bytes;

static unsigned char test_sasl_frame_value[] = { 0x42, 0x43, 0x44 };
static unsigned char* sasl_frame_value_decoded_bytes;
static size_t sasl_frame_value_decoded_byte_count;

static char expected_stringified_io[8192];
static char actual_stringified_io[8192];
static uint64_t sasl_frame_descriptor_ulong;

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

TYPED_MOCK_CLASS(sasl_frame_codec_mocks, CGlobalMock)
{
public:
	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* amqpvalue mocks*/
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_ulong, uint64_t, value);
	MOCK_METHOD_END(AMQP_VALUE, TEST_AMQP_VALUE);
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_descriptor, AMQP_VALUE, value);
	MOCK_METHOD_END(AMQP_VALUE, TEST_AMQP_VALUE);
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value)
		*ulong_value = sasl_frame_descriptor_ulong;
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_get_inplace_descriptor, AMQP_VALUE, value)
	MOCK_METHOD_END(AMQP_VALUE, TEST_DESCRIPTOR_AMQP_VALUE);
	MOCK_STATIC_METHOD_1(, void, amqpvalue_destroy, AMQP_VALUE, value)
	MOCK_VOID_METHOD_END();

	/* frame_codec mocks */
	MOCK_STATIC_METHOD_2(, FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, LOGGER_LOG, logger_log);
	MOCK_METHOD_END(FRAME_CODEC_HANDLE, TEST_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, int, frame_codec_subscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, FRAME_RECEIVED_CALLBACK, frame_begin_callback, void*, callback_context);
		saved_frame_begin_callback = frame_begin_callback;
		saved_callback_context = callback_context;
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, frame_codec_unsubscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_5(, int, frame_codec_begin_encode_frame, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, uint32_t, frame_body_size, const unsigned char*, type_specific_bytes, uint32_t, type_specific_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, frame_codec_encode_frame_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, bytes, size_t, length);
	MOCK_METHOD_END(int, 0);

	/* decoder mocks */
	MOCK_STATIC_METHOD_2(, AMQPVALUE_DECODER_HANDLE, amqpvalue_decoder_create, VALUE_DECODED_CALLBACK, value_decoded_callback, void*, value_decoded_callback_context);
		saved_value_decoded_callback = value_decoded_callback;
		saved_value_decoded_callback_context = value_decoded_callback_context;
		total_bytes = 0;
	MOCK_METHOD_END(AMQPVALUE_DECODER_HANDLE, TEST_DECODER_HANDLE);
	MOCK_STATIC_METHOD_1(, void, amqpvalue_decoder_destroy, AMQPVALUE_DECODER_HANDLE, handle);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, amqpvalue_decode_bytes, AMQPVALUE_DECODER_HANDLE, handle, const unsigned char*, buffer, size_t, size);
		unsigned char* new_bytes = (unsigned char*)realloc(sasl_frame_value_decoded_bytes, sasl_frame_value_decoded_byte_count + size);
		int my_result = 0;
		if (new_bytes != NULL)
		{
			sasl_frame_value_decoded_bytes = new_bytes;
			(void)memcpy(sasl_frame_value_decoded_bytes + sasl_frame_value_decoded_byte_count, buffer, size);
			sasl_frame_value_decoded_byte_count += size;
		}
		total_bytes += size;
		if (total_bytes == sizeof(test_sasl_frame_value))
		{
			saved_value_decoded_callback(saved_value_decoded_callback_context, TEST_AMQP_VALUE);
			total_bytes = 0;
		}
		MOCK_METHOD_END(int, 0);

	/* encoder mocks */
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, amqpvalue_encode, AMQP_VALUE, value, AMQPVALUE_ENCODER_OUTPUT, encoder_output, void*, context);
		encoder_output(context, test_encoded_bytes, test_encoded_bytes_size);
	MOCK_METHOD_END(int, 0);

	/* callbacks */
	MOCK_STATIC_METHOD_2(, void, amqp_frame_received_callback_1, void*, context, AMQP_VALUE, performative);
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , AMQP_VALUE, amqpvalue_create_ulong, uint64_t, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , AMQP_VALUE, amqpvalue_create_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_2(sasl_frame_codec_mocks, , int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , AMQP_VALUE, amqpvalue_get_inplace_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , void, amqpvalue_destroy, AMQP_VALUE, value);

	DECLARE_GLOBAL_MOCK_METHOD_2(sasl_frame_codec_mocks, , FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec);
	DECLARE_GLOBAL_MOCK_METHOD_4(sasl_frame_codec_mocks, , int, frame_codec_subscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, FRAME_RECEIVED_CALLBACK, frame_begin_callback, void*, callback_context);
	DECLARE_GLOBAL_MOCK_METHOD_2(sasl_frame_codec_mocks, , int, frame_codec_unsubscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type);
	DECLARE_GLOBAL_MOCK_METHOD_5(sasl_frame_codec_mocks, , int, frame_codec_begin_encode_frame, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, uint32_t, frame_body_size, const unsigned char*, type_specific_bytes, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(sasl_frame_codec_mocks, , int, frame_codec_encode_frame_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, bytes, size_t, length);

	DECLARE_GLOBAL_MOCK_METHOD_2(sasl_frame_codec_mocks, , AMQPVALUE_DECODER_HANDLE, amqpvalue_decoder_create, VALUE_DECODED_CALLBACK, value_decoded_callback, void*, value_decoded_callback_context);
	DECLARE_GLOBAL_MOCK_METHOD_1(sasl_frame_codec_mocks, , void, amqpvalue_decoder_destroy, AMQPVALUE_DECODER_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(sasl_frame_codec_mocks, , int, amqpvalue_decode_bytes, AMQPVALUE_DECODER_HANDLE, handle, const unsigned char*, buffer, size_t, size);

	DECLARE_GLOBAL_MOCK_METHOD_2(sasl_frame_codec_mocks, , int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(sasl_frame_codec_mocks, , int, amqpvalue_encode, AMQP_VALUE, value, AMQPVALUE_ENCODER_OUTPUT, encoder_output, void*, context);

	DECLARE_GLOBAL_MOCK_METHOD_2(sasl_frame_codec_mocks, , void, amqp_frame_received_callback_1, void*, context, AMQP_VALUE, performative);

	extern void consolelogger_log(char* format, ...)
	{
		(void)format;
	}
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(sasl_frame_codec_unittests)

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

	test_encoded_bytes = default_test_encoded_bytes;
	test_encoded_bytes_size = sizeof(default_test_encoded_bytes);
	sasl_frame_descriptor_ulong = SASL_MECHANISMS;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
	if (sasl_frame_value_decoded_bytes != NULL)
	{
		free(sasl_frame_value_decoded_bytes);
		sasl_frame_value_decoded_bytes = NULL;
	}
	sasl_frame_value_decoded_byte_count = 0;
	if (!MicroMockReleaseMutex(test_serialize_mutex))
	{
		ASSERT_FAIL("Could not release test serialization mutex.");
	}
}

/* sasl_frame_codec_create */

/* Tests_SRS_SASL_FRAME_CODEC_01_018: [sasl_frame_codec_create shall create an instance of an sasl_frame_codec and return a non-NULL handle to it.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_020: [sasl_frame_codec_create shall subscribe for SASL frames with the given frame_codec.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_022: [sasl_frame_codec_create shall create a decoder to be used for decoding SASL values.] */
TEST_FUNCTION(sasl_frame_codec_create_with_valid_args_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4);

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NOT_NULL(sasl_frame_codec);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_018: [sasl_frame_codec_create shall create an instance of an sasl_frame_codec and return a non-NULL handle to it.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_020: [sasl_frame_codec_create shall subscribe for SASL frames with the given frame_codec.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_022: [sasl_frame_codec_create shall create a decoder to be used for decoding SASL values.] */
TEST_FUNCTION(sasl_frame_codec_create_with_valid_args_and_NULL_context_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4);

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, NULL);

	// assert
	ASSERT_IS_NOT_NULL(sasl_frame_codec);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_019: [If any of the arguments frame_codec or frame_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
TEST_FUNCTION(sasl_frame_codec_create_with_NULL_frame_codec_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(NULL, amqp_frame_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_019: [If any of the arguments frame_codec or frame_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
TEST_FUNCTION(sasl_frame_codec_create_with_NULL_frame_received_callback_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, NULL, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_021: [If subscribing for SASL frames fails, sasl_frame_codec_create shall fail and return NULL.] */
TEST_FUNCTION(when_frame_codec_subscribe_fails_then_sasl_frame_codec_create_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4)
		.SetReturn(1);

	STRICT_EXPECTED_CALL(mocks, amqpvalue_decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_023: [If creating the decoder fails, sasl_frame_codec_create shall fail and return NULL.] */
TEST_FUNCTION(when_creating_the_decoder_fails_then_sasl_frame_codec_create_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.SetReturn((AMQPVALUE_DECODER_HANDLE)NULL);

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_024: [If allocating memory for the new sasl_frame_codec fails, then sasl_frame_codec_create shall fail and return NULL.]  */
TEST_FUNCTION(when_allocating_memory_for_sasl_frame_codec_fails_then_sasl_frame_codec_create_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* sasl_frame_codec_destroy */

/* Tests_SRS_SASL_FRAME_CODEC_01_025: [sasl_frame_codec_destroy shall free all resources associated with the sasl_frame_codec instance.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_027: [sasl_frame_codec_destroy shall unsubscribe from receiving SASL frames from the frame_codec that was passed to sasl_frame_codec_create.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_028: [The decoder created in sasl_frame_codec_create shall be destroyed by sasl_frame_codec_destroy.] */
TEST_FUNCTION(sasl_frame_codec_destroy_frees_the_decoder_and_unsubscribes_from_AMQP_frames)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	sasl_frame_codec_destroy(sasl_frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_SASL_FRAME_CODEC_01_025: [sasl_frame_codec_destroy shall free all resources associated with the sasl_frame_codec instance.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_027: [sasl_frame_codec_destroy shall unsubscribe from receiving SASL frames from the frame_codec that was passed to sasl_frame_codec_create.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_028: [The decoder created in sasl_frame_codec_create shall be destroyed by sasl_frame_codec_destroy.] */
TEST_FUNCTION(when_unsubscribe_fails_sasl_frame_codec_destroy_still_frees_everything)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	sasl_frame_codec_destroy(sasl_frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_SASL_FRAME_CODEC_01_026: [If sasl_frame_codec is NULL, sasl_frame_codec_destroy shall do nothing.] */
TEST_FUNCTION(sasl_frame_codec_destroy_with_NULL_handle_does_nothing)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	sasl_frame_codec_destroy(NULL);

	// assert
	// uMock checks the calls
}

/* sasl_frame_codec_encode_frame */

/* Tests_SRS_SASL_FRAME_CODEC_01_029: [sasl_frame_codec_encode_frame shall encode the frame header and AMQP value in a SASL frame and on success it shall return 0.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_031: [sasl_frame_codec_encode_frame shall encode the frame header by using frame_codec_begin_encode_frame.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_032: [The payload frame size shall be computed based on the encoded size of the sasl_frame and its fields.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_033: [The encoded size of the sasl_frame and its fields shall be obtained by calling amqpvalue_get_encoded_size.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_035: [Encoding of the sasl_frame and its fields shall be done by calling amqpvalue_encode.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_036: [The encode result for the sasl_frame and its fields shall be given to frame_codec by calling frame_codec_encode_frame_bytes.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_012: [Bytes 6 and 7 of the header are ignored.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_013: [Implementations SHOULD set these to 0x00.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_014: [The extended header is ignored.] */
/* Tests_SRS_SASL_FRAME_CODEC_01_015: [Implementations SHOULD therefore set DOFF to 0x02.] */
TEST_FUNCTION(encoding_a_sasl_frame_succeeds)
{
	// arrange
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t sasl_frame_value_size = 2;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_value_size, sizeof(sasl_frame_value_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, sasl_frame_value_size, NULL, 0));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, test_encoded_bytes_size))
		.ValidateArgumentBuffer(2, test_encoded_bytes, test_encoded_bytes_size);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_030: [If frame_codec or sasl_frame_fields is NULL, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_encode_frame_with_NULL_sasl_frame_codec_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	int result = sasl_frame_codec_encode_frame(NULL, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_030: [If frame_codec or sasl_frame_fields is NULL, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_encode_frame_with_NULL_performative_value_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, NULL);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_amqpvalue_get_inplace_descriptor_fails_then_sasl_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t sasl_frame_value_size = 2;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE))
		.SetReturn((AMQP_VALUE)NULL);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_amqpvalue_get_ulong_fails_then_sasl_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t sasl_frame_value_size = 2;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_amqpvalue_get_encoded_size_fails_then_sasl_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t sasl_frame_value_size = 2;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_frame_codec_begin_encode_frame_fails_then_sasl_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t sasl_frame_value_size = 2;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_value_size, sizeof(sasl_frame_value_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, sasl_frame_value_size, NULL, 0))
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_amqpvalue_encode_fails_then_sasl_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t sasl_frame_value_size = 2;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_value_size, sizeof(sasl_frame_value_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, sasl_frame_value_size, NULL, 0));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, test_encoded_bytes_size))
		.ValidateArgumentBuffer(2, test_encoded_bytes, test_encoded_bytes_size);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_047: [The frame body of a SASL frame MUST contain exactly one AMQP type, whose type encoding MUST have provides=�sasl-frame�.] */
TEST_FUNCTION(sasl_frame_values_are_encoded_successfully)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();
	size_t i;
	uint64_t valid_sasl_frame_descriptor_ulongs[] = { SASL_MECHANISMS, SASL_INIT, SASL_CHALLENGE, SASL_RESPONSE, SASL_OUTCOME };

	for (i = 0; i < sizeof(valid_sasl_frame_descriptor_ulongs) / sizeof(valid_sasl_frame_descriptor_ulongs[0]); i++)
	{
		mocks.ResetAllCalls();

		size_t sasl_frame_value_size = 2;
		sasl_frame_descriptor_ulong = valid_sasl_frame_descriptor_ulongs[i];
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
			.CopyOutArgumentBuffer(2, &sasl_frame_descriptor_ulong, sizeof(sasl_frame_descriptor_ulong));
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
			.CopyOutArgumentBuffer(2, &sasl_frame_value_size, sizeof(sasl_frame_value_size));
		STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, sasl_frame_value_size, NULL, 0));
		EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.ValidateArgument(1);
		STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, test_encoded_bytes_size))
			.ValidateArgumentBuffer(2, test_encoded_bytes, test_encoded_bytes_size);

		// act
		int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

		// assert
		ASSERT_ARE_EQUAL(int, 0, result);
		mocks.AssertActualAndExpectedCalls();
	}

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_011: [A SASL frame has a type code of 0x01.] */
TEST_FUNCTION(the_SASL_frame_type_is_according_to_ISO)
{
	// arrange
	// act

	// assert
	ASSERT_ARE_EQUAL(int, (int)1, FRAME_TYPE_SASL);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_016: [The maximum size of a SASL frame is defined by MIN-MAX-FRAME-SIZE.] */
TEST_FUNCTION(when_encoding_a_sasl_frame_value_that_makes_the_frame_be_the_max_size_sasl_frame_codec_encode_frame_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	unsigned char encoded_bytes[TEST_MIX_MAX_FRAME_SIZE - 8] = { 0 };
	test_encoded_bytes = encoded_bytes;
	test_encoded_bytes_size = sizeof(encoded_bytes);
	size_t sasl_frame_value_size = test_encoded_bytes_size;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_value_size, sizeof(sasl_frame_value_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_SASL, sasl_frame_value_size, NULL, 0));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, test_encoded_bytes_size))
		.ValidateArgumentBuffer(2, test_encoded_bytes, test_encoded_bytes_size);

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_SASL_FRAME_CODEC_01_016: [The maximum size of a SASL frame is defined by MIN-MAX-FRAME-SIZE.] */
TEST_FUNCTION(when_encoding_a_sasl_frame_value_that_makes_the_frame_exceed_the_allowed_size_sasl_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	unsigned char encoded_bytes[TEST_MIX_MAX_FRAME_SIZE - 8 + 1] = { 0 };
	test_encoded_bytes = encoded_bytes;
	test_encoded_bytes_size = sizeof(encoded_bytes);
	size_t sasl_frame_value_size = test_encoded_bytes_size;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_value_size, sizeof(sasl_frame_value_size));

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_the_sasl_frame_value_has_a_descriptor_ulong_lower_than_MECHANISMS_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	sasl_frame_descriptor_ulong = SASL_MECHANISMS - 1;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_descriptor_ulong, sizeof(sasl_frame_descriptor_ulong));

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_the_sasl_frame_value_has_a_descriptor_ulong_higher_than_OUTCOME_frame_codec_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	sasl_frame_descriptor_ulong = SASL_OUTCOME + 1;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &sasl_frame_descriptor_ulong, sizeof(sasl_frame_descriptor_ulong));

	// act
	int result = sasl_frame_codec_encode_frame(sasl_frame_codec, TEST_AMQP_VALUE);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

/* Receive frames */

/* Tests_SRS_SASL_FRAME_CODEC_01_039: [sasl_frame_codec shall decode the sasl-frame value as a described type.] */
TEST_FUNCTION(when_performative_bytes_are_expected_no_callback_is_triggered)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	amqp_definitions_mocks amqp_definitions_mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).ExpectedTimesExactly(sizeof(test_sasl_frame_value));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(amqp_definitions_mocks, is_sasl_mechanisms_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, TEST_AMQP_VALUE));

	// act
	int result = saved_frame_begin_callback(saved_callback_context, NULL, 0, test_sasl_frame_value,  sizeof(test_sasl_frame_value));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	sasl_frame_codec_destroy(sasl_frame_codec);
}

#if 0
/* Tests_SRS_AMQP_FRAME_CODEC_01_052: [Decoding the performative shall be done by feeding the bytes to the decoder create in sasl_frame_codec_create.] */
TEST_FUNCTION(when_1_of_all_performative_bytes_is_received_no_callback_is_triggered)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, test_sasl_frame_value, 1))
		.ValidateArgumentBuffer(2, test_sasl_frame_value, 1);

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_052: [Decoding the performative shall be done by feeding the bytes to the decoder create in sasl_frame_codec_create.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_054: [Once the performative is decoded, the callback frame_received_callback shall be called.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_055: [The decoded channel and performative shall be passed to frame_received_callback.]  */
TEST_FUNCTION(when_all_performative_bytes_are_received_and_AMQP_frame_payload_is_0_callback_is_triggered)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	uint64_t descriptor_ulong = 0;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &descriptor_ulong, sizeof(descriptor_ulong));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, 0x4243, TEST_AMQP_VALUE, 0));

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	stringify_bytes(test_sasl_frame_value, sizeof(test_sasl_frame_value), expected_stringified_io);
	stringify_bytes(sasl_frame_value_decoded_bytes, sasl_frame_value_decoded_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_056: [The AMQP frame payload size passed to frame_received_callback shall be computed from the frame payload size received from frame_codec and substracting the performative size.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_057: [The bytes of an AMQP frame shall be allowed to be indicated by the frame_codec layer in multiple calls.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_002: [The frame body is defined as a performative followed by an opaque payload.] */
TEST_FUNCTION(the_amqp_frame_payload_size_is_computed_based_on_the_performative_size)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + 2, channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	uint64_t descriptor_ulong = 0;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &descriptor_ulong, sizeof(descriptor_ulong));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, 0x4243, TEST_AMQP_VALUE, 2));

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	stringify_bytes(test_sasl_frame_value, sizeof(test_sasl_frame_value), expected_stringified_io);
	stringify_bytes(sasl_frame_value_decoded_bytes, sasl_frame_value_decoded_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_058: [If more bytes than expected are indicated, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
TEST_FUNCTION(when_more_bytes_than_expected_are_received_decoding_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) - 1, channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));
	mocks.ResetAllCalls();

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_059: [If the decoding of an AMQP frame is incomplete, but a new one is started, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
TEST_FUNCTION(when_another_frame_decoding_is_started_before_one_is_finished_decoder_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_002: [The frame body is defined as a performative followed by an opaque payload.] */
TEST_FUNCTION(amqp_frame_with_1_payload_bytes_are_reported_via_the_amqp_frame_payload_bytes_received_callback)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + 1, channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_payload_bytes_received_callback_1(TEST_CONTEXT, test_frame_payload_bytes, 1))
		.ValidateArgumentBuffer(2, test_frame_payload_bytes, 1);

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_frame_payload_bytes, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_002: [The frame body is defined as a performative followed by an opaque payload.] */
TEST_FUNCTION(amqp_frame_with_2_payload_bytes_are_reported_via_the_amqp_frame_payload_bytes_received_callback)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_payload_bytes_received_callback_1(TEST_CONTEXT, test_frame_payload_bytes, sizeof(test_frame_payload_bytes)))
		.ValidateArgumentBuffer(2, test_frame_payload_bytes, 2);

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_frame_payload_bytes, sizeof(test_frame_payload_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_002: [The frame body is defined as a performative followed by an opaque payload.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_004: [The remaining bytes in the frame body form the payload for that frame.] */
TEST_FUNCTION(when_performative_and_payload_are_received_in_one_chunk_decoding_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	unsigned char all_bytes[sizeof(test_sasl_frame_value) + sizeof(test_frame_payload_bytes)];
	(void)memcpy(all_bytes, test_sasl_frame_value, sizeof(test_sasl_frame_value));
	(void)memcpy(all_bytes + sizeof(test_sasl_frame_value), test_frame_payload_bytes, sizeof(test_frame_payload_bytes));
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(all_bytes), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	uint64_t descriptor_ulong = 0;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &descriptor_ulong, sizeof(descriptor_ulong));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, 0x4243, TEST_AMQP_VALUE, 2));

	STRICT_EXPECTED_CALL(mocks, amqp_frame_payload_bytes_received_callback_1(TEST_CONTEXT, test_frame_payload_bytes, sizeof(test_frame_payload_bytes)))
		.ValidateArgumentBuffer(2, test_frame_payload_bytes, sizeof(test_frame_payload_bytes));

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, all_bytes, sizeof(all_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_002: [The frame body is defined as a performative followed by an opaque payload.] */
TEST_FUNCTION(after_decoding_succesfully_a_complete_frame_a_new_one_can_be_started)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_frame_payload_bytes, sizeof(test_frame_payload_bytes));
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	uint64_t descriptor_ulong = 0;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &descriptor_ulong, sizeof(descriptor_ulong));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, 0x4243, TEST_AMQP_VALUE, 2));

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_003: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
TEST_FUNCTION(valid_performative_codes_trigger_callbacks)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	size_t i;
	uint64_t valid_performatives[] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 };

	for (i = 0; i < 2/*sizeof(valid_performatives) / sizeof(valid_performatives[0])*/; i++)
	{
		(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
		mocks.ResetAllCalls();

		performative_ulong = valid_performatives[i];
		EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.ValidateArgument(1);
		EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
			.ValidateArgument(1).IgnoreAllCalls();
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
			.CopyOutArgumentBuffer(2, &performative_ulong, sizeof(performative_ulong));
		STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, 0x4243, TEST_AMQP_VALUE, 0));

		// act
		int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

		// assert
		ASSERT_ARE_EQUAL(int, 0, result);
		mocks.AssertActualAndExpectedCalls();
	}
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_003: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
TEST_FUNCTION(performative_0x09_can_not_be_decoded)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	performative_ulong = 0x09;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_ulong, sizeof(performative_ulong));

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_003: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
TEST_FUNCTION(performative_0x19_can_not_be_decoded)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	performative_ulong = 0x19;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_ulong, sizeof(performative_ulong));

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_060: [If any error occurs while decoding a frame, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
TEST_FUNCTION(when_getting_the_descriptor_fails_decoder_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	performative_ulong = 0x19;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE))
		.SetReturn((AMQP_VALUE)NULL);
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_060: [If any error occurs while decoding a frame, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
TEST_FUNCTION(when_getting_the_ulong_value_of_the_descriptor_fails_decoder_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	performative_ulong = 0x19;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_ulong, sizeof(performative_ulong))
		.SetReturn(1);
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_sasl_frame_value, sizeof(test_sasl_frame_value));

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_sasl_frame_value) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}
#endif

END_TEST_SUITE(sasl_frame_codec_unittests)
