#include <cstdint>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "amqpvalue.h"
#include "amqp_frame_codec.h"
#include "frame_codec.h"

#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4242
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4243
#define TEST_DECODER_HANDLE				(DECODER_HANDLE)0x4244
#define TEST_ENCODER_HANDLE				(ENCODER_HANDLE)0x4245
#define TEST_AMQP_VALUE					(AMQP_VALUE)0x4246
#define TEST_CONTEXT					(void*)0x4247

static const unsigned char test_encoded_bytes[2] = { 0x42, 0x43 };

static FRAME_BEGIN_CALLBACK saved_frame_begin_callback;
static FRAME_BODY_BYTES_RECEIVED_CALLBACK saved_frame_body_bytes_received_callback;
static void* saved_callback_context;

static VALUE_DECODED_CALLBACK saved_value_decoded_callback;
static void* saved_value_decoded_callback_context;
static size_t total_bytes;

static unsigned char test_performative[] = { 0x42, 0x43, 0x44 };

TYPED_MOCK_CLASS(amqp_frame_codec_mocks, CGlobalMock)
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
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_get_descriptor, AMQP_VALUE, value)
	MOCK_METHOD_END(AMQP_VALUE, TEST_DESCRIPTOR_AMQP_VALUE);
	MOCK_STATIC_METHOD_1(, void, amqpvalue_destroy, AMQP_VALUE, value)
	MOCK_VOID_METHOD_END();

	/* frame_codec mocks */
	MOCK_STATIC_METHOD_2(, FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, LOGGER_LOG, logger_log);
	MOCK_METHOD_END(FRAME_CODEC_HANDLE, TEST_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_5(, int, frame_codec_subscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, FRAME_BEGIN_CALLBACK, frame_begin_callback, FRAME_BODY_BYTES_RECEIVED_CALLBACK, frame_body_bytes_received_callback, void*, callback_context);
		saved_frame_begin_callback = frame_begin_callback;
		saved_frame_body_bytes_received_callback = frame_body_bytes_received_callback;
		saved_callback_context = callback_context;
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, frame_codec_unsubscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_5(, int, frame_codec_begin_encode_frame, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, uint32_t, frame_body_size, const unsigned char*, type_specific_bytes, uint32_t, type_specific_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, frame_codec_encode_frame_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, bytes, size_t, length);
	MOCK_METHOD_END(int, 0);

	/* decoder mocks */
	MOCK_STATIC_METHOD_2(, DECODER_HANDLE, decoder_create, VALUE_DECODED_CALLBACK, value_decoded_callback, void*, value_decoded_callback_context);
		saved_value_decoded_callback = value_decoded_callback;
		saved_value_decoded_callback_context = value_decoded_callback_context;
		total_bytes = 0;
	MOCK_METHOD_END(DECODER_HANDLE, TEST_DECODER_HANDLE);
	MOCK_STATIC_METHOD_1(, void, decoder_destroy, DECODER_HANDLE, handle);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, decoder_decode_bytes, DECODER_HANDLE, handle, const unsigned char*, buffer, size_t, size);
		total_bytes += size;
		if (total_bytes == sizeof(test_performative))
		{
			saved_value_decoded_callback(saved_value_decoded_callback_context, TEST_AMQP_VALUE);
		}
	MOCK_METHOD_END(int, 0);

	/* encoder mocks */
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, amqpvalue_encode, AMQP_VALUE, value, ENCODER_OUTPUT, encoder_output, void*, context);
		encoder_output(context, test_encoded_bytes, sizeof(test_encoded_bytes));
	MOCK_METHOD_END(int, 0);

	/* callbacks */
	MOCK_STATIC_METHOD_2(, void, amqp_empty_frame_received_callback_1, void*, context, uint16_t, channel);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, void, amqp_frame_received_callback_1, void*, context, uint16_t, channel, AMQP_VALUE, performative, uint32_t, frame_payload_size);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, amqp_frame_payload_bytes_received_callback_1, void*, context, const unsigned char*, payload_bytes, uint32_t, byte_count);
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , AMQP_VALUE, amqpvalue_create_ulong, uint64_t, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , AMQP_VALUE, amqpvalue_create_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , AMQP_VALUE, amqpvalue_get_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, amqpvalue_destroy, AMQP_VALUE, value);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec);
	DECLARE_GLOBAL_MOCK_METHOD_5(amqp_frame_codec_mocks, , int, frame_codec_subscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, FRAME_BEGIN_CALLBACK, frame_begin_callback, FRAME_BODY_BYTES_RECEIVED_CALLBACK, frame_body_bytes_received_callback, void*, callback_context);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , int, frame_codec_unsubscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type);
	DECLARE_GLOBAL_MOCK_METHOD_5(amqp_frame_codec_mocks, , int, frame_codec_begin_encode_frame, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, uint32_t, frame_body_size, const unsigned char*, type_specific_bytes, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , int, frame_codec_encode_frame_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, bytes, size_t, length);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , DECODER_HANDLE, decoder_create, VALUE_DECODED_CALLBACK, value_decoded_callback, void*, value_decoded_callback_context);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, decoder_destroy, DECODER_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , int, decoder_decode_bytes, DECODER_HANDLE, handle, const unsigned char*, buffer, size_t, size);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , int, amqpvalue_encode, AMQP_VALUE, value, ENCODER_OUTPUT, encoder_output, void*, context);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , void, amqp_empty_frame_received_callback_1, void*, context, uint16_t, channel);
	DECLARE_GLOBAL_MOCK_METHOD_4(amqp_frame_codec_mocks, , void, amqp_frame_received_callback_1, void*, context, uint16_t, channel, AMQP_VALUE, performative, uint32_t, frame_payload_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , void, amqp_frame_payload_bytes_received_callback_1, void*, context, const unsigned char*, payload_bytes, uint32_t, byte_count);

	extern void consolelogger_log(char* format, ...)
	{
		(void)format;
	}
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(amqpvalue_unittests)

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
		ASSERT_FAIL("Could not release test serialization mutex.");
	}
}

