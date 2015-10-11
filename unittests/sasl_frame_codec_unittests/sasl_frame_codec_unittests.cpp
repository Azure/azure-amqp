#include <cstdint>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "amqpvalue.h"
#include "sasl_frame_codec.h"
#include "frame_codec.h"

#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4242
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4243
#define TEST_DECODER_HANDLE				(AMQPVALUE_DECODER_HANDLE)0x4244
#define TEST_ENCODER_HANDLE				(ENCODER_HANDLE)0x4245
#define TEST_AMQP_VALUE					(AMQP_VALUE)0x4246
#define TEST_CONTEXT					(void*)0x4247

static const unsigned char test_encoded_bytes[2] = { 0x42, 0x43 };

static FRAME_RECEIVED_CALLBACK saved_frame_begin_callback;
static void* saved_callback_context;

static VALUE_DECODED_CALLBACK saved_value_decoded_callback;
static void* saved_value_decoded_callback_context;
static size_t total_bytes;

static unsigned char test_performative[] = { 0x42, 0x43, 0x44 };
static unsigned char* performative_decoded_bytes;
static size_t performative_decoded_byte_count;

static char expected_stringified_io[8192];
static char actual_stringified_io[8192];
static uint64_t performative_ulong;

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
		*ulong_value = performative_ulong;
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
		unsigned char* new_bytes = (unsigned char*)realloc(performative_decoded_bytes, performative_decoded_byte_count + size);
		int my_result = 0;
		if (new_bytes != NULL)
		{
			performative_decoded_bytes = new_bytes;
			(void)memcpy(performative_decoded_bytes + performative_decoded_byte_count, buffer, size);
			performative_decoded_byte_count += size;
		}
		total_bytes += size;
		if (total_bytes == sizeof(test_performative))
		{
			saved_value_decoded_callback(saved_value_decoded_callback_context, TEST_AMQP_VALUE);
			total_bytes = 0;
		}
		MOCK_METHOD_END(int, 0);

	/* encoder mocks */
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, amqpvalue_encode, AMQP_VALUE, value, AMQPVALUE_ENCODER_OUTPUT, encoder_output, void*, context);
		encoder_output(context, test_encoded_bytes, sizeof(test_encoded_bytes));
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
	performative_ulong = 0x10;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
	if (performative_decoded_bytes != NULL)
	{
		free(performative_decoded_bytes);
		performative_decoded_bytes = NULL;
	}
	performative_decoded_byte_count = 0;
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
}

#if 0
/* Tests_SRS_AMQP_FRAME_CODEC_01_011: [sasl_frame_codec_create shall create an instance of an sasl_frame_codec and return a non-NULL handle to it.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_013: [sasl_frame_codec_create shall subscribe for AMQP frames with the given frame_codec.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_018: [sasl_frame_codec_create shall create a decoder to be used for decoding AMQP values.] */
TEST_FUNCTION(sasl_frame_codec_create_with_valid_args_and_NULL_context_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4).IgnoreArgument(5);

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, NULL);

	// assert
	ASSERT_IS_NOT_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
