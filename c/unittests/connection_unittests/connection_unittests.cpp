#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "connection.h"
#include "io.h"
#include "socketio.h"
#include "frame_codec.h"
#include "amqp_frame_codec.h"
#include "amqp_definitions.h"
#include "amqp_definitions_mocks.h"
#include "list.h"

/* Requirements implicitly tested */
/* Tests_SRS_CONNECTION_01_088: [Any data appearing beyond the protocol header MUST match the version indicated by the protocol header.] */
/* Tests_SRS_CONNECTION_01_039: [START In this state a connection exists, but nothing has been sent or received. This is the state an implementation would be in immediately after performing a socket connect or socket accept.] */
/* Tests_SRS_CONNECTION_01_015: [Implementations SHOULD NOT expect to be able to reuse open TCP sockets after close performatives have been exchanged.] */

/* Requirements enforced by design */
/* Tests_SRS_CONNECTION_01_225: [HDR_RCVD HDR OPEN] */
/* Tests_SRS_CONNECTION_01_224: [START HDR HDR] */
/* Tests_SRS_CONNECTION_01_227: [HDR_EXCH OPEN OPEN] */
/* Tests_SRS_CONNECTION_01_228: [OPEN_RCVD OPEN *] */
/* Tests_SRS_CONNECTION_01_235: [CLOSE_SENT - * TCP Close for Write] */
/* Tests_SRS_CONNECTION_01_234: [CLOSE_RCVD * -TCP Close for Read] */

#define TEST_IO_HANDLE					(IO_HANDLE)0x4242
#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4243
#define TEST_AMQP_FRAME_CODEC_HANDLE	(AMQP_FRAME_CODEC_HANDLE)0x4244
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4245
#define TEST_LIST_ITEM_AMQP_VALUE		(AMQP_VALUE)0x4246
#define TEST_DESCRIBED_AMQP_VALUE		(AMQP_VALUE)0x4247
#define TEST_AMQP_OPEN_FRAME_HANDLE		(AMQP_OPEN_FRAME_HANDLE)0x4245
#define TEST_LIST_HANDLE				(LIST_HANDLE)0x4246
#define TEST_OPEN_PERFORMATIVE			(AMQP_VALUE)0x4301
#define TEST_CLOSE_PERFORMATIVE				(AMQP_VALUE)0x4302
#define TEST_CLOSE_DESCRIPTOR_AMQP_VALUE	(AMQP_VALUE)0x4303
#define TEST_BEGIN_PERFORMATIVE			(AMQP_VALUE)0x4304

#define TEST_CONTEXT					(void*)(0x4242)

static const char test_container_id[] = "1234";

const IO_INTERFACE_DESCRIPTION test_io_interface_description = { 0 };

static IO_RECEIVE_CALLBACK io_receive_callback;
static void* io_receive_callback_context;
static uint64_t performative_ulong;
static const void** list_items = NULL;
static size_t list_item_count = 0;
unsigned char* frame_codec_bytes = NULL;
size_t frame_codec_byte_count = 0;
static AMQP_FRAME_RECEIVED_CALLBACK saved_frame_received_callback;
static AMQP_EMPTY_FRAME_RECEIVED_CALLBACK saved_empty_frame_received_callback;
static AMQP_FRAME_CODEC_ERROR_CALLBACK saved_amqp_frame_codec_error_callback;
static void* saved_callback_context;

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

static char expected_stringified_io[8192];
static char actual_stringified_io[8192];

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

