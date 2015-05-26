#include <cstdint>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "amqpvalue.h"
#include "frame_codec.h"
#include "io.h"
#include "encoder.h"
#include "decoder.h"

#define TEST_IO_HANDLE					(IO_HANDLE)0x4242
#define TEST_DESCRIPTION_AMQP_VALUE		(AMQP_VALUE)0x4243
#define TEST_ENCODER_HANDLE				(ENCODER_HANDLE)0x4244
#define TEST_DECODER_HANDLE				(DECODER_HANDLE)0x4245

const IO_INTERFACE_DESCRIPTION test_io_interface_description = { 0 };

static IO_RECEIVE_CALLBACK io_receive_callback;
static void* io_receive_callback_context;

TYPED_MOCK_CLASS(frame_codec_mocks, CGlobalMock)
{
public:
	/* io mocks */
	MOCK_STATIC_METHOD_5(, IO_HANDLE, io_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context, LOGGER_LOG, logger_log)
		io_receive_callback = receive_callback;
		io_receive_callback_context = receive_callback_context;
	MOCK_METHOD_END(IO_HANDLE, TEST_IO_HANDLE);
	MOCK_STATIC_METHOD_1(, void, io_destroy, IO_HANDLE, handle)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, io_send, IO_HANDLE, handle, const void*, buffer, size_t, size)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, int, io_dowork, IO_HANDLE, handle)
	MOCK_METHOD_END(int, 0);

	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* amqpvalue mocks*/
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_get_descriptor, AMQP_VALUE, value)
	MOCK_METHOD_END(AMQP_VALUE, TEST_DESCRIPTION_AMQP_VALUE);
	MOCK_STATIC_METHOD_1(, void, amqpvalue_destroy, AMQP_VALUE, value)
	MOCK_VOID_METHOD_END();

	/* encoder mocks */
	MOCK_STATIC_METHOD_2(, ENCODER_HANDLE, encoder_create, ENCODER_OUTPUT, encoder_output, void*, context)
	MOCK_METHOD_END(ENCODER_HANDLE, TEST_ENCODER_HANDLE);
	MOCK_STATIC_METHOD_1(, void, encoder_destroy, ENCODER_HANDLE, handle)
	MOCK_VOID_METHOD_END();

	/* decoder mocks */
	MOCK_STATIC_METHOD_2(, DECODER_HANDLE, decoder_create, const void*, buffer, size_t, length)
	MOCK_METHOD_END(DECODER_HANDLE, TEST_DECODER_HANDLE);
	MOCK_STATIC_METHOD_1(, void, decoder_destroy, DECODER_HANDLE, handle)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, decoder_decode, DECODER_HANDLE, handle, AMQP_VALUE*, amqp_value, bool*, more)
	MOCK_METHOD_END(int, 0);

	/* frame received callback */
	MOCK_STATIC_METHOD_6(, void, frame_received_callback, void*, context, uint8_t, type, const unsigned char*, frame_body, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_5(frame_codec_mocks, , IO_HANDLE, io_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, context, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void, io_destroy, IO_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(frame_codec_mocks, , int, io_send, IO_HANDLE, handle, const void*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , int, io_dowork, IO_HANDLE, handle);

	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_2(frame_codec_mocks, , int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , AMQP_VALUE, amqpvalue_get_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void, amqpvalue_destroy, AMQP_VALUE, value);

	DECLARE_GLOBAL_MOCK_METHOD_2(frame_codec_mocks, , ENCODER_HANDLE, encoder_create, ENCODER_OUTPUT, encoder_output, void*, context);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void, encoder_destroy, ENCODER_HANDLE, handle);

	DECLARE_GLOBAL_MOCK_METHOD_2(frame_codec_mocks, , DECODER_HANDLE, decoder_create, const void*, buffer, size_t, length);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void, decoder_destroy, DECODER_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(frame_codec_mocks, , int, decoder_decode, DECODER_HANDLE, handle, AMQP_VALUE*, amqp_value, bool*, more);

	DECLARE_GLOBAL_MOCK_METHOD_6(frame_codec_mocks, , void, frame_received_callback, void*, context, uint8_t, type, const unsigned char*, frame_body, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size)

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

/* frame_codec_create */

/* Tests_SRS_FRAME_CODEC_01_021: [frame_codec_create shall create a new instance of frame_codec and return a non-NULL handle to it on success.] */
TEST_METHOD(frame_codec_create_with_valid_args_succeeds)
{
	// arrange
	frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	// act
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);

	// assert
	ASSERT_IS_NOT_NULL(frame_codec);
}

