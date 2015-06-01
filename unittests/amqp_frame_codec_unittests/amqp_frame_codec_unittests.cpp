#include <cstdint>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "amqpvalue.h"
#include "amqp_frame_codec.h"
#include "frame_codec.h"
#include "encoder.h"

#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4242
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4243
#define TEST_DECODER_HANDLE				(DECODER_HANDLE)0x4244
#define TEST_ENCODER_HANDLE				(ENCODER_HANDLE)0x4245
#define TEST_AMQP_VALUE					(AMQP_VALUE)0x4246

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
	MOCK_STATIC_METHOD_2(, ENCODER_HANDLE, encoder_create, ENCODER_OUTPUT, encoderOutput, void*, context);
	MOCK_METHOD_END(ENCODER_HANDLE, TEST_ENCODER_HANDLE);
	MOCK_STATIC_METHOD_1(, void, encoder_destroy, ENCODER_HANDLE, handle);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_2(, int, encoder_get_encoded_size, ENCODER_HANDLE, handle, size_t*, size);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, encoder_encode_amqp_value, ENCODER_HANDLE, handle, AMQP_VALUE, value);
	MOCK_METHOD_END(int, 0);
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
	DECLARE_GLOBAL_MOCK_METHOD_5(amqp_frame_codec_mocks, , int, frame_codec_begin_encode_frame, FRAME_CODEC_HANDLE, frame_codec, uint8_t, type, uint32_t, frame_body_size, const unsigned char*, type_specific_bytes, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , int, frame_codec_encode_frame_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, bytes, size_t, length);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , DECODER_HANDLE, decoder_create, VALUE_DECODED_CALLBACK, value_decoded_callback, void*, value_decoded_callback_context);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, decoder_destroy, DECODER_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_frame_codec_mocks, , int, decoder_decode_bytes, DECODER_HANDLE, handle, const unsigned char*, buffer, size_t, size);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , ENCODER_HANDLE, encoder_create, ENCODER_OUTPUT, encoderOutput, void*, context);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_frame_codec_mocks, , void, encoder_destroy, ENCODER_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , int, encoder_get_encoded_size, ENCODER_HANDLE, handle, size_t*, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_frame_codec_mocks, , int, encoder_encode_amqp_value, ENCODER_HANDLE, handle, AMQP_VALUE, value);

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

/* Tests_SRS_FRAME_CODEC_01_021: [frame_codec_create shall create a new instance of frame_codec and return a non-NULL handle to it on success.] */
TEST_METHOD(amqp_frame_codec_create_with_valid_args_succeeds)
{
	// arrange
	amqp_frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	// act
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_FRAME_CODEC_HANDLE, consolelogger_log);

	// assert
	ASSERT_IS_NOT_NULL(frame_codec);
}

END_TEST_SUITE(frame_codec_unittests)