TYPED_MOCK_CLASS(connection_mocks, CGlobalMock)
{
public:
	/* io mocks */
	MOCK_STATIC_METHOD_3(, IO_HANDLE, io_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, const void*, io_create_parameters, LOGGER_LOG, logger_log)
	MOCK_METHOD_END(IO_HANDLE, TEST_IO_HANDLE);
	MOCK_STATIC_METHOD_1(, void, io_destroy, IO_HANDLE, io)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, io_open, IO_HANDLE, io, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context)
		io_receive_callback = receive_callback;
		io_receive_callback_context = receive_callback_context;
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, int, io_close, IO_HANDLE, io)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, io_send, IO_HANDLE, io, const void*, buffer, size_t, size)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, void, io_dowork, IO_HANDLE, io)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, IO_STATE, io_get_state, IO_HANDLE, io)
	MOCK_METHOD_END(IO_STATE, IO_STATE_OPEN);

	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_2(, void*, amqpalloc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, realloc(ptr, size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* frame_codec */
	MOCK_STATIC_METHOD_4(, FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, FRAME_CODEC_ERROR_CALLBACK, frame_codec_error_callback, void*, frame_codec_error_callback_context, LOGGER_LOG, logger_log)
	MOCK_METHOD_END(FRAME_CODEC_HANDLE, TEST_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, frame_codec_receive_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, buffer, size_t, size)
		unsigned char* new_frame_codec_bytes = (unsigned char*)realloc(frame_codec_bytes, frame_codec_byte_count + size);
		if (new_frame_codec_bytes != NULL)
		{
			frame_codec_bytes = new_frame_codec_bytes;
			memcpy(frame_codec_bytes + frame_codec_byte_count, buffer, size);
			frame_codec_byte_count += size;
		}
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, frame_codec_set_max_frame_size, FRAME_CODEC_HANDLE, frame_codec, uint32_t, max_frame_size)
	MOCK_METHOD_END(int, 0);

	/* amqp_frame_codec */
	MOCK_STATIC_METHOD_5(, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_create, FRAME_CODEC_HANDLE, frame_codec, AMQP_FRAME_RECEIVED_CALLBACK, frame_received_callback, AMQP_EMPTY_FRAME_RECEIVED_CALLBACK, empty_frame_received_callback, AMQP_FRAME_CODEC_ERROR_CALLBACK, amqp_frame_codec_error_callback, void*, callback_context)
		saved_frame_received_callback = frame_received_callback;
		saved_empty_frame_received_callback = empty_frame_received_callback;
		saved_amqp_frame_codec_error_callback = amqp_frame_codec_error_callback;
		saved_callback_context = callback_context;
	MOCK_METHOD_END(AMQP_FRAME_CODEC_HANDLE, TEST_AMQP_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, amqp_frame_codec_destroy, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, int, amqp_frame_codec_begin_encode_frame, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec, uint16_t, channel, const AMQP_VALUE, performative, uint32_t, payload_size)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_3(, int, amqp_frame_codec_encode_payload_bytes, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec, const unsigned char*, bytes, uint32_t, count)
	MOCK_METHOD_END(int, 0);

	/* amqpvalue mocks */
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value)
		*ulong_value = performative_ulong;
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_get_inplace_descriptor, AMQP_VALUE, value)
	MOCK_METHOD_END(AMQP_VALUE, TEST_DESCRIPTOR_AMQP_VALUE);

    MOCK_STATIC_METHOD_1(, void, amqpvalue_destroy, AMQP_VALUE, value)
    MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_string, AMQP_VALUE, value, const char**, string_value)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, AMQP_VALUE, amqpvalue_get_list_item, AMQP_VALUE, value, size_t, index)
	MOCK_METHOD_END(AMQP_VALUE, TEST_LIST_ITEM_AMQP_VALUE);
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_get_described_value, AMQP_VALUE, value)
	MOCK_METHOD_END(AMQP_VALUE, TEST_DESCRIBED_AMQP_VALUE);

	/* amqpvalue_to_string mocks */
	MOCK_STATIC_METHOD_1(, char*, amqpvalue_to_string, AMQP_VALUE, amqp_value)
	MOCK_METHOD_END(char*, NULL);

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
	MOCK_STATIC_METHOD_1(, const void*, list_item_get_value, LIST_ITEM_HANDLE, item_handle)
	MOCK_METHOD_END(const void*, (const void*)item_handle);
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

	/* frame received callback mocks */
	MOCK_STATIC_METHOD_4(, void, test_on_frame_received, void*, context, AMQP_VALUE, performative, uint32_t, frame_payload_size, const unsigned char*, payload_bytes)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, test_on_connection_state_changed, void*, context, CONNECTION_STATE, new_connection_state, CONNECTION_STATE, previous_connection_state)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , IO_HANDLE, io_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, const void*, io_create_parameters, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, io_destroy, IO_HANDLE, io);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, io_open, IO_HANDLE, io, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , int, io_close, IO_HANDLE, io);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, io_send, IO_HANDLE, io, const void*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, io_dowork, IO_HANDLE, io);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , IO_STATE, io_get_state, IO_HANDLE, io);

	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , void*, amqpalloc_realloc, void*, ptr, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_4(connection_mocks, , FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, FRAME_CODEC_ERROR_CALLBACK, frame_codec_error_callback, void*, frame_codec_error_callback_context, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, frame_codec_receive_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, frame_codec_set_max_frame_size, FRAME_CODEC_HANDLE, frame_codec, uint32_t, max_frame_size);

	DECLARE_GLOBAL_MOCK_METHOD_5(connection_mocks, , AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_create, FRAME_CODEC_HANDLE, frame_codec, AMQP_FRAME_RECEIVED_CALLBACK, frame_received_callback, AMQP_EMPTY_FRAME_RECEIVED_CALLBACK, empty_frame_received_callback, AMQP_FRAME_CODEC_ERROR_CALLBACK, amqp_frame_codec_error_callback, void*, callback_context);
    DECLARE_GLOBAL_MOCK_METHOD_4(connection_mocks, , int, amqp_frame_codec_begin_encode_frame, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec, uint16_t, channel, const AMQP_VALUE, performative, uint32_t, payload_size);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, amqp_frame_codec_destroy, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, amqp_frame_codec_encode_payload_bytes, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec, const unsigned char*, bytes, uint32_t, count);

	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , AMQP_VALUE, amqpvalue_get_inplace_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, amqpvalue_get_string, AMQP_VALUE, value, const char**, string_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , AMQP_VALUE, amqpvalue_get_list_item, AMQP_VALUE, value, size_t, index);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , AMQP_VALUE, amqpvalue_get_described_value, AMQP_VALUE, value);

    DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, amqpvalue_destroy, AMQP_VALUE, value);

	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , char*, amqpvalue_to_string, AMQP_VALUE, amqp_value)

	DECLARE_GLOBAL_MOCK_METHOD_0(connection_mocks, , LIST_HANDLE, list_create);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, list_destroy, LIST_HANDLE, list);
	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, list_add, LIST_HANDLE, handle, const void*, item);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , const void*, list_item_get_value, LIST_ITEM_HANDLE, item_handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , LIST_ITEM_HANDLE, list_find, LIST_HANDLE, handle, LIST_MATCH_FUNCTION, match_function, const void*, match_context);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, list_remove_matching_item, LIST_HANDLE, handle, LIST_MATCH_FUNCTION, match_function, const void*, match_context);

	DECLARE_GLOBAL_MOCK_METHOD_4(connection_mocks, , void, test_on_frame_received, void*, context, AMQP_VALUE, performative, uint32_t, frame_payload_size, const unsigned char*, payload_bytes);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , void, test_on_connection_state_changed, void*, context, CONNECTION_STATE, new_connection_state, CONNECTION_STATE, previous_connection_state);

	extern void consolelogger_log(char* format, ...)
	{
		(void)format;
	}
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(connection_unittests)

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
	frame_codec_bytes = NULL;
	frame_codec_byte_count = 0;
	performative_ulong = 0x10;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
	free(frame_codec_bytes);
	if (!MicroMockReleaseMutex(test_serialize_mutex))
	{
		ASSERT_FAIL("Could not release test serialization mutex.");
	}
}

/* connection_create */

/* Tests_SRS_CONNECTION_01_001: [connection_create shall open a new connection to a specified host/port.] */
/* Tests_SRS_CONNECTION_01_082: [connection_create shall allocate a new frame_codec instance to be used for frame encoding/decoding.] */
/* Tests_SRS_CONNECTION_01_107: [connection_create shall create an amqp_frame_codec instance by calling amqp_frame_codec_create.] */
/* Tests_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
TEST_METHOD(connection_create_with_valid_args_succeeds)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NOT_NULL(connection);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_001: [connection_create shall open a new connection to a specified host/port.] */
/* Tests_SRS_CONNECTION_01_082: [connection_create shall allocate a new frame_codec instance to be used for frame encoding/decoding.] */
/* Tests_SRS_CONNECTION_01_107: [connection_create shall create an amqp_frame_codec instance by calling amqp_frame_codec_create.] */
/* Tests_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
TEST_METHOD(connection_create_with_valid_args_but_NULL_host_name_succeeds)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, test_container_id);

	// assert
	ASSERT_IS_NOT_NULL(connection);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
TEST_METHOD(when_allocating_memory_fails_then_connection_create_fails)
{
	// arrange
	connection_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_083: [If frame_codec_create fails then connection_create shall return NULL.] */