TEST_FUNCTION(sasl_frame_codec_create_with_NULL_frame_codec_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(NULL, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
TEST_FUNCTION(sasl_frame_codec_create_with_NULL_frame_received_callback_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, NULL, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
TEST_FUNCTION(sasl_frame_codec_create_with_NULL_empty_frame_received_callback_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, NULL, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
TEST_FUNCTION(sasl_frame_codec_create_with_NULL_frame_payload_bytes_received_callback_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, NULL, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_014: [If subscribing for AMQP frames fails, sasl_frame_codec_create shall fail and return NULL.] */
TEST_FUNCTION(when_frame_codec_subscribe_fails_then_sasl_frame_codec_create_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4).IgnoreArgument(5)
		.SetReturn(1);

	STRICT_EXPECTED_CALL(mocks, amqpvalue_decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_019: [If creating the decoder fails, sasl_frame_codec_create shall fail and return NULL.] */
TEST_FUNCTION(when_creating_the_decoder_fails_then_sasl_frame_codec_create_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	EXPECTED_CALL(mocks, amqpvalue_decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.SetReturn((AMQPVALUE_DECODER_HANDLE)NULL);

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_020: [If allocating memory for the new sasl_frame_codec fails, then sasl_frame_codec_create shall fail and return NULL.] */
TEST_FUNCTION(when_allocating_memory_for_sasl_frame_codec_fails_then_sasl_frame_codec_create_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(sasl_frame_codec);
}

/* sasl_frame_codec_destroy */

/* Tests_SRS_AMQP_FRAME_CODEC_01_015: [sasl_frame_codec_destroy shall free all resources associated with the sasl_frame_codec instance.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_021: [The decoder created in sasl_frame_codec_create shall be destroyed by sasl_frame_codec_destroy.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_017: [sasl_frame_codec_destroy shall unsubscribe from receiving AMQP frames from the frame_codec that was passed to sasl_frame_codec_create.] */
TEST_FUNCTION(sasl_frame_codec_destroy_frees_the_decoder_and_unsubscribes_from_AMQP_frames)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	sasl_frame_codec_destroy(sasl_frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_015: [sasl_frame_codec_destroy shall free all resources associated with the sasl_frame_codec instance.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_021: [The decoder created in sasl_frame_codec_create shall be destroyed by sasl_frame_codec_destroy.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_017: [sasl_frame_codec_destroy shall unsubscribe from receiving AMQP frames from the frame_codec that was passed to sasl_frame_codec_create.] */
TEST_FUNCTION(when_unsubscribe_fails_sasl_frame_codec_destroy_still_frees_everything)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	sasl_frame_codec_destroy(sasl_frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_016: [If sasl_frame_codec is NULL, sasl_frame_codec_destroy shall do nothing.] */
TEST_FUNCTION(sasl_frame_codec_destroy_with_NULL_handle_does_nothing)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	sasl_frame_codec_destroy(NULL);

	// assert
	// uMock checks the calls
}

/* sasl_frame_codec_begin_encode_frame */

/* Tests_SRS_AMQP_FRAME_CODEC_01_022: [sasl_frame_codec_begin_encode_frame shall encode the frame header and AMQP performative in an AMQP frame and on success it shall return 0.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_025: [sasl_frame_codec_begin_encode_frame shall encode the frame header by using frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_026: [The payload frame size shall be computed based on the encoded size of the performative and its fields plus the payload_size argument.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_027: [The encoded size of the performative and its fields shall be obtained by calling amqpvalue_get_encoded_size.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_030: [Encoding of the AMQP performative and its fields shall be done by calling amqpvalue_encode.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_028: [The encode result for the performative and its fields shall be given to frame_codec by calling frame_codec_encode_frame_bytes.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_005: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_006: [The frame body is defined as a performative followed by an opaque payload.] */
TEST_FUNCTION(encoding_the_beginning_of_a_frame_succeeds)
{
	// arrange
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_005: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
TEST_FUNCTION(using_channel_no_0x4243_passes_the_channel_number_as_type_specific_bytes)
{
	// arrange
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0x4243;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_026: [The payload frame size shall be computed based on the encoded size of the performative and its fields plus the payload_size argument.] */
TEST_FUNCTION(encoding_the_beginning_of_a_frame_with_1_byte_payload_computes_correct_payload_size)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_024: [If frame_codec or performative_fields is NULL, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_begin_encode_frame_with_NULL_sasl_frame_codec_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;

	// act
	int result = sasl_frame_codec_begin_encode_frame(NULL, 0, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_024: [If frame_codec or performative_fields is NULL, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_begin_encode_frame_with_NULL_performative_value_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, NULL, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_amqpvalue_get_encoded_size_fails_then_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size))
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_frame_codec_begin_encode_frame_fails_then_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes))
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_amqpvalue_encode_fails_then_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_008: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_008: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
TEST_FUNCTION(amqp_performatives_are_encoded_successfully)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();
	size_t i;
	uint64_t valid_performatives[] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 };

	for (i = 0; i < sizeof(valid_performatives) / sizeof(valid_performatives[0]); i++)
	{
		size_t performative_size = 2;
		uint16_t channel = 0;
		unsigned char channel_bytes[] = { 0, 0 };
		performative_ulong = valid_performatives[i];
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
			.IgnoreArgument(2);
		STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
			.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
		STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size, channel_bytes, sizeof(channel_bytes)))
			.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
		EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
			.ValidateArgument(1);
		STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
			.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

		// act
		int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

		// assert
		ASSERT_ARE_EQUAL(int, 0, result);
	}
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_getting_the_descriptor_fails_then_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	uint16_t channel = 0;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE))
		.SetReturn((AMQP_VALUE)NULL);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(when_getting_the_ulong_value_of_the_descriptor_fails_then_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	uint16_t channel = 0;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_008: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
TEST_FUNCTION(when_performative_ulong_is_0x09_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	performative_ulong = 0x09;
	uint16_t channel = 0;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_008: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the AMQP type system.] */
TEST_FUNCTION(when_performative_ulong_is_0x19_sasl_frame_codec_begin_encode_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	performative_ulong = 0x19;
	uint16_t channel = 0;
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* sasl_frame_codec_encode_payload_bytes */

/* Tests_SRS_AMQP_FRAME_CODEC_01_031: [sasl_frame_codec_encode_payload_bytes shall encode the bytes for a frame that was begun with sasl_frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_032: [On success sasl_frame_codec_encode_payload_bytes shall return 0.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_037: [The bytes shall be passed to frame codec by a call to frame_codec_encode_frame_bytes.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_023: [sasl_frame_codec_begin_encode_frame shall not encode the frame payload bytes, but switch to a state where it expects the bytes to be passed by using sasl_frame_codec_encode_payload_bytes.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_009: [The remaining bytes in the frame body form the payload for that frame.] */
TEST_FUNCTION(sasl_frame_codec_encode_payload_bytes_encodes_the_bytes)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, sizeof(payload_bytes)))
		.ValidateArgumentBuffer(2, &payload_bytes, sizeof(payload_bytes));

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_033: [If sasl_frame_codec or bytes is NULL, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(when_sasl_frame_codec_is_NULL_sasl_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };

	// act
	int result = sasl_frame_codec_encode_payload_bytes(NULL, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_033: [If sasl_frame_codec or bytes is NULL, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(when_bytes_is_NULL_sasl_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, NULL, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_034: [If count is 0, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(when_count_is_zero_sasl_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_047: [If sasl_frame_codec_encode_payload_bytes is called without starting a frame encode, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_encode_payload_bytes_without_starting_a_frame_encode_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_041: [If all bytes for the frame payload were given and sasl_frame_codec_encode_payload_bytes is called again, it shall return a non-zero value, but subsequent encoding attempts shall succeed.] */
TEST_FUNCTION(sasl_frame_codec_encode_payload_bytes_without_starting_a_frame_encode_after_a_succesfull_encode_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	(void)sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_encode_payload_bytes_with_too_many_bytes_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes) + 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_encode_payload_bytes_when_payload_bytes_was_zero_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, 0);
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests__SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, sasl_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_FUNCTION(after_sasl_frame_codec_encode_payload_bytes_beginning_a_new_frame_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, 0);
	(void)sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_035: [sasl_frame_codec shall maintain the number of bytes needed to be sent as the payload for the frame started with sasl_frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_039: [The frame payload bytes for one frame shall be allowed to be given by multiple calls to sasl_frame_codec_encode_payload_bytes.] */
TEST_FUNCTION(sending_the_1st_byte_of_a_payload_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, 1))
		.ValidateArgumentBuffer(2, payload_bytes, 1);

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_035: [sasl_frame_codec shall maintain the number of bytes needed to be sent as the payload for the frame started with sasl_frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_039: [The frame payload bytes for one frame shall be allowed to be given by multiple calls to sasl_frame_codec_encode_payload_bytes.] */
TEST_FUNCTION(sending_the_2nd_byte_of_a_payload_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	(void)sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, 1);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes + 1, 1))
		.ValidateArgumentBuffer(2, payload_bytes + 1, 1);

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes + 1, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
TEST_FUNCTION(when_giving_the_data_to_frame_codec_fails_then_sasl_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, 2))
		.ValidateArgumentBuffer(2, payload_bytes, 2)
		.SetReturn(1);

	(void)sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, 2);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
TEST_FUNCTION(when_giving_the_data_to_frame_codec_fails_subsequent_encode_attempts_fail)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, 2))
		.ValidateArgumentBuffer(2, payload_bytes, 2)
		.SetReturn(1);

	(void)sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, 2);

	// act
	int result = sasl_frame_codec_begin_encode_frame(sasl_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
TEST_FUNCTION(when_beginning_the_frame_fails_encoding_payload_bytes_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_payload_bytes(sasl_frame_codec, payload_bytes, 2);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* sasl_frame_codec_encode_empty_frame */

/* Tests_SRS_AMQP_FRAME_CODEC_01_042: [sasl_frame_codec_encode_empty_frame shall encode a frame with no payload.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_043: [On success, sasl_frame_codec_encode_empty_frame shall return 0.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_044: [sasl_frame_codec_encode_empty_frame shall use frame_codec_begin_encode_frame to send the frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_010: [An AMQP frame with no body MAY be used to generate artificial traffic as needed to satisfy any negotiated idle timeout interval ] */
TEST_FUNCTION(encoding_of_an_empty_frame_succeeds)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, 0, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));

	// act
	int result = sasl_frame_codec_encode_empty_frame(sasl_frame_codec, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_045: [If sasl_frame_codec is NULL, sasl_frame_codec_encode_empty_frame shall fail and return a non-zero value.] */
TEST_FUNCTION(sasl_frame_codec_encode_empty_frame_with_NULL_sasl_frame_codec_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = sasl_frame_codec_encode_empty_frame(NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_046: [If encoding fails in any way, sasl_frame_codec_encode_empty_frame shall fail and return a non-zero value.]  */
TEST_FUNCTION(when_frame_codec_begin_frame_fails_then_sasl_frame_codec_encode_empty_frame_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, 0, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes))
		.SetReturn(1);

	// act
	int result = sasl_frame_codec_encode_empty_frame(sasl_frame_codec, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Receive frames */

/* Tests_SRS_AMQP_FRAME_CODEC_01_048: [When a frame header is received from frame_codec and the frame payload size is 0, empty_frame_received_callback shall be invoked, while passing the channel number as argument.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_007: [An AMQP frame with no body MAY be used to generate artificial traffic as needed to satisfy any negotiated idle timeout interval ] */
TEST_FUNCTION(when_an_empty_frame_is_decoded_the_empty_frame_callback_is_called)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_empty_frame_received_callback_1(TEST_CONTEXT, 0));

	unsigned char channel_bytes[] = { 0, 0 };

	// act
	int result = saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_048: [When a frame header is received from frame_codec and the frame payload size is 0, empty_frame_received_callback shall be invoked, while passing the channel number as argument.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_001: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
TEST_FUNCTION(when_an_empty_frame_is_decoded_the_empty_frame_callback_is_called_and_the_channel_number_is_passed_to_the_callback)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_empty_frame_received_callback_1(TEST_CONTEXT, 0x4243));

	unsigned char channel_bytes[] = { 0x42, 0x43 };

	// act
	int result = saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_049: [If not enough type specific bytes are received to decode the channel number, the decoding shall stop with an error.] */
TEST_FUNCTION(when_an_empty_frame_with_only_1_byte_of_type_specific_data_is_received_decoding_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	unsigned char channel_bytes[] = { 0x42, 0x43 };

	// act
	int result = saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_050: [All subsequent decoding shall fail and no AMQP frames shall be indicated from that point on to the consumers of sasl_frame_codec.] */
TEST_FUNCTION(when_an_empty_frame_with_only_1_byte_of_type_specific_data_is_received_decoding_fails_and_subsequent_decodes_fail_too)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, 1);
	mocks.ResetAllCalls();

	// act
	int result = saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_051: [If the frame payload is greater than 0, sasl_frame_codec shall decode the performative as a described AMQP type.] */
TEST_FUNCTION(when_performative_bytes_are_expected_no_callback_is_triggered)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_052: [Decoding the performative shall be done by feeding the bytes to the decoder create in sasl_frame_codec_create.] */
TEST_FUNCTION(when_1_of_all_performative_bytes_is_received_no_callback_is_triggered)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, test_performative, 1))
		.ValidateArgumentBuffer(2, test_performative, 1);

	// act
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, 1);

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
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
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	stringify_bytes(test_performative, sizeof(test_performative), expected_stringified_io);
	stringify_bytes(performative_decoded_bytes, performative_decoded_byte_count, actual_stringified_io);
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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + 2, channel_bytes, sizeof(channel_bytes));
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
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	stringify_bytes(test_performative, sizeof(test_performative), expected_stringified_io);
	stringify_bytes(performative_decoded_bytes, performative_decoded_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_058: [If more bytes than expected are indicated, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
TEST_FUNCTION(when_more_bytes_than_expected_are_received_decoding_fails)
{
	// arrange
	sasl_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec = sasl_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) - 1, channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));
	mocks.ResetAllCalls();

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + 1, channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));
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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));
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
	unsigned char all_bytes[sizeof(test_performative) + sizeof(test_frame_payload_bytes)];
	(void)memcpy(all_bytes, test_performative, sizeof(test_performative));
	(void)memcpy(all_bytes + sizeof(test_performative), test_frame_payload_bytes, sizeof(test_frame_payload_bytes));
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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_frame_payload_bytes, sizeof(test_frame_payload_bytes));
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));
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
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

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
		(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
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
		int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
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
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
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
	int result = saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	performative_ulong = 0x19;
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpvalue_decode_bytes(TEST_DECODER_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1).IgnoreAllCalls();
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_AMQP_VALUE))
		.SetReturn((AMQP_VALUE)NULL);
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));

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
	(void)saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
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
	(void)saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

	// act
	int result = saved_frame_begin_callback(saved_callback_context, sizeof(test_performative) + sizeof(test_frame_payload_bytes), channel_bytes, sizeof(channel_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}
#endif

END_TEST_SUITE(sasl_frame_codec_unittests)
