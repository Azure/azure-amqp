#include <cstdint>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "amqpvalue.h"
#include "frame_codec.h"
#include "io.h"
#include "list.h"

#define TEST_IO_HANDLE					(IO_HANDLE)0x4242
#define TEST_DESCRIPTION_AMQP_VALUE		(AMQP_VALUE)0x4243
#define TEST_ENCODER_HANDLE				(ENCODER_HANDLE)0x4244
#define TEST_DECODER_HANDLE				(DECODER_HANDLE)0x4245
#define TEST_LIST_HANDLE				(LIST_HANDLE)0x4246
#define TEST_SUBSCRIPTION_ITEM			(void*)0x4247

const IO_INTERFACE_DESCRIPTION test_io_interface_description = { 0 };

static IO_RECEIVE_CALLBACK io_receive_callback;
static void* io_receive_callback_context;
static const void** list_items = NULL;
static size_t list_item_count = 0;
static unsigned char* sent_io_bytes;
static size_t sent_io_byte_count;

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
		unsigned char* new_bytes = (unsigned char*)realloc(sent_io_bytes, sent_io_byte_count + size);
		if (new_bytes != NULL)
		{
			sent_io_bytes = new_bytes;
			(void)memcpy(sent_io_bytes + sent_io_byte_count, buffer, size);
			sent_io_byte_count += size;
		}
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

	/* frame received callback */
	MOCK_STATIC_METHOD_4(, void, frame_begin_callback_1, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, frame_body_bytes_received_callback_1, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, void, frame_begin_callback_2, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, frame_body_bytes_received_callback_2, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size)
	MOCK_VOID_METHOD_END();

	/* list mocks */
	MOCK_STATIC_METHOD_0(, LIST_HANDLE, list_create)
	MOCK_METHOD_END(LIST_HANDLE, TEST_LIST_HANDLE);
	MOCK_STATIC_METHOD_1(, void, list_destroy, LIST_HANDLE, list)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_2(, int, list_add, LIST_HANDLE, handle, const void*, item)
		const void** items = (const void**)realloc(list_items, (list_item_count + 1) * sizeof(item));
		if (items != NULL)
		{
			list_items = items;
			list_items[list_item_count++] = item;
		}
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, LIST_ITEM_HANDLE, list_find, LIST_HANDLE, handle, LIST_MATCH_FUNCTION, match_function, const void*, match_context)
		size_t i;
		const void* found_item = NULL;
		for (i = 0; i < list_item_count; i++)
		{
			if (match_function((LIST_ITEM_HANDLE)list_items[i], match_context))
			{
				found_item = list_items[i];
				break;
			}
		}
	MOCK_METHOD_END(LIST_ITEM_HANDLE, (LIST_ITEM_HANDLE)found_item);
	MOCK_STATIC_METHOD_3(, int, list_remove_matching_item, LIST_HANDLE, handle, LIST_MATCH_FUNCTION, match_function, const void*, match_context)
		size_t i;
		int res = __LINE__;
		for (i = 0; i < list_item_count; i++)
		{
			if (match_function((LIST_ITEM_HANDLE)list_items[i], match_context))
			{
				(void)memcpy(&list_items[i], &list_items[i + 1], (list_item_count - i - 1) * sizeof(const void*));
				list_item_count--;
				res = 0;
				break;
			}
		}
	MOCK_METHOD_END(int, res);
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

	DECLARE_GLOBAL_MOCK_METHOD_4(frame_codec_mocks, , void, frame_begin_callback_1, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(frame_codec_mocks, , void, frame_body_bytes_received_callback_1, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size);
	DECLARE_GLOBAL_MOCK_METHOD_4(frame_codec_mocks, , void, frame_begin_callback_2, void*, context, uint32_t, frame_body_size, const unsigned char*, type_specific, uint32_t, type_specific_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(frame_codec_mocks, , void, frame_body_bytes_received_callback_2, void*, context, const unsigned char*, frame_body_bytes, uint32_t, frame_body_bytes_size);

	DECLARE_GLOBAL_MOCK_METHOD_0(frame_codec_mocks, , LIST_HANDLE, list_create);
	DECLARE_GLOBAL_MOCK_METHOD_1(frame_codec_mocks, , void, list_destroy, LIST_HANDLE, list);
	DECLARE_GLOBAL_MOCK_METHOD_2(frame_codec_mocks, , int, list_add, LIST_HANDLE, handle, const void*, item);
	DECLARE_GLOBAL_MOCK_METHOD_3(frame_codec_mocks, , LIST_ITEM_HANDLE, list_find, LIST_HANDLE, handle, LIST_MATCH_FUNCTION, match_function, const void*, match_context);
	DECLARE_GLOBAL_MOCK_METHOD_3(frame_codec_mocks, , int, list_remove_matching_item, LIST_HANDLE, handle, LIST_MATCH_FUNCTION, match_function, const void*, match_context);

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
	if (list_items != NULL)
	{
		free(list_items);
		list_items = NULL;
	}
	if (sent_io_bytes != NULL)
	{
		free(sent_io_bytes);
		sent_io_bytes = NULL;
	}
	list_item_count = 0;
	sent_io_byte_count = 0;
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
	STRICT_EXPECTED_CALL(mocks, list_create());

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

	STRICT_EXPECTED_CALL(mocks, list_destroy(TEST_LIST_HANDLE));
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

/* Tests_SRS_FRAME_CODEC_01_023: [frame_codec_destroy shall free all resources associated with a frame_codec instance.] */
TEST_METHOD(frame_codec_destroy_while_receiving_type_specific_data_frees_the_type_specific_buffer)
{
	// arrange
	frame_codec_mocks mocks;
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00 };
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_destroy(TEST_LIST_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	frame_codec_destroy(frame_codec);

	// assert
	// uMock checks the calls
}

/* frame_codec_set_max_frame_size */

/* Tests_SRS_FRAME_CODEC_01_075: [frame_codec_set_max_frame_size shall set the maximum frame size for a frame_codec.] */
TEST_METHOD(frame_codec_set_max_frame_size_with_8_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_set_max_frame_size(frame_codec, 8);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* frame_codec_receive_bytes */

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.] */
/* Tests_SRS_FRAME_CODEC_01_031: [When a frame header is successfully decoded it shall be indicated to the upper layer by invoking the frame_begin_callback_1 passed to frame_codec_subscribe.] */
/* Tests_SRS_FRAME_CODEC_01_032: [Besides passing the frame information, the callback_context value passed to frame_codec_subscribe shall be passed to the frame_begin_callback_1 function.] */
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
/* Tests_SRS_FRAME_CODEC_01_028: [The sequence of bytes shall be decoded according to the AMQP ISO.] */
/* Tests_SRS_FRAME_CODEC_01_085: [If the frame body is empty, no call to frame_body_bytes_received_callback_1 shall be made.] */
/* Tests_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value. */
TEST_METHOD(frame_codec_receive_bytes_decodes_one_empty_frame)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.] */
TEST_METHOD(frame_codec_receive_bytes_with_not_enough_bytes_for_a_frame_does_not_trigger_callback)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_026: [If frame_codec or buffer are NULL, frame_codec_receive_bytes shall return a non-zero value.] */
TEST_METHOD(frame_codec_receive_bytes_with_NULL_frame_codec_handle_fails)
{
	// arrange
	frame_codec_mocks mocks;
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };

	// act
	int result = frame_codec_receive_bytes(NULL, frame, sizeof(frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_026: [If frame_codec or buffer are NULL, frame_codec_receive_bytes shall return a non-zero value.] */
TEST_METHOD(frame_codec_receive_bytes_with_NULL_buffer_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_receive_bytes(frame_codec, NULL, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_027: [If size is zero, frame_codec_receive_bytes shall return a non-zero value.] */
TEST_METHOD(frame_codec_receive_bytes_with_zero_size_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
TEST_METHOD(when_frame_codec_receive_1_byte_in_one_call_and_the_rest_of_the_frame_in_another_call_yields_succesfull_decode)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	(void)frame_codec_receive_bytes(frame_codec, frame, 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame + 1, sizeof(frame) - 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
TEST_METHOD(when_frame_codec_receive_the_frame_bytes_in_1_byte_per_call_a_succesfull_decode_happens)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };
	size_t i;

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	for (i = 0; i < sizeof(frame) - 1; i++)
	{
		(void)frame_codec_receive_bytes(frame_codec, &frame[i], 1);
	}

	// act
	int result = frame_codec_receive_bytes(frame_codec, &frame[sizeof(frame) - 1], 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
TEST_METHOD(a_frame_codec_receive_bytes_call_with_bad_args_before_any_real_frame_bytes_does_not_affect_decoding)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	(void)frame_codec_receive_bytes(frame_codec, NULL, 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
TEST_METHOD(a_frame_codec_receive_bytes_call_with_bad_args_in_the_middle_of_the_frame_does_not_affect_decoding)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	(void)frame_codec_receive_bytes(frame_codec, frame, 1);
	(void)frame_codec_receive_bytes(frame_codec, NULL, 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame + 1, sizeof(frame) - 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.] */
TEST_METHOD(frame_codec_receive_bytes_decodes_2_empty_frames)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame1[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x01, 0x02 };
	unsigned char frame2[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x03, 0x04 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame1[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame1[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame2[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame2[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	(void)frame_codec_receive_bytes(frame_codec, frame1, sizeof(frame1));

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame2, sizeof(frame2));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.] */
TEST_METHOD(a_call_to_frame_codec_receive_bytes_with_bad_args_between_2_frames_does_not_affect_decoding)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame1[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x01, 0x02 };
	unsigned char frame2[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x03, 0x04 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame1[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame1[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame2[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame2[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	(void)frame_codec_receive_bytes(frame_codec, frame1, sizeof(frame1));
	(void)frame_codec_receive_bytes(frame_codec, NULL, 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame2, sizeof(frame2));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_010: [The frame is malformed if the size is less than the size of the frame header (8 bytes).] */
TEST_METHOD(when_frame_size_is_bad_frame_codec_receive_bytes_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0x01, 0x02 };

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_014: [Due to the mandatory 8-byte frame header, the frame is malformed if the value is less than 2.] */
TEST_METHOD(when_frame_size_has_a_bad_doff_frame_codec_receive_bytes_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x01, 0x02 };

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_receive_bytes shall fail.] */
TEST_METHOD(after_a_frame_decode_error_occurs_due_to_frame_size_a_subsequent_decode_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char bad_frame[] = { 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0x01, 0x02 };
	unsigned char good_frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x01, 0x02 };

	(void)frame_codec_receive_bytes(frame_codec, bad_frame, sizeof(bad_frame));

	// act
	int result = frame_codec_receive_bytes(frame_codec, good_frame, sizeof(good_frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_receive_bytes shall fail.] */
TEST_METHOD(after_a_frame_decode_error_occurs_due_to_bad_doff_size_a_subsequent_decode_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	unsigned char bad_frame[] = { 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x01, 0x02 };
	unsigned char good_frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x01, 0x02 };

	(void)frame_codec_receive_bytes(frame_codec, bad_frame, sizeof(bad_frame));
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_receive_bytes(frame_codec, good_frame, sizeof(good_frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.] */
/* Tests_SRS_FRAME_CODEC_01_083: [The frame body bytes shall be passed to the frame_body_bytes_received_callback_1 function that was given to frame_codec_subscribe.] */
/* Tests_SRS_FRAME_CODEC_01_086: [Besides passing the frame information, the callback_context value passed to frame_codec_subscribe shall be passed to the frame_body_bytes_received_callback_1 function.] */
TEST_METHOD(receiving_a_frame_with_1_byte_frame_body_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x09, 0x02, 0x00, 0x01, 0x02, 0x42 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 1, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 1))
		.ValidateArgumentBuffer(2, &frame[8], 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_030: [If a decoding error occurs, frame_codec_receive_bytes shall return a non-zero value.] */
TEST_METHOD(when_allocating_type_specific_data_fails_frame_codec_receive_bytes_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x09, 0x02, 0x00, 0x01, 0x02, 0x42 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2))
		.SetReturn((void*)NULL);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_030: [If a decoding error occurs, frame_codec_receive_bytes shall return a non-zero value.] */
TEST_METHOD(when_allocating_type_specific_data_fails_a_subsequent_decode_Call_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x09, 0x02, 0x00, 0x01, 0x02, 0x42 };

	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2))
		.SetReturn((void*)NULL);

	(void)frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_084: [The bytes shall be passed to frame_body_bytes_received_callback_1 as they arrive, not waiting for all frame body bytes to be received.] */
TEST_METHOD(a_frame_with_2_bytes_received_in_2_frame_codec_receive_bytes_calls_passes_the_bytes_as_they_arrive)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };
	frame_codec_receive_bytes(frame_codec, frame, sizeof(frame) - 2);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 1))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 2], 1);
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 1))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 1], 1);

	(void)frame_codec_receive_bytes(frame_codec, &frame[sizeof(frame) - 2], 1);

	// act
	(void)frame_codec_receive_bytes(frame_codec, &frame[sizeof(frame) - 1], 1);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_FRAME_CODEC_01_084: [The bytes shall be passed to frame_body_bytes_received_callback_1 as they arrive, not waiting for all frame body bytes to be received.] */
TEST_METHOD(a_frame_with_2_bytes_received_in_1_frame_codec_receive_bytes_call_passes_the_bytes_as_they_arrive)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };
	frame_codec_receive_bytes(frame_codec, frame, sizeof(frame) - 2);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 2], 2);

	// act
	(void)frame_codec_receive_bytes(frame_codec, &frame[sizeof(frame) - 2], 2);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_FRAME_CODEC_01_084: [The bytes shall be passed to frame_body_bytes_received_callback_1 as they arrive, not waiting for all frame body bytes to be received.] */
TEST_METHOD(a_frame_with_2_bytes_received_together_with_the_header_passes_the_bytes_in_one_call)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 2, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 2], 2);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.]  */
TEST_METHOD(two_empty_frames_received_in_the_same_call_yields_2_callbacks)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x01, 0x02,
		0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x03, 0x04 };
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 0, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[14], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it shall return zero.]  */
TEST_METHOD(two_frames_with_1_byte_each_received_in_the_same_call_yields_2_callbacks)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x09, 0x02, 0x00, 0x01, 0x02, 0x42,
		0x00, 0x00, 0x00, 0x09, 0x02, 0x00, 0x03, 0x04, 0x43 };
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 1, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 1))
		.ValidateArgumentBuffer(2, &frame[8], 1);
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 1, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[15], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 1))
		.ValidateArgumentBuffer(2, &frame[17], 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* frame_codec_subscribe */

/* Tests_SRS_FRAME_CODEC_01_033: [frame_codec_subscribe subscribes for a certain type of frame received by the frame_codec instance identified by frame_codec.] */
/* Tests_SRS_FRAME_CODEC_01_087: [On success, frame_codec_subscribe shall return zero.] */
TEST_METHOD(frame_codec_subscribe_with_valid_args_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, list_add(TEST_LIST_HANDLE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_033: [frame_codec_subscribe subscribes for a certain type of frame received by the frame_codec instance identified by frame_codec.] */
/* Tests_SRS_FRAME_CODEC_01_087: [On success, frame_codec_subscribe shall return zero.] */
TEST_METHOD(when_list_find_returns_NULL_a_new_subscription_is_created)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1)
		.SetReturn((LIST_ITEM_HANDLE)NULL);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, list_add(TEST_LIST_HANDLE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_034: [If any of the frame_codec, frame_begin_callback or frame_body_bytes_received_callback arguments is NULL, frame_codec_subscribe shall return a non-zero value.] */
TEST_METHOD(when_frame_codec_is_NULL_frame_codec_subscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;

	// act
	int result = frame_codec_subscribe(NULL, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, (void*)0x01);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_034: [If any of the frame_codec, frame_begin_callback or frame_body_bytes_received_callback arguments is NULL, frame_codec_subscribe shall return a non-zero value.] */
TEST_METHOD(when_frame_begin_callback_is_NULL_frame_codec_subscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_subscribe(frame_codec, 0, NULL, frame_body_bytes_received_callback_1, frame_codec);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_034: [If any of the frame_codec, frame_begin_callback_1 or frame_received_callback arguments is NULL, frame_codec_subscribe shall return a non-zero value.] */
TEST_METHOD(when_frame_body_bytes_received_callback_is_NULL_frame_codec_subscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, NULL, frame_codec);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value. */
TEST_METHOD(when_a_frame_type_that_has_no_subscribers_is_received_no_callback_is_called)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x01, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value.] */
TEST_METHOD(when_no_subscribe_is_done_no_callback_is_called)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x08, 0x02, 0x01, 0x00, 0x00 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value.] */
TEST_METHOD(when_2_subscriptions_exist_and_first_one_matches_the_callback_is_invoked)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_subscribe(frame_codec, 1, frame_begin_callback_2, frame_body_bytes_received_callback_2, frame_codec);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_1(frame_codec, 2, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_1(frame_codec, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 2], 2);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value.] */
TEST_METHOD(when_2_subscriptions_exist_and_second_one_matches_the_callback_is_invoked)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_subscribe(frame_codec, 1, frame_begin_callback_2, frame_body_bytes_received_callback_2, frame_codec);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x01, 0x01, 0x02, 0x42, 0x43 };
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_2(frame_codec, 2, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_2(frame_codec, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 2], 2);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_036: [Only one callback pair shall be allowed to be registered for a given frame type.] */
TEST_METHOD(when_frame_codec_subscribe_is_called_twice_for_the_same_frame_type_it_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_2, frame_body_bytes_received_callback_2, frame_codec);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_036: [Only one callback pair shall be allowed to be registered for a given frame type.] */
TEST_METHOD(the_callbacks_for_the_2nd_frame_codec_subscribe_for_the_same_frame_type_remain_in_effect)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_2, frame_body_bytes_received_callback_2, frame_codec);
	mocks.ResetAllCalls();

	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);
	STRICT_EXPECTED_CALL(mocks, amqpalloc_malloc(2));
	STRICT_EXPECTED_CALL(mocks, frame_begin_callback_2(frame_codec, 2, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(3, &frame[6], 2);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, frame_body_bytes_received_callback_2(frame_codec, IGNORED_PTR_ARG, 2))
		.ValidateArgumentBuffer(2, &frame[sizeof(frame) - 2], 2);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_037: [If any failure occurs while performing the subscribe operation, frame_codec_subscribe shall return a non-zero value.] */
TEST_METHOD(when_allocating_memory_for_the_subscription_fails_frame_codec_subscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_037: [If any failure occurs while performing the subscribe operation, frame_codec_subscribe shall return a non-zero value.] */
TEST_METHOD(when_adding_the_subscription_fails_then_frame_codec_subscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, list_add(TEST_LIST_HANDLE, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(1);

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* frame_codec_unsubscribe */

/* Tests_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
TEST_METHOD(removing_an_existing_subscription_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_remove_matching_item(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);

	// act
	int result = frame_codec_unsubscribe(frame_codec, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
TEST_METHOD(removing_an_existing_subscription_does_not_trigger_callback_when_a_frame_of_that_type_is_received)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_unsubscribe(frame_codec, 0);
	unsigned char frame[] = { 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x01, 0x02, 0x42, 0x43 };
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame[5], 1);

	// act
	int result = frame_codec_receive_bytes(frame_codec, frame, sizeof(frame));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_039: [If frame_codec is NULL, frame_codec_unsubscribe shall return a non-zero value.] */
TEST_METHOD(frame_codec_unsubscribe_with_NULL_frame_codec_handle_fails)
{
	// arrange
	frame_codec_mocks mocks;

	// act
	int result = frame_codec_unsubscribe(NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_040: [If no subscription for the type frame type exists, frame_codec_unsubscribe shall return a non-zero value.] */
TEST_METHOD(frame_codec_unsubscribe_with_no_subscribe_call_has_been_made_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_remove_matching_item(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);

	// act
	int result = frame_codec_unsubscribe(frame_codec, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_041: [If any failure occurs while performing the unsubscribe operation, frame_codec_unsubscribe shall return a non-zero value.] */
TEST_METHOD(when_list_remove_matching_item_fails_then_frame_codec_unsubscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_remove_matching_item(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1)
		.SetReturn(1);

	// act
	int result = frame_codec_unsubscribe(frame_codec, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
TEST_METHOD(unsubscribe_one_of_2_subscriptions_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_subscribe(frame_codec, 1, frame_begin_callback_2, frame_body_bytes_received_callback_2, frame_codec);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_remove_matching_item(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);

	// act
	int result = frame_codec_unsubscribe(frame_codec, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
TEST_METHOD(unsubscribe_2nd_out_of_2_subscriptions_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_subscribe(frame_codec, 1, frame_begin_callback_2, frame_body_bytes_received_callback_2, frame_codec);
	mocks.ResetAllCalls();

	uint8_t frame_type = 1;
	STRICT_EXPECTED_CALL(mocks, list_remove_matching_item(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);

	// act
	int result = frame_codec_unsubscribe(frame_codec, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
TEST_METHOD(subscribe_unsubscribe_subscribe_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_unsubscribe(frame_codec, 0);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_find(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, list_add(TEST_LIST_HANDLE, IGNORED_PTR_ARG))
		.IgnoreArgument(2);

	// act
	int result = frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
TEST_METHOD(subscribe_unsubscribe_unsubscribe_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_subscribe(frame_codec, 0, frame_begin_callback_1, frame_body_bytes_received_callback_1, frame_codec);
	(void)frame_codec_unsubscribe(frame_codec, 0);
	mocks.ResetAllCalls();

	uint8_t frame_type = 0;
	STRICT_EXPECTED_CALL(mocks, list_remove_matching_item(TEST_LIST_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.ValidateArgumentBuffer(3, &frame_type, 1);

	// act
	int result = frame_codec_unsubscribe(frame_codec, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_016: [The type code indicates the format and purpose of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_017: [The subsequent bytes in the frame header MAY be interpreted differently depending on the type of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_018: [A type code of 0x00 indicates that the frame is an AMQP frame.] */
/* Tests_SRS_FRAME_CODEC_01_070: [The type code indicates the format and purpose of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_071: [The subsequent bytes in the frame header MAY be interpreted differently depending on the type of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_072: [A type code of 0x00 indicates that the frame is an AMQP frame.] */
TEST_METHOD(frame_type_amqp_is_zero)
{
	// arrange

	// act

	// assert
	ASSERT_ARE_EQUAL(uint8_t, 0, FRAME_TYPE_AMQP);
}

/* Tests_SRS_FRAME_CODEC_01_016: [The type code indicates the format and purpose of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_017: [The subsequent bytes in the frame header MAY be interpreted differently depending on the type of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_019: [A type code of 0x01 indicates that the frame is a SASL frame] */
/* Tests_SRS_FRAME_CODEC_01_070: [The type code indicates the format and purpose of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_071: [The subsequent bytes in the frame header MAY be interpreted differently depending on the type of the frame.] */
/* Tests_SRS_FRAME_CODEC_01_073: [A type code of 0x01 indicates that the frame is a SASL frame] */
TEST_METHOD(frame_type_sasl_is_one)
{
	// arrange

	// act

	// assert
	ASSERT_ARE_EQUAL(uint8_t, 1, FRAME_TYPE_SASL);
}

/* frame_codec_begin_encode_frame */

/* Tests_SRS_FRAME_CODEC_01_042: [frame_codec_begin_encode_frame encodes the header and type specific bytes of a frame that has frame_payload_size bytes.] */
/* Tests_SRS_FRAME_CODEC_01_043: [On success it shall return 0.] */
/* Tests_SRS_FRAME_CODEC_01_088: [Encoding the bytes shall happen by passing the bytes to the underlying IO interface.] */
/* Tests_SRS_FRAME_CODEC_01_055: [Frames are divided into three distinct areas: a fixed width frame header, a variable width extended header, and a variable width frame body.] */
/* Tests_SRS_FRAME_CODEC_01_056: [frame header The frame header is a fixed size (8 byte) structure that precedes each frame.] */
/* Tests_SRS_FRAME_CODEC_01_057: [The frame header includes mandatory information necessary to parse the rest of the frame including size and type information.] */
/* Tests_SRS_FRAME_CODEC_01_058: [extended header The extended header is a variable width area preceding the frame body.] */
/* Tests_SRS_FRAME_CODEC_01_059: [This is an extension point defined for future expansion.] */
/* Tests_SRS_FRAME_CODEC_01_060: [The treatment of this area depends on the frame type.] */
/* Tests_SRS_FRAME_CODEC_01_062: [SIZE Bytes 0-3 of the frame header contain the frame size.] */
/* Tests_SRS_FRAME_CODEC_01_063: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
/* Tests_SRS_FRAME_CODEC_01_064: [The frame is malformed if the size is less than the size of the frame header (8 bytes).] */
TEST_METHOD(frame_codec_begin_encode_frame_with_a_zero_frame_body_length_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.ExpectedAtLeastTimes(1);
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.IgnoreAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0, 0, NULL, 0);

	// assert
	char stringified_io[512];
	stringify_bytes(sent_io_bytes, sent_io_byte_count, stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, "[0x00,0x00,0x00,0x08,0x02,0x00,0x00,0x00]", stringified_io);
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_044: [If the argument frame_codec is NULL, frame_codec_begin_encode_frame shall return a non-zero value.] */
TEST_METHOD(when_frame_codec_is_NULL_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;

	// act
	int result = frame_codec_begin_encode_frame(NULL, 0, 0, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_091: [If the argument type_specific_size is greater than 0 and type_specific_bytes is NULL, frame_codec_begin_encode_frame shall return a non-zero value.] */
TEST_METHOD(when_type_specific_size_is_positive_and_type_speific_bytes_is_NULL_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0, 0, NULL, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_045: [If encoding the header fails (cannot be sent through the IO interface), frame_codec_begin_encode_frame shall return a non-zero value.] */
TEST_METHOD(when_io_send_fails_then_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.SetReturn(1);

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0, 0, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_092: [If type_specific_size is too big to allow encoding the frame according to the AMQP ISO then frame_codec_begin_encode_frame shall return a non-zero value.] */
/* Tests_SRS_FRAME_CODEC_01_065: [DOFF Byte 4 of the frame header is the data offset.] */
/* Tests_SRS_FRAME_CODEC_01_066: [This gives the position of the body within the frame.] */
/* Tests_SRS_FRAME_CODEC_01_058: [extended header The extended header is a variable width area preceding the frame body.] */
TEST_METHOD(when_type_specific_size_is_too_big_then_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	unsigned char expected_frame[1020] = { 0x00, 0x00, 0x00, 0x0A, 0xFF, 0x00, 0x00, 0x00 };
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0, 0, &expected_frame[6], 1015);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_092: [If type_specific_size is too big to allow encoding the frame according to the AMQP ISO then frame_codec_begin_encode_frame shall return a non-zero value.] */
/* Tests_SRS_FRAME_CODEC_01_065: [DOFF Byte 4 of the frame header is the data offset.] */
/* Tests_SRS_FRAME_CODEC_01_066: [This gives the position of the body within the frame.] */
/* Tests_SRS_FRAME_CODEC_01_058: [extended header The extended header is a variable width area preceding the frame body.] */
/* Tests_SRS_FRAME_CODEC_01_065: [DOFF Byte 4 of the frame header is the data offset.] */
/* Tests_SRS_FRAME_CODEC_01_066: [This gives the position of the body within the frame.] */
/* Tests_SRS_FRAME_CODEC_01_067: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
TEST_METHOD(when_type_specific_size_is_max_allowed_then_frame_codec_begin_encode_frame_succeeds)
{
	// arrange
	frame_codec_mocks mocks;
	unsigned char expected_frame[1020] = { 0x00, 0x00, 0x03, 0xFC, 0xFF, 0x00, 0x00, 0x00 };
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.ExpectedAtLeastTimes(1);
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.IgnoreAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0, 0, &expected_frame[6], 1014);

	// assert
	char expected_stringified_io[8192];
	char actual_stringified_io[8192];
	memset(expected_frame + 6, 0, 1020 - 6);
	stringify_bytes(expected_frame, sizeof(expected_frame), expected_stringified_io);
	stringify_bytes(sent_io_bytes, sent_io_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_090: [If the type_specific_size – 2 does not divide by 4, frame_codec_begin_encode_frame shall pad the type_specific bytes with zeroes so that type specific data is according to the AMQP ISO.] */
/* Tests_SRS_FRAME_CODEC_01_065: [DOFF Byte 4 of the frame header is the data offset.] */
/* Tests_SRS_FRAME_CODEC_01_066: [This gives the position of the body within the frame.] */
/* Tests_SRS_FRAME_CODEC_01_067: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
/* Tests_SRS_FRAME_CODEC_01_068: [Due to the mandatory 8-byte frame header, the frame is malformed if the value is less than 2.] */
TEST_METHOD(one_byte_of_pading_is_added_to_type_specific_data_to_make_the_frame_header)
{
	// arrange
	frame_codec_mocks mocks;
	unsigned char expected_frame[] = { 0x00, 0x00, 0x00, 0x8, 0x02, 0x00, 0x42, 0x00 };
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.ExpectedAtLeastTimes(1);
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.IgnoreAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0, 0, &expected_frame[6], 1);

	// assert
	char expected_stringified_io[8192];
	char actual_stringified_io[8192];
	stringify_bytes(expected_frame, sizeof(expected_frame), expected_stringified_io);
	stringify_bytes(sent_io_bytes, sent_io_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_069: [TYPE Byte 5 of the frame header is a type code.] */
TEST_METHOD(the_type_is_placed_in_the_underlying_frame)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.ExpectedAtLeastTimes(1);
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.IgnoreAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 0, NULL, 0);

	// assert
	char actual_stringified_io[8192];
	stringify_bytes(sent_io_bytes, sent_io_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, "[0x00,0x00,0x00,0x08,0x02,0x42,0x00,0x00]", actual_stringified_io);
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_089: [If any IO calls fail then frame_codec_begin_encode_frame shall return a non-zero value.] */
TEST_METHOD(when_2nd_io_send_fails_frame_codec_begin_encode_frame_fails_too)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE));
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.SetReturn(1);

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 0, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_089: [If any IO calls fail then frame_codec_begin_encode_frame shall return a non-zero value.] */
TEST_METHOD(when_3rd_io_send_fails_frame_codec_begin_encode_frame_fails_too)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	unsigned char expected_frame[] = { 0x00, 0x00, 0x00, 0x8, 0x02, 0x42, 0x42, 0x00 };
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE));
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE));
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.SetReturn(1);

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 0, &expected_frame[6], 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_046: [Once encoding succeeds, all subsequent frame_codec_begin_encode_frame calls shall fail, until all the bytes of the frame have been encoded by using frame_codec_encode_frame_bytes.] */
TEST_METHOD(while_not_all_frame_body_bytes_were_encoded_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	(void)frame_codec_begin_encode_frame(frame_codec, 0x42, 1, NULL, 0);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 1, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_093: [Once encoding has failed due to IO issues, all subsequent calls to frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(after_an_io_failure_a_subsequent_call_to_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.SetReturn(1);

	(void)frame_codec_begin_encode_frame(frame_codec, 0x42, 0, NULL, 0);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 0, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_093: [Once encoding has failed due to IO issues, all subsequent calls to frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(after_an_io_failure_with_2nd_io_send_call_a_subsequent_call_to_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE));
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.SetReturn(1);

	uint8_t byte = 0x42;
	(void)frame_codec_begin_encode_frame(frame_codec, 0x42, 0, &byte, 1);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 0, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_FRAME_CODEC_01_093: [Once encoding has failed due to IO issues, all subsequent calls to frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(after_an_io_failure_with_3rd_io_send_call_a_subsequent_call_to_frame_codec_begin_encode_frame_fails)
{
	// arrange
	frame_codec_mocks mocks;
	FRAME_CODEC_HANDLE frame_codec = frame_codec_create(TEST_IO_HANDLE, consolelogger_log);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE));
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE));
	EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORE))
		.SetReturn(1);

	uint8_t byte = 0x42;
	(void)frame_codec_begin_encode_frame(frame_codec, 0x42, 0, &byte, 1);
	mocks.ResetAllCalls();

	// act
	int result = frame_codec_begin_encode_frame(frame_codec, 0x42, 0, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

END_TEST_SUITE(frame_codec_unittests)