TEST_METHOD(when_frame_codec_create_fails_then_connection_create_fails)
{
	// arrange
	connection_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn((FRAME_CODEC_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_108: [If amqp_frame_codec_create fails, connection_create shall return NULL.] */
TEST_METHOD(when_amqp_frame_codec_create_fails_then_connection_create_fails)
{
	// arrange
	connection_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn((AMQP_FRAME_CODEC_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, frame_codec_destroy(TEST_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
TEST_METHOD(when_allocating_memory_for_hostname_fails_connection_create_fails)
{
	// arrange
	connection_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_destroy(TEST_AMQP_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, frame_codec_destroy(TEST_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
TEST_METHOD(when_allocating_memory_for_container_id_fails_connection_create_fails)
{
	// arrange
	connection_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_destroy(TEST_AMQP_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, frame_codec_destroy(TEST_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_071: [If io or container_id is NULL, connection_create shall return NULL.] */
TEST_METHOD(connection_create_with_NULL_io_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	CONNECTION_HANDLE connection = connection_create(NULL, "testhost", test_container_id);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_071: [If io or container_id is NULL, connection_create shall return NULL.] */
TEST_METHOD(connection_create_with_NULL_container_id_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", NULL);

	// assert
	ASSERT_IS_NULL(connection);
}

/* connection_destroy */

/* Tests_SRS_CONNECTION_01_073: [connection_destroy shall free all resources associated with a connection.] */
/* Tests_SRS_CONNECTION_01_074: [connection_destroy shall close the socket connection.] */
TEST_METHOD(connection_destroy_frees_resources)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_destroy(TEST_AMQP_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, frame_codec_destroy(TEST_FRAME_CODEC_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	connection_destroy(connection);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_CONNECTION_01_079: [If handle is NULL, connection_destroy shall do nothing.] */
TEST_METHOD(connection_destroy_with_NULL_handle_does_nothing)
{
	// arrange
	connection_mocks mocks;

	// act
	connection_destroy(NULL);

	// assert
	// uMock checks the calls
}

/* connection_set_max_frame_size */

/* Tests_SRS_CONNECTION_01_163: [If connection is NULL, connection_set_max_frame_size shall fail and return a non-zero value.] */
TEST_METHOD(connection_set_max_frame_size_with_NULL_connection_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	int result = connection_set_max_frame_size(NULL, 512);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_148: [connection_set_max_frame_size shall set the max_frame_size associated with a connection.] */
/* Tests_SRS_CONNECTION_01_149: [On success connection_set_max_frame_size shall return 0.] */
TEST_METHOD(connection_set_max_frame_size_with_valid_connection_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_set_max_frame_size(connection, 512);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_150: [If the max_frame_size is invalid then connection_set_max_frame_size shall fail and return a non-zero value.] */
/* Tests_SRS_CONNECTION_01_167: [Both peers MUST accept frames of up to 512 (MIN-MAX-FRAME-SIZE) octets.] */
TEST_METHOD(connection_set_max_frame_size_with_511_bytes_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_set_max_frame_size(connection, 511);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_164: [If connection_set_max_frame_size fails, the previous max_frame_size setting shall be retained.] */
/* Tests_SRS_CONNECTION_01_167: [Both peers MUST accept frames of up to 512 (MIN-MAX-FRAME-SIZE) octets.] */
TEST_METHOD(connection_set_max_frame_size_with_511_bytes_fails_and_previous_value_is_kept)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_max_frame_size(connection, 1042);
	mocks.ResetAllCalls();

	// act
	int result = connection_set_max_frame_size(connection, 511);

	// assert
	mocks.AssertActualAndExpectedCalls();
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	uint32_t max_frame_size;
	(void)connection_get_max_frame_size(connection, &max_frame_size);
	ASSERT_ARE_EQUAL(uint32_t, 1042, max_frame_size);

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_157: [If connection_set_max_frame_size is called after the initial Open frame has been sent, it shall fail and return a non-zero value.] */
TEST_METHOD(set_max_frame_size_after_open_is_sent_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	int result = connection_set_max_frame_size(connection, 1024);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_get_max_frame_size */

/* Tests_SRS_CONNECTION_01_170: [If connection or max_frame_size is NULL, connection_get_max_frame_size shall fail and return a non-zero value.] */
TEST_METHOD(connection_get_max_frame_size_with_NULL_connection_fails)
{
	// arrange
	connection_mocks mocks;
	uint32_t max_frame_size;

	// act
	int result = connection_get_max_frame_size(NULL, &max_frame_size);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_170: [If connection or max_frame_size is NULL, connection_get_max_frame_size shall fail and return a non-zero value.] */
TEST_METHOD(connection_get_max_frame_size_with_NULL_max_frame_size_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_get_max_frame_size(connection, NULL);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_168: [connection_get_max_frame_size shall return in the max_frame_size argument the current max frame size setting.] */
/* Tests_SRS_CONNECTION_01_169: [On success, connection_get_max_frame_size shall return 0.] */
/* Tests_SRS_CONNECTION_01_173: [<field name="max-frame-size" type="uint" default="4294967295"/>] */
TEST_METHOD(connection_get_max_frame_size_with_valid_arguments_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();
	uint32_t max_frame_size;

	// act
	int result = connection_get_max_frame_size(connection, &max_frame_size);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(uint32_t, 4294967295, max_frame_size);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_set_channel_max */

/* Tests_SRS_CONNECTION_01_181: [If connection is NULL then connection_set_channel_max shall fail and return a non-zero value.] */
TEST_METHOD(connection_set_channel_max_with_NULL_connection_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	int result = connection_set_channel_max(NULL, 10);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_153: [connection_set_channel_max shall set the channel_max associated with a connection.] */
/* Tests_SRS_CONNECTION_01_154: [On success connection_set_channel_max shall return 0.] */
TEST_METHOD(connection_set_channel_max_with_valid_connection_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_set_channel_max(connection, 10);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_156: [If connection_set_channel_max is called after the initial Open frame has been sent, it shall fail and return a non-zero value.] */
TEST_METHOD(set_channel_max_after_open_is_sent_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	int result = connection_set_channel_max(connection, 1024);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_get_channel_max */

/* Tests_SRS_CONNECTION_01_184: [If connection or channel_max is NULL, connection_get_channel_max shall fail and return a non-zero value.] */
TEST_METHOD(connection_get_channel_max_with_NULL_connection_fails)
{
	// arrange
	connection_mocks mocks;
	uint16_t channel_max;

	// act
	int result = connection_get_channel_max(NULL, &channel_max);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_184: [If connection or channel_max is NULL, connection_get_channel_max shall fail and return a non-zero value.] */
TEST_METHOD(connection_get_channel_max_with_NULL_channel_max_argument_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_get_channel_max(connection, NULL);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_182: [connection_get_channel_max shall return in the channel_max argument the current channel_max setting.] */
/* Tests_SRS_CONNECTION_01_183: [On success, connection_get_channel_max shall return 0.] */
TEST_METHOD(connection_get_channel_max_with_valid_argument_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_channel_max(connection, 12);
	mocks.ResetAllCalls();
	uint16_t channel_max;

	// act
	int result = connection_get_channel_max(connection, &channel_max);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(uint32_t, 12, (uint32_t)channel_max);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_182: [connection_get_channel_max shall return in the channel_max argument the current channel_max setting.] */
/* Tests_SRS_CONNECTION_01_183: [On success, connection_get_channel_max shall return 0.] */
/* Tests_SRS_CONNECTION_01_174: [<field name="channel-max" type="ushort" default="65535"/>] */
TEST_METHOD(connection_get_channel_max_default_value_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();
	uint16_t channel_max;

	// act
	int result = connection_get_channel_max(connection, &channel_max);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(uint32_t, 65535, (uint32_t)channel_max);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_set_idle_timeout */

/* Tests_SRS_CONNECTION_01_191: [If connection is NULL, connection_set_idle_timeout shall fail and return a non-zero value.] */
TEST_METHOD(connection_set_idle_timeout_with_NULL_connection_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	int result = connection_set_idle_timeout(NULL, 1000);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_159: [connection_set_idle_timeout shall set the idle_timeout associated with a connection.] */
/* Tests_SRS_CONNECTION_01_160: [On success connection_set_idle_timeout shall return 0.] */
TEST_METHOD(connection_set_idle_timeout_with_valid_connection_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_set_idle_timeout(connection, 1000);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_158: [If connection_set_idle_timeout is called after the initial Open frame has been sent, it shall fail and return a non-zero value.] */
TEST_METHOD(set_idle_timeout_after_open_is_sent_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	int result = connection_set_idle_timeout(connection, 1000);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_get_idle_timeout */

/* Tests_SRS_CONNECTION_01_190: [If connection or idle_timeout is NULL, connection_get_idle_timeout shall fail and return a non-zero value.]  */
TEST_METHOD(connection_get_idle_timeout_with_NULL_connection_fails)
{
	// arrange
	connection_mocks mocks;
	milliseconds idle_timeout;

	// act
	int result = connection_get_idle_timeout(NULL, &idle_timeout);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_190: [If connection or idle_timeout is NULL, connection_get_idle_timeout shall fail and return a non-zero value.]  */
TEST_METHOD(connection_get_idle_timeout_with_NULL_idle_timeout_argument_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	int result = connection_get_idle_timeout(connection, NULL);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_188: [connection_get_idle_timeout shall return in the idle_timeout argument the current idle_timeout setting.] */
/* Tests_SRS_CONNECTION_01_189: [On success, connection_get_idle_timeout shall return 0.] */
TEST_METHOD(connection_get_idle_timeout_with_valid_argument_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_idle_timeout(connection, 12);
	mocks.ResetAllCalls();
	milliseconds idle_timeout;

	// act
	int result = connection_get_idle_timeout(connection, &idle_timeout);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(uint32_t, 12, (uint32_t)idle_timeout);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_188: [connection_get_idle_timeout shall return in the idle_timeout argument the current idle_timeout setting.] */
/* Tests_SRS_CONNECTION_01_189: [On success, connection_get_idle_timeout shall return 0.] */
/* Tests_SRS_CONNECTION_01_175: [<field name="idle-time-out" type="milliseconds"/>] */
/* Tests_SRS_CONNECTION_01_192: [A value of zero is the same as if it was not set (null).] */
TEST_METHOD(connection_get_idle_timeout_default_value_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();
	milliseconds idle_timeout;

	// act
	int result = connection_get_idle_timeout(connection, &idle_timeout);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(uint32_t, 0, (uint32_t)idle_timeout);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_dowork */

/* Tests_SRS_CONNECTION_01_078: [If handle is NULL, connection_dowork shall do nothing.] */
TEST_METHOD(connection_dowork_with_NULL_handle_does_nothing)
{
	// arrange
	connection_mocks mocks;
	mocks.ResetAllCalls();

	// act
	connection_dowork(NULL);

	// assert
	// no explicit assert, uMock checks the calls
}

/* Tests_SRS_CONNECTION_01_203: [If the io has not been open before is IO_STATE_NOT_OPEN, connection_dowork shall attempt to open the io by calling io_open.] */
TEST_METHOD(when_io_state_is_not_open_connection_dowork_opens_the_io)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_open(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_OPENING);
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_204: [If io_open_fails, no more work shall be done by connection_dowork and the connection shall be consideren in the END state.] */
TEST_METHOD(when_io_open_fails_the_connection_state_shall_be_set_to_END)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_NOT_OPEN);
	EXPECTED_CALL(mocks, io_open(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_076: [connection_dowork shall schedule the underlying IO interface to do its work by calling io_dowork.] */
/* Tests_SRS_CONNECTION_01_084: [The connection state machine implementing the protocol requirements shall be run as part of connection_dowork.] */
/* Tests_SRS_CONNECTION_01_086: [Prior to sending any frames on a connection, each peer MUST start by sending a protocol header that indicates the protocol version used on the connection.] */
/* Tests_SRS_CONNECTION_01_087: [The protocol header consists of the upper case ASCII letters “AMQP” followed by a protocol id of zero, followed by three unsigned bytes representing the major, minor, and revision of the protocol version (currently 1 (MAJOR), 0 (MINOR), 0 (REVISION)). In total this is an 8-octet sequence] */
/* Tests_SRS_CONNECTION_01_091: [The AMQP peer which acted in the role of the TCP client (i.e. the peer that actively opened the connection) MUST immediately send its outgoing protocol header on establishment of the TCP connection.] */
/* Tests_SRS_CONNECTION_01_093: [_ When the client opens a new socket connection to a server, it MUST send a protocol header with the client’s preferred protocol version.] */
/* Tests_SRS_CONNECTION_01_104: [Sending the protocol header shall be done by using io_send.] */
/* Tests_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
/* Tests_SRS_CONNECTION_01_200: [The connection state machine processing shall only be done when the IO interface state is ready.] */
/* Tests_SRS_CONNECTION_01_201: [The IO interface state shall be queried by using io_get_state.] */
TEST_METHOD(connection_dowork_when_state_is_start_sends_the_AMQP_header_and_triggers_io_dowork)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, io_open(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_OPEN);
	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_200: [The connection state machine processing shall only be done when the IO interface state is ready.] */
TEST_METHOD(when_io_is_not_ready_connection_dowork_does_not_process_connection_states)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	connection_dowork(connection);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_OPENING);
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_200: [The connection state machine processing shall only be done when the IO interface state is ready.] */
/* Tests_SRS_CONNECTION_01_202: [If the io_get_state call returns IO_STATE_ERROR the connection shall be closed and the state set to END.] */
TEST_METHOD(when_io_is_in_error_connection_dowork_does_not_process_connection_states)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	connection_dowork(connection);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_ERROR);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_200: [The connection state machine processing shall only be done when the IO interface state is ready.] */
/* Tests_SRS_CONNECTION_01_202: [If the io_get_state call returns IO_STATE_ERROR the connection shall be closed and the state set to END.] */
TEST_METHOD(when_io_is_in_error_connection_dowork_closes_the_io)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_NOT_OPEN);
	EXPECTED_CALL(mocks, io_open(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	connection_dowork(connection);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE))
		.SetReturn(IO_STATE_ERROR);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_057: [END In this state it is illegal for either endpoint to write anything more onto the connection. The connection can be safely closed and discarded.] */
/* Tests_SRS_CONNECTION_01_106: [When sending the protocol header fails, the connection shall be immediately closed.] */
TEST_METHOD(when_sending_the_header_fails_connection_dowork_fails_and_io_is_destroyed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, io_open(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_headers_do_not_match_connection_gets_closed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'Q', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_header_first_byte_does_not_match_connection_gets_closed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'B' };

	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_header_last_byte_does_not_match_connection_gets_closed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 1 };

	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_header_first_byte_matches_but_only_1st_byte_received_no_io_close_is_done)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A' };

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_134: [The container id field shall be filled with the container id specified in connection_create.] */
/* Tests_SRS_CONNECTION_01_135: [If hostname has been specified by a call to connection_set_hostname, then that value shall be stamped in the open frame.] */
/* Tests_SRS_CONNECTION_01_205: [Sending the AMQP OPEN frame shall be done by calling amqp_frame_codec_begin_encode_frame with channel number 0, the actual performative payload and 0 as payload_size.] */
/* Tests_SRS_CONNECTION_01_151: [The connection max_frame_size setting shall be passed down to the frame_codec when the Open frame is sent.] */
/* Tests_SRS_CONNECTION_01_137: [The max_frame_size connection setting shall be set in the open frame by using open_set_max_frame_size.] */
/* Tests_SRS_CONNECTION_01_139: [The channel_max connection setting shall be set in the open frame by using open_set_channel_max.] */
/* Tests_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
/* Tests_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.] */
/* Tests_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
/* Tests_SRS_CONNECTION_01_006: [The open frame can only be sent on channel 0.] */
TEST_METHOD(when_the_header_is_received_an_open_frame_is_sent_out)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_hostname(test_open_handle, "testhost"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_207: [If frame_codec_set_max_frame_size fails the connection shall be closed and the state set to END.] */
TEST_METHOD(when_setting_the_max_frame_size_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_open_create_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"))
		.SetReturn((OPEN_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_open_set_hostname_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_hostname(test_open_handle, "testhost"))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_amqpvalue_create_open_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_hostname(test_open_handle, "testhost"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle))
		.SetReturn((AMQP_VALUE)NULL);
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_206: [If sending the frame fails, the connection shall be closed and state set to END.] */
TEST_METHOD(when_amqp_frame_codec_begin_encode_frame_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_hostname(test_open_handle, "testhost"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_136: [If no hostname value has been specified, no value shall be stamped in the open frame (no call to open_set_hostname shall be made).] */
TEST_METHOD(when_no_hostname_is_specified_no_hostname_is_stamped_on_the_open_frame)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_137: [The max_frame_size connection setting shall be set in the open frame by using open_set_max_frame_size.] */
TEST_METHOD(when_max_frame_size_has_been_specified_it_shall_be_set_in_the_open_frame)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_max_frame_size(connection, 1024);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 1024));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 1024));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and setto the END state.] */
TEST_METHOD(when_setting_the_max_frame_size_on_the_open_frame_fails_then_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_max_frame_size(connection, 1024);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 1024));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 1024))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_139: [The channel_max connection setting shall be set in the open frame by using open_set_channel_max.]  */
TEST_METHOD(when_channel_max_has_been_specified_it_shall_be_set_in_the_open_frame)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_channel_max(connection, 1024);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 1024));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and setto the END state.] */
TEST_METHOD(when_setting_the_channel_max_on_the_open_frame_fails_then_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_channel_max(connection, 1024);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 1024))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_141: [If idle_timeout has been specified by a call to connection_set_idle_timeout, then that value shall be stamped in the open frame.] */
TEST_METHOD(when_idle_timeout_has_been_specified_it_shall_be_set_in_the_open_frame)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_idle_timeout(connection, 1000);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_idle_time_out(test_open_handle, 1000));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and setto the END state.] */
TEST_METHOD(when_setting_the_idle_timeout_on_the_open_frame_fails_then_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_idle_timeout(connection, 1000);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_idle_time_out(test_open_handle, 1000))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_212: [After the initial handshake has been done all bytes received from the io instance shall be passed to the frame_codec for decoding by calling frame_codec_receive_bytes.] */
TEST_METHOD(when_1_byte_is_received_from_the_io_it_is_passed_to_the_frame_codec)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.IgnoreAllCalls();

	// act
	unsigned char byte = 42;
	io_receive_callback(io_receive_callback_context, &byte, 1);

	// assert
	stringify_bytes(&byte, 1, expected_stringified_io);
	stringify_bytes(frame_codec_bytes, frame_codec_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_212: [After the initial handshake has been done all bytes received from the io instance shall be passed to the frame_codec for decoding by calling frame_codec_receive_bytes.] */
TEST_METHOD(when_2_bytes_are_received_from_the_io_it_is_passed_to_the_frame_codec)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.IgnoreAllCalls();

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	stringify_bytes(bytes, sizeof(bytes), expected_stringified_io);
	stringify_bytes(frame_codec_bytes, frame_codec_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_213: [When passing the bytes to frame_codec fails, a CLOSE frame shall be sent and the state shall be set to DISCARDING.]  */
/* Tests_SRS_CONNECTION_01_217: [The CLOSE frame shall be constructed by using close_create.] */
/* Tests_SRS_CONNECTION_01_215: [Sending the AMQP CLOSE frame shall be done by calling amqp_frame_codec_begin_encode_frame with channel number 0, the actual performative payload and 0 as payload_size.] */
/* Tests_SRS_CONNECTION_01_218: [The error amqp:internal-error shall be set in the error.condition field of the CLOSE frame.] */
/* Tests_SRS_CONNECTION_01_013: [However, implementations SHOULD send it on channel 0] */
/* Codes_SRS_CONNECTION_01_238: [If set, this field indicates that the connection is being closed due to an error condition.] */
TEST_METHOD(when_giving_the_bytes_to_frame_codec_fails_the_connection_is_closed_with_internal_error)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_creating_a_close_frame_fails_then_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create())
		.SetReturn((CLOSE_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_creating_the_amqp_value_for_the_close_performative_fails_then_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle))
		.SetReturn((AMQP_VALUE)NULL);
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_sending_the_close_frame_fails_then_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_creating_the_error_object_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"))
		.SetReturn((ERROR_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_setting_the_error_description_on_the_error_handle_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
/* Tests_SRS_CONNECTION_01_218: [The error amqp:internal-error shall be set in the error.condition field of the CLOSE frame.] */
/* Tests_SRS_CONNECTION_01_219: [The error description shall be set to an implementation defined string.] */
TEST_METHOD(when_setting_the_error_on_the_close_frame_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, frame_codec_receive_bytes(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.ValidateArgument(1)
		.SetReturn(1);
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	unsigned char bytes[] = { 42, 43 };
	io_receive_callback(io_receive_callback_context, bytes, sizeof(bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_212: [After the initial handshake has been done all bytes received from the io instance shall be passed to the frame_codec for decoding by calling frame_codec_receive_bytes.] */
TEST_METHOD(when_one_extra_byte_is_received_with_the_header_the_extra_byte_is_passed_to_the_frame_codec)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char in_bytes[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0, 42 };

	// act
	io_receive_callback(io_receive_callback_context, in_bytes, sizeof(in_bytes));

	// assert
	stringify_bytes(&in_bytes[sizeof(in_bytes) - 1], 1, expected_stringified_io);
	stringify_bytes(frame_codec_bytes, frame_codec_byte_count, actual_stringified_io);
	ASSERT_ARE_EQUAL(char_ptr, expected_stringified_io, actual_stringified_io);
	mocks.SetPerformAutomaticCallComparison(AUTOMATIC_CALL_COMPARISON_OFF);
	definition_mocks.SetPerformAutomaticCallComparison(AUTOMATIC_CALL_COMPARISON_OFF);

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
/* Tests_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
TEST_METHOD(when_an_open_frame_that_cannot_be_parsed_properly_is_received_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_open(TEST_OPEN_PERFORMATIVE, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(1);

	/* we expect to close because of bad OPEN */
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:invalid-field"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
/* Tests_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
TEST_METHOD(when_the_max_frame_size_cannot_be_retrieved_from_the_open_framethe_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_open(TEST_OPEN_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &test_open_handle, sizeof(test_open_handle));
	STRICT_EXPECTED_CALL(definition_mocks, open_get_max_frame_size(test_open_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2).SetReturn(1);

	/* we expect to close because of bad OPEN */
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:invalid-field"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
/* Tests_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
/* Tests_SRS_CONNECTION_01_167: [Both peers MUST accept frames of up to 512 (MIN-MAX-FRAME-SIZE) octets.] */
TEST_METHOD(when_an_open_frame_with_max_frame_size_511_is_received_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_open(TEST_OPEN_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &test_open_handle, sizeof(test_open_handle));
	uint32_t remote_max_frame_size = 511;
	STRICT_EXPECTED_CALL(definition_mocks, open_get_max_frame_size(test_open_handle, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));

	/* we expect to close because of bad OPEN */
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:invalid-field"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_006: [The open frame can only be sent on channel 0.] */
/* Tests_SRS_CONNECTION_01_222: [If an Open frame is received in a manner violating the ISO specification, the connection shall be closed with condition amqp:not-allowed and description being an implementation defined string.] */
TEST_METHOD(when_an_open_frame_is_received_on_channel_1_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));

	/* we expect to close because of bad OPEN */
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:not-allowed"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 1, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_223: [If the frame_received_callback is called with a NULL performative then the connection shall be closed with the error condition amqp:internal-error and an implementation defined error description.] */
TEST_METHOD(when_the_frame_received_callback_is_called_with_a_NULL_performative_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	/* we expect to close because of bad OPEN */
	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:internal-error"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 1, NULL, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_224: [START HDR HDR] */
TEST_METHOD(when_an_open_frame_is_indicated_as_received_before_even_opening_the_io_nothing_is_done)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char payload_bytes[] = { 0x42 };

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, payload_bytes, sizeof(payload_bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_226: [HDR_SENT OPEN HDR] */
TEST_METHOD(when_an_open_frame_is_indicated_as_received_before_the_header_exchange_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_226: [HDR_SENT OPEN HDR] */
TEST_METHOD(when_a_close_frame_is_received_in_HDR_SENT_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_229: [OPEN_SENT ** OPEN] */
/* Tests_SRS_CONNECTION_01_008: [Prior to closing a connection, each peer MUST write a close frame with a code indicating the reason for closing.] */
/* Codes_SRS_CONNECTION_01_238: [If set, this field indicates that the connection is being closed due to an error condition.] */
TEST_METHOD(when_a_close_frame_is_received_in_OPEN_SENT_a_CLOSE_is_sent)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE))
		.SetReturn(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE);
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	/* we expect to close with no error */
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_a_close_frame_is_sent_as_response_to_a_close_frame_and_creating_the_close_frame_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE))
		.SetReturn(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE);
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	/* we expect to close with no error */
	STRICT_EXPECTED_CALL(definition_mocks, close_create())
		.SetReturn((CLOSE_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_a_close_frame_is_sent_as_response_to_a_close_frame_and_creating_the_close_frame_AMQP_VALUE_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE))
		.SetReturn(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE);
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	/* we expect to close with no error */
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle))
		.SetReturn((AMQP_VALUE)NULL);
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
TEST_METHOD(when_a_close_frame_is_sent_as_response_to_a_close_frame_and_sending_the_frame_fails_the_connection_is_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE))
		.SetReturn(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE);
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	/* we expect to close with no error */
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_239: [If an Open frame is received in the Opened state the connection shall be closed with condition amqp:illegal-state and description being an implementation defined string.] */
TEST_METHOD(when_an_open_frame_is_received_in_open_the_connection_shall_be_closed_with_illegal_state)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));

	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:illegal-state"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_055: [DISCARDING The DISCARDING state is a variant of the CLOSE SENT state where the close is triggered by an error.] */
TEST_METHOD(when_an_open_frame_is_received_in_the_DISCARDING_state_the_connection_is_not_closed)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_010: [After writing this frame the peer SHOULD continue to read from the connection until it receives the partner’s close frame ] */
/* Tests_SRS_CONNECTION_01_240: [There is no requirement for an implementation to read from a socket after a close performative has been received.] */
TEST_METHOD(when_in_discarding_state_the_connection_still_looks_for_the_close_frame_and_then_closes_the_io)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_012: [A close frame MAY be received on any channel up to the maximum channel number negotiated in open.] */
TEST_METHOD(when_a_CLOSE_frame_is_received_on_channel_1_it_is_still_valid)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	// act
	saved_frame_received_callback(saved_callback_context, 1, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_242: [The connection module shall accept CLOSE frames even if they have extra payload bytes besides the Close performative.] */
TEST_METHOD(when_a_CLOSE_frame_with_1_byte_payload_is_received_it_is_still_valid)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	unsigned char payload_bytes[] = { 0x42 };

	// act
	saved_frame_received_callback(saved_callback_context, 1, TEST_CLOSE_PERFORMATIVE, payload_bytes, sizeof(payload_bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_242: [The connection module shall accept CLOSE frames even if they have extra payload bytes besides the Close performative.] */
TEST_METHOD(when_an_OPEN_frame_with_1_byte_payload_is_received_it_is_still_valid)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_open(TEST_OPEN_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &test_open_handle, sizeof(test_open_handle));
	uint32_t remote_max_frame_size = 1024;
	STRICT_EXPECTED_CALL(definition_mocks, open_get_max_frame_size(test_open_handle, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));

	unsigned char payload_bytes[] = { 0x42 };

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, payload_bytes, sizeof(payload_bytes));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_012: [A close frame MAY be received on any channel up to the maximum channel number negotiated in open.] */
TEST_METHOD(when_a_CLOSE_FRAME_is_received_on_a_channel_higher_than_the_max_negotiated_channel_a_close_with_invalid_field_shall_be_done)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, "1234");
	(void)connection_set_channel_max(connection, 0);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));

	STRICT_EXPECTED_CALL(definition_mocks, error_create("amqp:invalid-field"));
	STRICT_EXPECTED_CALL(definition_mocks, error_set_description(test_error_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, close_set_error(test_close_handle, test_error_handle));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, error_destroy(test_error_handle));

	// act
	saved_frame_received_callback(saved_callback_context, 1, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* connection_create_endpoint */

/* Tests_SRS_CONNECTION_01_113: [If connection, frame_received_callback or connection_state_changed_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(connection_create_endpoint_with_NULL_conneciton_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	ENDPOINT_HANDLE result = connection_create_endpoint(NULL, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_CONNECTION_01_113: [If connection, frame_received_callback or connection_state_changed_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(connection_create_endpoint_with_NULL_frame_receive_callback_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE result = connection_create_endpoint(connection, NULL, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_113: [If connection, frame_received_callback or connection_state_changed_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(connection_create_endpoint_with_NULL_connection_state_changed_callback_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE result = connection_create_endpoint(connection, test_on_frame_received, NULL, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_112: [connection_create_endpoint shall create a new endpoint that can be used by a session.] */
/* Tests_SRS_CONNECTION_01_127: [On success, connection_create_endpoint shall return a non-NULL handle to the newly created endpoint.] */
/* Tests_SRS_CONNECTION_01_197: [The newly created endpoint shall be added to the endpoints list, so that it can be tracked.] */
TEST_METHOD(connection_create_endpoint_with_valid_arguments_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

	// act
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NOT_NULL(endpoint);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_196: [If memory cannot be allocated for the new endpoint, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_fails_connection_create_endpoint_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(endpoint);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_198: [If adding the endpoint to the endpoints list tracked by the connection fails, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(when_realloc_for_the_endpoint_list_fails_connection_create_endpoint_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(endpoint);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_193: [The context argument shall be allowed to be NULL.] */
TEST_METHOD(connection_create_endpoint_with_valid_arguments_and_NULL_context_succeeds)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

	// act
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, NULL);

	// assert
	ASSERT_IS_NOT_NULL(endpoint);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_115: [If no more endpoints can be created due to all channels being used, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(when_no_more_channels_are_available_connection_create_endpoint_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_channel_max(connection, 0);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(endpoint1);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_115: [If no more endpoints can be created due to all channels being used, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(when_no_more_channels_are_available_after_create_destroy_and_create_again_connection_create_endpoint_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_channel_max(connection, 0);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	connection_destroy_endpoint(endpoint0);
	endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(endpoint1);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_115: [If no more endpoints can be created due to all channels being used, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(when_no_more_channels_are_available_with_channel_max_1_connection_create_endpoint_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_channel_max(connection, 1);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE endpoint2 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(endpoint1);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

/* connection_destroy_endpoint */

/* Tests_SRS_CONNECTION_01_199: [If endpoint is NULL, connection_destroy_endpoint shall do nothing.] */
TEST_METHOD(connection_destroy_endpoint_with_NULL_argument_does_nothing)
{
	// arrange
	connection_mocks mocks;

	// act
	connection_destroy_endpoint(NULL);

	// assert
	// no explicit assert, uMock checks the calls
}

/* Tests_SRS_CONNECTION_01_129: [connection_destroy_endpoint shall free all resources associated with an endpoint created by connection_create_endpoint.] */
/* Tests_SRS_CONNECTION_01_130: [The outgoing channel associated with the endpoint shall be released by removing the endpoint from the endpoint list.] */
TEST_METHOD(connection_destroy_endpoint_frees_the_resources_associated_with_the_endpoint)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	connection_destroy_endpoint(endpoint);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_129: [connection_destroy_endpoint shall free all resources associated with an endpoint created by connection_create_endpoint.] */
/* Tests_SRS_CONNECTION_01_130: [The outgoing channel associated with the endpoint shall be released by removing the endpoint from the endpoint list.] */
/* Tests_SRS_CONNECTION_01_131: [Any incoming channel number associated with the endpoint shall be released.] */
TEST_METHOD(when_reallocating_the_endpoints_list_fails_connection_destroy_endpoint_shall_still_free_all_resources)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	connection_destroy_endpoint(endpoint);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_130: [The outgoing channel associated with the endpoint shall be released by removing the endpoint from the endpoint list.] */
TEST_METHOD(when_an_endpoint_is_released_another_one_can_be_created_in_its_place)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	(void)connection_set_channel_max(connection, 2);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint2 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	connection_destroy_endpoint(endpoint1);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

	// act
	endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NOT_NULL(endpoint1);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy_endpoint(endpoint2);
	connection_destroy(connection);
}

/* connection_encode_frame */

/* Tests_SRS_CONNECTION_01_249: [If endpoint or performative are NULL, connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(connection_encode_frame_with_NULL_endpoint_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	int result = connection_encode_frame(NULL, TEST_BEGIN_PERFORMATIVE, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_249: [If endpoint or performative are NULL, connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(connection_encode_frame_with_NULL_performative_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();

	// act
	int result = connection_encode_frame(endpoint, NULL, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_247: [connection_encode_frame shall send a frame for a certain endpoint.] */
/* Tests_SRS_CONNECTION_01_248: [On success it shall return 0.] */
/* Tests_SRS_CONNECTION_01_250: [connection_encode_frame shall initiate the frame send by calling amqp_frame_codec_begin_encode_frame.] */
/* Tests_SRS_CONNECTION_01_251: [The channel number passed to amqp_frame_codec_begin_encode_frame shall be the outgoing channel number associated with the endpoint by connection_create_endpoint.] */
/* Tests_SRS_CONNECTION_01_252: [The performative passed to amqp_frame_codec_begin_encode_frame shall be the performative argument of connection_encode_frame.] */
/* Tests_SRS_CONNECTION_01_255: [The payload size shall be computed based on all the payload chunks passed as argument in payloads.] */
TEST_METHOD(connection_encode_frame_sends_the_frame)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, 0));

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, NULL, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_255: [The payload size shall be computed based on all the payload chunks passed as argument in payloads.] */
/* Tests_SRS_CONNECTION_01_256: [Each payload passed in the payloads array shall be passed to amqp_frame_codec by calling amqp_frame_codec_encode_payload_bytes.] */
TEST_METHOD(connection_encode_frame_with_1_payload_adds_the_bytes_to_the_frame_payload)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload[] = { 0x42 };
	PAYLOAD payload = { test_payload, sizeof(test_payload) };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, sizeof(test_payload)));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload, sizeof(test_payload)))
		.ValidateArgumentBuffer(2, test_payload, sizeof(test_payload));

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, &payload, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_255: [The payload size shall be computed based on all the payload chunks passed as argument in payloads.] */
/* Tests_SRS_CONNECTION_01_256: [Each payload passed in the payloads array shall be passed to amqp_frame_codec by calling amqp_frame_codec_encode_payload_bytes.] */
TEST_METHOD(connection_encode_frame_with_1_payload_of_2_bytes_adds_the_bytes_to_the_frame_payload)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload[] = { 0x42, 0x43 };
	PAYLOAD payload = { test_payload, sizeof(test_payload) };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, sizeof(test_payload)));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload, sizeof(test_payload)))
		.ValidateArgumentBuffer(2, test_payload, sizeof(test_payload));

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, &payload, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_255: [The payload size shall be computed based on all the payload chunks passed as argument in payloads.] */
/* Tests_SRS_CONNECTION_01_256: [Each payload passed in the payloads array shall be passed to amqp_frame_codec by calling amqp_frame_codec_encode_payload_bytes.] */
TEST_METHOD(connection_encode_frame_with_2_payloads_of_1_byte_rach_adds_the_bytes_to_the_frame_payload)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload1[] = { 0x42 };
	unsigned char test_payload2[] = { 0x43 };
	PAYLOAD payloads[] = { { test_payload1, sizeof(test_payload1) }, { test_payload2, sizeof(test_payload2) } };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, 2));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload1, sizeof(test_payload1)))
		.ValidateArgumentBuffer(2, test_payload1, sizeof(test_payload1));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload2, sizeof(test_payload2)))
		.ValidateArgumentBuffer(2, test_payload2, sizeof(test_payload2));

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, payloads, 2);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_253: [If amqp_frame_codec_begin_encode_frame or amqp_frame_codec_encode_payload_bytes fails, then connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_amqp_frame_codec_begin_encode_frame_fails_then_connection_encode_frame_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload1[] = { 0x42 };
	unsigned char test_payload2[] = { 0x43 };
	PAYLOAD payloads[] = { { test_payload1, sizeof(test_payload1) }, { test_payload2, sizeof(test_payload2) } };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, 2))
		.SetReturn(1);

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, payloads, 2);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_253: [If amqp_frame_codec_begin_encode_frame or amqp_frame_codec_encode_payload_bytes fails, then connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_amqp_frame_codec_encode_payload_bytes_frame_fails_then_connection_encode_frame_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload1[] = { 0x42 };
	unsigned char test_payload2[] = { 0x43 };
	PAYLOAD payloads[] = { { test_payload1, sizeof(test_payload1) }, { test_payload2, sizeof(test_payload2) } };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, 2));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload1, sizeof(test_payload1)))
		.ValidateArgumentBuffer(2, test_payload1, sizeof(test_payload1))
		.SetReturn(1);

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, payloads, 2);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_253: [If amqp_frame_codec_begin_encode_frame or amqp_frame_codec_encode_payload_bytes fails, then connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_a_second_call_to_amqp_frame_codec_encode_payload_bytes_frame_fails_then_connection_encode_frame_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload1[] = { 0x42 };
	unsigned char test_payload2[] = { 0x43 };
	PAYLOAD payloads[] = { { test_payload1, sizeof(test_payload1) }, { test_payload2, sizeof(test_payload2) } };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, 2));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload1, sizeof(test_payload1)))
		.ValidateArgumentBuffer(2, test_payload1, sizeof(test_payload1));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_encode_payload_bytes(TEST_AMQP_FRAME_CODEC_HANDLE, test_payload2, sizeof(test_payload2)))
		.ValidateArgumentBuffer(2, test_payload2, sizeof(test_payload2))
		.SetReturn(1);

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, payloads, 2);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_254: [If connection_encode_frame is called before the connection is in the OPENED state, connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(connection_encode_frame_when_connection_is_not_opened_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, NULL, 0);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_253: [If amqp_frame_codec_begin_encode_frame or amqp_frame_codec_encode_payload_bytes fails, then connection_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(connection_encode_frame_after_close_has_been_received_fails)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);

	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	unsigned char test_payload1[] = { 0x42 };
	unsigned char test_payload2[] = { 0x43 };
	PAYLOAD payloads[] = { { test_payload1, sizeof(test_payload1) }, { test_payload2, sizeof(test_payload2) } };

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	// act
	int result = connection_encode_frame(endpoint, TEST_BEGIN_PERFORMATIVE, payloads, 2);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_251: [The channel number passed to amqp_frame_codec_begin_encode_frame shall be the outgoing channel number associated with the endpoint by connection_create_endpoint.] */
/* Tests_SRS_CONNECTION_01_128: [The lowest number outgoing channel shall be associated with the newly created endpoint.] */
TEST_METHOD(connection_encode_frame_with_a_second_endpoint_sends_on_channel_1)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 1, TEST_BEGIN_PERFORMATIVE, 0));

	// act
	int result = connection_encode_frame(endpoint1, TEST_BEGIN_PERFORMATIVE, NULL, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_251: [The channel number passed to amqp_frame_codec_begin_encode_frame shall be the outgoing channel number associated with the endpoint by connection_create_endpoint.] */
/* Tests_SRS_CONNECTION_01_128: [The lowest number outgoing channel shall be associated with the newly created endpoint.] */
TEST_METHOD(when_an_endpoint_is_destroyed_and_a_new_one_is_created_the_channel_is_reused_on_the_new_endpoint)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	connection_destroy_endpoint(endpoint0);
	endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, TEST_BEGIN_PERFORMATIVE, 0));

	// act
	int result = connection_encode_frame(endpoint0, TEST_BEGIN_PERFORMATIVE, NULL, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_258: [connection_state_changed_callback shall be invoked whenever the connection state changes.] */
/* Tests_SRS_CONNECTION_01_260: [Each endpoint’s connection_state_changed_callback shall be called.] */
/* Tests_SRS_CONNECTION_01_259: [As context, the callback_context passed in connection_create_endpoint shall be given.] */
TEST_METHOD(when_state_changes_to_HDR_SENT_all_endpoints_are_notified)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, NULL);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).IgnoreAllCalls();
	EXPECTED_CALL(mocks, io_send(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)).IgnoreAllCalls();
	EXPECTED_CALL(mocks, io_dowork(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(TEST_CONTEXT, CONNECTION_STATE_HDR_SENT, CONNECTION_STATE_START));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(NULL, CONNECTION_STATE_HDR_SENT, CONNECTION_STATE_START));

	// act
	connection_dowork(connection);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_258: [connection_state_changed_callback shall be invoked whenever the connection state changes.] */
/* Tests_SRS_CONNECTION_01_260: [Each endpoint’s connection_state_changed_callback shall be called.] */
/* Tests_SRS_CONNECTION_01_259: [As context, the callback_context passed in connection_create_endpoint shall be given.] */
TEST_METHOD(when_state_changes_to_HDR_EXCH_and_HDR_OPEN_SENT_all_endpoints_are_notified)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, NULL);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_hostname(test_open_handle, "testhost"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_max_frame_size(test_open_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_channel_max(test_open_handle, 65535));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_open(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_open_amqp_value, 0));
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_open_amqp_value));

	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(TEST_CONTEXT, CONNECTION_STATE_HDR_EXCH, CONNECTION_STATE_HDR_SENT));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(NULL, CONNECTION_STATE_HDR_EXCH, CONNECTION_STATE_HDR_SENT));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(TEST_CONTEXT, CONNECTION_STATE_OPEN_SENT, CONNECTION_STATE_HDR_EXCH));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(NULL, CONNECTION_STATE_OPEN_SENT, CONNECTION_STATE_HDR_EXCH));

	// act
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_258: [connection_state_changed_callback shall be invoked whenever the connection state changes.] */
/* Tests_SRS_CONNECTION_01_260: [Each endpoint’s connection_state_changed_callback shall be called.] */
/* Tests_SRS_CONNECTION_01_259: [As context, the callback_context passed in connection_create_endpoint shall be given.] */
TEST_METHOD(when_state_changes_to_OPENED_all_endpoints_are_notified)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, NULL);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_OPEN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_open(TEST_OPEN_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &test_open_handle, sizeof(test_open_handle));
	STRICT_EXPECTED_CALL(definition_mocks, open_get_max_frame_size(test_open_handle, IGNORED_PTR_ARG))
		.IgnoreArgument(2);
	STRICT_EXPECTED_CALL(definition_mocks, open_destroy(test_open_handle));

	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(TEST_CONTEXT, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(NULL, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_258: [connection_state_changed_callback shall be invoked whenever the connection state changes.] */
/* Tests_SRS_CONNECTION_01_260: [Each endpoint’s connection_state_changed_callback shall be called.] */
/* Tests_SRS_CONNECTION_01_259: [As context, the callback_context passed in connection_create_endpoint shall be given.] */
TEST_METHOD(when_state_changes_to_CLOSE_RCVD_and_END_SENT_all_endpoints_are_notified)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	ENDPOINT_HANDLE endpoint0 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, TEST_CONTEXT);
	ENDPOINT_HANDLE endpoint1 = connection_create_endpoint(connection, test_on_frame_received, test_on_connection_state_changed, NULL);
	STRICT_EXPECTED_CALL(mocks, io_get_state(TEST_IO_HANDLE)).SetReturn(IO_STATE_NOT_OPEN);
	connection_dowork(connection);
	connection_dowork(connection);
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));
	saved_frame_received_callback(saved_callback_context, 0, TEST_OPEN_PERFORMATIVE, 0, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_CLOSE_PERFORMATIVE))
		.SetReturn(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE);
	STRICT_EXPECTED_CALL(definition_mocks, is_open_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE))
		.SetReturn(false);
	STRICT_EXPECTED_CALL(definition_mocks, is_close_type_by_descriptor(TEST_CLOSE_DESCRIPTOR_AMQP_VALUE));
	CLOSE_HANDLE received_test_close_handle = (CLOSE_HANDLE)0x4000;
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_get_close(TEST_CLOSE_PERFORMATIVE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &received_test_close_handle, sizeof(received_test_close_handle));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(received_test_close_handle));

	/* we expect to close with no error */
	STRICT_EXPECTED_CALL(definition_mocks, close_create());
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_close(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, amqp_frame_codec_begin_encode_frame(TEST_AMQP_FRAME_CODEC_HANDLE, 0, test_close_amqp_value, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_close_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, close_destroy(test_close_handle));
	STRICT_EXPECTED_CALL(mocks, io_close(TEST_IO_HANDLE));

	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(TEST_CONTEXT, CONNECTION_STATE_CLOSE_RCVD, CONNECTION_STATE_OPENED));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(NULL, CONNECTION_STATE_CLOSE_RCVD, CONNECTION_STATE_OPENED));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(TEST_CONTEXT, CONNECTION_STATE_END, CONNECTION_STATE_CLOSE_RCVD));
	STRICT_EXPECTED_CALL(mocks, test_on_connection_state_changed(NULL, CONNECTION_STATE_END, CONNECTION_STATE_CLOSE_RCVD));

	// act
	saved_frame_received_callback(saved_callback_context, 0, TEST_CLOSE_PERFORMATIVE, 0, NULL);

	// assert
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint0);
	connection_destroy_endpoint(endpoint1);
	connection_destroy(connection);
}

END_TEST_SUITE(connection_unittests)
