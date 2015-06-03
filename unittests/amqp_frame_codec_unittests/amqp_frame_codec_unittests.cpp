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

TYPED_MOCK_CLASS(amqp_frame_codec_mocks, CGlobalMock)
{
public:
	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* frame received callback */
	MOCK_STATIC_METHOD_4(, void, frame_begin_callback_1, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, frame_body_bytes_received_callback_1, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, void, frame_begin_callback_2, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, frame_body_bytes_received_callback_2, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size)
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
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, frame_codec_unsubscribe, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_5(, int, frame_codec_begin_encode_frame, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, uint32_t, frame_body_size, const unsigned char*, type_specific_bytes, uint32_t, type_specific_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, frame_codec_encode_frame_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, bytes, size_t, length);
	MOCK_METHOD_END(int, 0);

	/* decoder mocks */
	MOCK_STATIC_METHOD_2(, DECODER_HANDLE, decoder_create, VALUE_DECODED_CALLBACK, value_decoded_callback, void*, value_decoded_callback_context);
	MOCK_METHOD_END(DECODER_HANDLE, TEST_DECODER_HANDLE);
	MOCK_STATIC_METHOD_1(, void, decoder_destroy, DECODER_HANDLE, handle);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, decoder_decode_bytes, DECODER_HANDLE, handle, const unsigned char*, buffer, size_t, size);
	MOCK_METHOD_END(int, 0);

	/* encoder mocks */
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, amqpvalue_encode, AMQP_VALUE, value, ENCODER_OUTPUT, encoder_output, void*, context);
		encoder_output(context, test_encoded_bytes, sizeof(test_encoded_bytes));
	MOCK_METHOD_END(int, 0);

	/* callbacks */
	MOCK_STATIC_METHOD_2(, void, amqp_empty_frame_received_callback, void*, context, uint16_t, channel);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_5(, void, amqp_frame_received_callback, void*, context, uint16_t, channel, uint64_t, performative, AMQP_VALUE, performative_fields, uint32_t, frame_payload_size);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, amqp_frame_payload_bytes_received_callback, void*, context, const unsigned char*, payload_bytes, uint32_t, byte_count);
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_4(amqp_frame_codec_mocks, , void, frame_begin_callback_1, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , void, frame_body_bytes_received_callback_1, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size);
	DECLARE_GLOBAL_MOCK_METHOD_4(amqp_frame_codec_mocks, , void, frame_begin_callback_2, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , void, frame_body_bytes_received_callback_2, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size);

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

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , void, amqp_empty_frame_received_callback, void*, context, uint16_t, channel);
	DECLARE_GLOBAL_MOCK_METHOD_5(amqp_frame_codec_mocks, , void, amqp_frame_received_callback, void*, context, uint16_t, channel, uint64_t, performative, AMQP_VALUE, performative_fields, uint32_t, frame_payload_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , void, amqp_frame_payload_bytes_received_callback, void*, context, const unsigned char*, payload_bytes, uint32_t, byte_count);

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
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NOT_NULL(frame_codec);
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
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, NULL);

	// assert
	ASSERT_IS_NOT_NULL(frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_frame_codec_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(NULL, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_frame_received_callback_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, NULL, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_empty_frame_received_callback_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, NULL, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
TEST_METHOD(amqp_frame_codec_create_with_NULL_frame_payload_bytes_received_callback_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	// act
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, NULL, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
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
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
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
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_020: [If allocating memory for the new amqp_frame_codec fails, then amqp_frame_codec_create shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_for_amqp_frame_codec_fails_then_amqp_frame_codec_create_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* amqp_frame_codec_destroy */

/* Tests_SRS_AMQP_FRAME_CODEC_01_015: [amqp_frame_codec_destroy shall free all resources associated with the amqp_frame_codec instance.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_021: [The decoder created in amqp_frame_codec_create shall be destroyed by amqp_frame_codec_destroy.] */
/* Tests_SRS_AMQP_FRAME_CODEC_01_017: [amqp_frame_codec_destroy shall unsubscribe from receiving AMQP frames from the frame_codec that was passed to amqp_frame_codec_create.] */
TEST_METHOD(amqp_frame_codec_destroy_frees_the_decoder_and_unsubscribes_from_AMQP_frames)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP));
	STRICT_EXPECTED_CALL(mocks, decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	amqp_frame_codec_destroy(frame_codec);

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
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_unsubscribe(TEST_FRAME_CODEC_HANDLE, FRAME_TYPE_AMQP))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, decoder_destroy(TEST_DECODER_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	amqp_frame_codec_destroy(frame_codec);

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
TEST_METHOD(encoding_the_beginning_of_a_frame_succeeds)
{
	// arrange
	// arrange
	amqp_frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
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
	int result = amqp_frame_codec_begin_encode_frame(frame_codec, channel, TEST_AMQP_VALUE, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_026: [The payload frame size shall be computed based on the encoded size of the performative and its fields plus the payload_size argument.] */
TEST_METHOD(encoding_the_beginning_of_a_frame_with_1_byte_payload_computes_correct_payload_size)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
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
	int result = amqp_frame_codec_begin_encode_frame(frame_codec, channel, TEST_AMQP_VALUE, 1);

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
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = amqp_frame_codec_begin_encode_frame(frame_codec, 0, NULL, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_amqpvalue_get_encoded_size_fails_then_amqp_frame_codec_begin_encode_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
	mocks.ResetAllCalls();

	size_t performative_size = 2;
	uint16_t channel = 0;
	unsigned char channel_bytes[] = { 0, 0 };
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_AMQP_VALUE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &performative_size, sizeof(performative_size))
		.SetReturn(1);

	// act
	int result = amqp_frame_codec_begin_encode_frame(frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_frame_codec_begin_encode_frame_fails_then_amqp_frame_codec_begin_encode_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
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
	int result = amqp_frame_codec_begin_encode_frame(frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_amqpvalue_encode_fails_then_amqp_frame_codec_begin_encode_frame_fails)
{
	// arrange
	amqp_frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, amqp_frame_received_callback, amqp_empty_frame_received_callback, amqp_frame_payload_bytes_received_callback, TEST_CONTEXT);
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
	int result = amqp_frame_codec_begin_encode_frame(frame_codec, channel, TEST_AMQP_VALUE, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

END_TEST_SUITE(frame_codec_unittests)