/* amqp_frame_codec_create */

/* Tests_SRS_AMQP_FRAME_CODEC_01_011: [amqp_frame_codec_create shall create an instance of an amqp_frame_codec and return a non-NULL handle to it.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_013: [amqp_frame_codec_create shall subscribe for AMQP frames with the given frame_codec.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_018: [amqp_frame_codec_create shall create a decoder to be used for decoding AMQP values.] */
TEST_METHOD(amqp_frame_codec_create_with_valid_args_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	EXPECTED_CALL(mocks, decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4).IgnoreArgument(5);

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NOT_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_011: [amqp_frame_codec_create shall create an instance of an amqp_frame_codec and return a non-NULL handle to it.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_013: [amqp_frame_codec_create shall subscribe for AMQP frames with the given frame_codec.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_018: [amqp_frame_codec_create shall create a decoder to be used for decoding AMQP values.] */
TEST_METHOD(amqp_frame_codec_create_with_valid_args_and_NULL_context_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	EXPECTED_CALL(mocks, decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4).IgnoreArgument(5);

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, NULL);

	// assert
	ASSERT_IS_NOT_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_frame_codec_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(NULL, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_frame_received_callback_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, NULL, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_empty_frame_received_callback_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, NULL, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_frame_payload_bytes_received_callback_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, NULL, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_014: [If subscribing for AMQP frames fails, amqp_frame_codec_create shall fail and return NULL.] */
TEST_METHOD(when_frame_codec_subscribe_fails_then_amqp_frame_codec_create_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	EXPECTED_CALL(mocks, decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_codec_subscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(3).IgnoreArgument(4).IgnoreArgument(5)
		.SetReturn(1);

	STRICT_EXPECTED_CALL(mocks, decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_019: [If creating the decoder fails, amqp_frame_codec_create shall fail and return NULL.] */
TEST_METHOD(when_creating_the_decoder_fails_then_amqp_frame_codec_create_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	EXPECTED_CALL(mocks, decoder_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.SetReturn((DECODER_HANDLE)NULL);

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_020: [If allocating memory for the new amqp_frame_codec fails, then amqp_frame_codec_create shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_for_amqp_frame_codec_fails_then_amqp_frame_codec_create_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(amqp_frame_codec);
}

/* amqp_frame_codec_destroy */

/* Tests_SRS_AMQP_FRAME_CODEC_01_015: [amqp_frame_codec_destroy shall free all resources associated with the amqp_frame_codec instance.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_021: [The decoder created in amqp_frame_codec_create shall be destroyed by amqp_frame_codec_destroy.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_017: [amqp_frame_codec_destroy shall unsubscribe from receiving AMQP frames from the frame_codec that was passed to amqp_frame_codec_create.] */
TEST_METHOD(amqp_frame_codec_destroy_frees_the_decoder_and_unsubscribes_from_AMQP_frames)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP));
	STRICT_EXPECTED_CALL(mocks, decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	amqp_frame_codec_destroy(amqp_frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_015: [amqp_frame_codec_destroy shall free all resources associated with the amqp_frame_codec instance.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_021: [The decoder created in amqp_frame_codec_create shall be destroyed by amqp_frame_codec_destroy.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_017: [amqp_frame_codec_destroy shall unsubscribe from receiving AMQP frames from the frame_codec that was passed to amqp_frame_codec_create.] */
TEST_METHOD(when_unsubscribe_fails_amqp_frame_codec_destroy_still_frees_everything)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	amqp_frame_codec_destroy(amqp_frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_016: [If amqp_frame_codec is NULL, amqp_frame_codec_destroy shall do nothing.] */
TEST_METHOD(amqp_frame_codec_destroy_with_NULL_handle_does_nothing)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	amqp_frame_codec_destroy(NULL);

	// assert
	// uMock checks the calls
}

/* amqp_frame_codec_begin_encode_frame */

/* Tests_SRS_AMQP_FRAME_CODEC_01_022: [amqp_frame_codec_begin_encode_frame shall encode the frame header and AMQP performative in an AMQP frame and on success it shall return 0.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_025: [amqp_frame_codec_begin_encode_frame shall encode the frame header by using frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_026: [The payload frame size shall be computed based on the encoded size of the performative and its fields plus the payload_size argument.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_027: [The encoded size of the performative and its fields shall be obtained by calling amqpvalue_get_encoded_size.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_030: [Encoding of the AMQP performative and its fields shall be done by calling amqpvalue_encode.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_028: [The encode result for the performative and its fields shall be given to frame_codec by calling frame_codec_encode_frame_bytes.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_005: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_006: [The frame body is defined as a performative followed by an opaque payload.] */
TEST_METHOD(encoding_the_beginning_of_a_frame_succeeds)
{
	// arrange
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_005: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
TEST_METHOD(using_channel_no_0x4243_passes_the_channel_number_as_type_specific_bytes)
{
	// arrange
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0x4243;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_026: [The payload frame size shall be computed based on the encoded size of the performative and its fields plus the payload_size argument.] */
TEST_METHOD(encoding_the_beginning_of_a_frame_with_1_byte_payload_computes_correct_payload_size)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_024: [If frame_codec or performative_fields is NULL, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(amqp_frame_codec_begin_encode_frame_with_NULL_amqp_frame_codec_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	int result = amqp_frame_codec_begin_encode_frame(NULL, 0, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_024: [If frame_codec or performative_fields is NULL, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(amqp_frame_codec_begin_encode_frame_with_NULL_performative_value_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, NULL, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_amqpvalue_get_encoded_size_fails_then_amqp_frame_codec_begin_encode_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size))
		.SetReturn(1);

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_frame_codec_begin_encode_frame_fails_then_amqp_frame_codec_begin_encode_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes))
		.SetReturn(1);

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_amqpvalue_encode_fails_then_amqp_frame_codec_begin_encode_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
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

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* amqp_frame_codec_encode_payload_bytes */

/* Tests_SRS_AMQP_FRAME_CODEC_01_031: [amqp_frame_codec_encode_payload_bytes shall encode the bytes for a frame that was begun with amqp_frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_032: [On success amqp_frame_codec_encode_payload_bytes shall return 0.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_037: [The bytes shall be passed to frame codec by a call to frame_codec_encode_frame_bytes.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_023: [amqp_frame_codec_begin_encode_frame shall not encode the frame payload bytes, but switch to a state where it expects the bytes to be passed by using amqp_frame_codec_encode_payload_bytes.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_009: [The remaining bytes in the frame body form the payload for that frame.] */
TEST_METHOD(amqp_frame_codec_encode_payload_bytes_encodes_the_bytes)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, sizeof(payload_bytes)))
		.ValidateArgumentBuffer(2, &payload_bytes, sizeof(payload_bytes));

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_033: [If amqp_frame_codec or bytes is NULL, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(when_amqp_frame_codec_is_NULL_amqp_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };

	// act
	int result = amqp_frame_codec_encode_payload_bytes(NULL, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_033: [If amqp_frame_codec or bytes is NULL, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(when_bytes_is_NULL_amqp_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, NULL, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_034: [If count is 0, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(when_count_is_zero_amqp_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_047: [If amqp_frame_codec_encode_payload_bytes is called without starting a frame encode, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(amqp_frame_codec_encode_payload_bytes_without_starting_a_frame_encode_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_041: [If all bytes for the frame payload were given and amqp_frame_codec_encode_payload_bytes is called again, it shall return a non-zero value, but subsequent encoding attempts shall succeed.] */
TEST_METHOD(amqp_frame_codec_encode_payload_bytes_without_starting_a_frame_encode_after_a_succesfull_encode_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	(void)amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_.] SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(amqp_frame_codec_encode_payload_bytes_with_too_many_bytes_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes) + 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(amqp_frame_codec_encode_payload_bytes_when_payload_bytes_was_zero_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, 0);
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests__SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
TEST_METHOD(after_amqp_frame_codec_encode_payload_bytes_beginning_a_new_frame_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, 0);
	(void)amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size));
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, performative_size + 1, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));
	EXPECTED_CALL(mocks, amqpvalue_encode(TEST_AMQP_VALUE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, test_encoded_bytes, sizeof(test_encoded_bytes)))
		.ValidateArgumentBuffer(2, &test_encoded_bytes, sizeof(test_encoded_bytes));

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_035: [amqp_frame_codec shall maintain the number of bytes needed to be sent as the payload for the frame started with amqp_frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_039: [The frame payload bytes for one frame shall be allowed to be given by multiple calls to amqp_frame_codec_encode_payload_bytes.] */
TEST_METHOD(sending_the_1st_byte_of_a_payload_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, 1))
		.ValidateArgumentBuffer(2, payload_bytes, 1);

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_035: [amqp_frame_codec shall maintain the number of bytes needed to be sent as the payload for the frame started with amqp_frame_codec_begin_encode_frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_039: [The frame payload bytes for one frame shall be allowed to be given by multiple calls to amqp_frame_codec_encode_payload_bytes.] */
TEST_METHOD(sending_the_2nd_byte_of_a_payload_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	(void)amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, 1);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes + 1, 1))
		.ValidateArgumentBuffer(2, payload_bytes + 1, 1);

	// act
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes + 1, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
TEST_METHOD(when_giving_the_data_to_frame_codec_fails_then_amqp_frame_codec_encode_payload_bytes_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, 2))
		.ValidateArgumentBuffer(2, payload_bytes, 2)
		.SetReturn(1);

	(void)amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, 2);

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
TEST_METHOD(when_giving_the_data_to_frame_codec_fails_subsequent_encode_attempts_fail)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	(void)amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_encode_frame_bytes(TEST_FRAME_CODEC_HANDLE, payload_bytes, 2))
		.ValidateArgumentBuffer(2, payload_bytes, 2)
		.SetReturn(1);

	(void)amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, 2);

	// act
	int result = amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, TEST_AMQP_VALUE, sizeof(payload_bytes));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
TEST_METHOD(when_beginning_the_frame_fails_encoding_payload_bytes_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
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
	int result = amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payload_bytes, 2);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* amqp_frame_codec_encode_empty_frame */

/* Tests_SRS_AMQP_FRAME_CODEC_01_042: [amqp_frame_codec_encode_empty_frame shall encode a frame with no payload.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_043: [On success, amqp_frame_codec_encode_empty_frame shall return 0.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_044: [amqp_frame_codec_encode_empty_frame shall use frame_codec_begin_encode_frame to send the frame.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_010: [An AMQP frame with no body MAY be used to generate artificial traffic as needed to satisfy any negotiated idle timeout interval ] */
TEST_METHOD(encoding_of_an_empty_frame_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, 0, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes));

	// act
	int result = amqp_frame_codec_encode_empty_frame(amqp_frame_codec, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_045: [If amqp_frame_codec is NULL, amqp_frame_codec_encode_empty_frame shall fail and return a non-zero value.] */
TEST_METHOD(amqp_frame_codec_encode_empty_frame_with_NULL_amqp_frame_codec_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_encode_empty_frame(NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_046: [If encoding fails in any way, amqp_frame_codec_encode_empty_frame shall fail and return a non-zero value.]  */
TEST_METHOD(when_frame_codec_begin_frame_fails_then_amqp_frame_codec_encode_empty_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char payload_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, frame_codec_begin_encode_frame(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP, 0, channel_bytes, sizeof(channel_bytes)))
		.ValidateArgumentBuffer(4, &channel_bytes, sizeof(channel_bytes))
		.SetReturn(1);

	// act
	int result = amqp_frame_codec_encode_empty_frame(amqp_frame_codec, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Receive frames */

/* Tests_SRS_AMQP_FRAME_CODEC_01_048: [When a frame header is received from frame_codec and the frame payload size is 0, empty_frame_received_callback shall be invoked, while passing the channel number as argument.] */
TEST_METHOD(when_an_empty_frame_is_decoded_the_empty_frame_callback_is_called)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_empty_frame_received_callback_1(TEST_CONTEXT, 0));

	unsigned char channel_bytes[] = { 0, 0 };

	// act
	saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, sizeof(channel_bytes));

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_048: [When a frame header is received from frame_codec and the frame payload size is 0, empty_frame_received_callback shall be invoked, while passing the channel number as argument.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_001: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
TEST_METHOD(when_an_empty_frame_is_decoded_the_empty_frame_callback_is_called_and_the_channel_number_is_passed_to_the_callback)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_empty_frame_received_callback_1(TEST_CONTEXT, 0x4243));

	unsigned char channel_bytes[] = { 0x42, 0x43 };

	// act
	saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, sizeof(channel_bytes));

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_049: [If not enough type specific bytes are received to decode the channel number, the decoding shall stop with an error.] */
TEST_METHOD(when_an_empty_frame_with_only_1_byte_of_type_specific_data_is_received_decoding_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	unsigned char channel_bytes[] = { 0x42, 0x43 };

	// act
	saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, 1);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_050: [All subsequent decoding shall fail and no AMQP frames shall be indicated from that point on to the consumers of amqp_frame_codec.] */
TEST_METHOD(when_an_empty_frame_with_only_1_byte_of_type_specific_data_is_received_decoding_fails_and_subsequent_decodes_fail_too)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, 1);
	mocks.ResetAllCalls();

	// act
	saved_frame_begin_callback(saved_callback_context, 0, channel_bytes, sizeof(channel_bytes));

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_051: [If the frame payload is greater than 0, amqp_frame_codec shall decode the performative as a described AMQP type.] */
TEST_METHOD(when_performative_bytes_are_expected_no_callback_is_triggered)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_052: [Decoding the performative shall be done by feeding the bytes to the decoder create in amqp_frame_codec_create.] */
TEST_METHOD(when_1_of_all_performative_bytes_is_received_no_callback_is_triggered)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, decoder_decode_bytes(TEST_DECODER_HANDLE, test_performative, 1))
		.ValidateArgumentBuffer(2, test_performative, 1);

	// act
	saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, 1);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_052: [Decoding the performative shall be done by feeding the bytes to the decoder create in amqp_frame_codec_create.] */
TEST_METHOD(when_all_performative_bytes_are_received_and_AMQP_frame_payload_is_0_callback_is_triggered)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	unsigned char channel_bytes[] = { 0x42, 0x43 };
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback_1, amqp_empty_frame_received_callback_1, amqp_frame_payload_bytes_received_callback_1, TEST_CONTEXT);
	saved_frame_begin_callback(saved_callback_context, sizeof(test_performative), channel_bytes, sizeof(channel_bytes));
	mocks.ResetAllCalls();

	uint64_t descriptor_ulong = 0;
	STRICT_EXPECTED_CALL(mocks, decoder_decode_bytes(TEST_DECODER_HANDLE, test_performative, sizeof(test_performative)))
		.ValidateArgumentBuffer(2, test_performative, sizeof(test_performative));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_descriptor(TEST_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_ulong(TEST_DESCRIPTOR_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &descriptor_ulong, sizeof(descriptor_ulong));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_received_callback_1(TEST_CONTEXT, 0x4243, TEST_AMQP_VALUE, 0));

	// act
	saved_frame_body_bytes_received_callback(saved_callback_context, test_performative, sizeof(test_performative));

	// assert
	// uMock checks the calls
}

END_TEST_SUITE(frame_codec_unittests)