/* Tests_SRS_FRAME_CODEC_01_020: [If the io argument is NULL, frame_codec_create shall return NULL.] */
TEST_METHOD(when_io_is_NULL_frame_codec_create_fails)
{
	// arrange
	frame_codec_mocks mocks;

	// act
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(NULL, consolelogger_log);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* Tests_SRS_FRAME_CODEC_01_022: [If allocating memory for the frame_codec instance fails, frame_codec_create shall return NULL.] */
TEST_METHOD(when_allocating_emory_for_the_frame_codec_fails_frame_code_create_fails)
{
	// arrange
	frame_codec_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);

	// assert
	ASSERT_IS_NULL(frame_codec);
}

/* frame_codec_destroy */

/* Tests_SRS_FRAME_CODEC_01_023: [frame_codec_destroy shall free all resources associated with a frame_codec instance.] */
TEST_METHOD(frame_codec_destroy_frees_the_memory_for_frame_codec)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	frame_codec_destroy(frame_codec);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_FRAME_CODEC_01_024: [If frame_codec is NULL, frame_codec_destroy shall do nothing.] */
TEST_METHOD(when_frame_codec_is_NULL_frame_codec_destroy_does_nothing)
{
	// arrange
	frame_codec_mocks mocks;

	// act
	frame_codec_destroy(NULL);

	// assert
	// uMock checks the calls
}

/* frame_codec_receive_bytes */

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it returns zero.] */
/* Tests_SRS_FRAME_CODEC_01_031: [When a frame is successfully decoded it shall be indicated to the upper layer by invoking the receive callback passed to frame_codec_create.] */
/* Tests_SRS_FRAME_CODEC_01_032: [Besides passing the frame information, the frame_received_callback_context value passed to frame_codec_create shall be passed to the frame_received_callback function.] */
/* Tests_SRS_FRAME_CODEC_01_001: [Frames are divided into three distinct areas: a fixed width frame header, a variable width extended header, and a variable width frame body.] */
/* Tests_SRS_FRAME_CODEC_01_002: [frame header The frame header is a fixed size (8 byte) structure that precedes each frame.] */
/* Tests_SRS_FRAME_CODEC_01_003: [The frame header includes mandatory information necessary to parse the rest of the frame including size and type information.] */
/* Tests_SRS_FRAME_CODEC_01_004: [extended header The extended header is a variable width area preceding the frame body.] */
/* Tests_SRS_FRAME_CODEC_01_007: [frame body The frame body is a variable width sequence of bytes the format of which depends on the frame type.] */
/* Tests_SRS_FRAME_CODEC_01_008: [SIZE Bytes 0-3 of the frame header contain the frame size.] */
/* Tests_SRS_FRAME_CODEC_01_009: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
/* Tests_SRS_FRAME_CODEC_01_011: [DOFF Byte 4 of the frame header is the data offset.] */
/* Tests_SRS_FRAME_CODEC_01_012: [This gives the position of the body within the frame.] */
/* Tests_SRS_FRAME_CODEC_01_013: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
/* Tests_SRS_FRAME_CODEC_01_015: [TYPE Byte 5 of the frame header is a type code.] */
TEST_METHOD(frame_codec_receive_bytes_decodes_one_empty_frame)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	frame_codec_subscribe(frame_codec, 0, frame_received_callback, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, frame_received_callback(frame_codec, 0, IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, 2))
		.IgnoreArgument(3)
		.ValidateArgumentBuffer(5, &frame[6], 2);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

END_TEST_SUITE(frame_codec_unittests)