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

/* Requirements implictly tested */
/* Tests_SRS_CONNECTION_01_088: [Any data appearing beyond the protocol header MUST match the version indicated by the protocol header.] */
/* Tests_SRS_CONNECTION_01_039: [START In this state a connection exists, but nothing has been sent or received. This is the state an implementation would be in immediately after performing a socket connect or socket accept.] */

#define TEST_IO_HANDLE					(IO_HANDLE)0x4242
#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4243
#define TEST_AMQP_FRAME_CODEC_HANDLE	(AMQP_FRAME_CODEC_HANDLE)0x4244
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4245
#define TEST_LIST_ITEM_AMQP_VALUE		(AMQP_VALUE)0x4246
#define TEST_DESCRIBED_AMQP_VALUE		(AMQP_VALUE)0x4247
#define TEST_AMQP_OPEN_FRAME_HANDLE		(AMQP_OPEN_FRAME_HANDLE)0x4245
#define TEST_LIST_HANDLE				(LIST_HANDLE)0x4246

#define TEST_CONTEXT					(void*)(0x4242)

static const char test_container_id[] = "1234";

const IO_INTERFACE_DESCRIPTION test_io_interface_description = { 0 };

static IO_RECEIVE_CALLBACK io_receive_callback;
static void* io_receive_callback_context;
static uint64_t performative_ulong;
static const void** list_items = NULL;
static size_t list_item_count = 0;

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
	MOCK_METHOD_END(IO_STATE, IO_STATE_READY);

	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_2(, void*, amqpalloc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, realloc(ptr, size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* frame_codec */
	MOCK_STATIC_METHOD_2(, FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, LOGGER_LOG, logger_log)
	MOCK_METHOD_END(FRAME_CODEC_HANDLE, TEST_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, frame_codec_receive_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, buffer, size_t, size)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, frame_codec_set_max_frame_size, FRAME_CODEC_HANDLE, frame_codec, uint32_t, max_frame_size)
	MOCK_METHOD_END(int, 0);

	/* amqp_frame_codec */
	MOCK_STATIC_METHOD_5(, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_create, FRAME_CODEC_HANDLE, frame_codec, AMQP_FRAME_RECEIVED_CALLBACK, frame_received_callback, AMQP_EMPTY_FRAME_RECEIVED_CALLBACK, empty_frame_received_callback, AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK, payload_bytes_received_callback, void*, frame_received_callback_context)
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
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_get_descriptor, AMQP_VALUE, value)
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
	MOCK_STATIC_METHOD_3(, void, test_frame_received_callback, void*, context, AMQP_VALUE, performative, uint32_t, frame_payload_size)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, test_frame_payload_bytes_received_callback, void*, context, const unsigned char*, payload_bytes, uint32_t, byte_count)
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

	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, frame_codec_destroy, FRAME_CODEC_HANDLE, frame_codec);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, frame_codec_receive_bytes, FRAME_CODEC_HANDLE, frame_codec, const unsigned char*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, frame_codec_set_max_frame_size, FRAME_CODEC_HANDLE, frame_codec, uint32_t, max_frame_size);

	DECLARE_GLOBAL_MOCK_METHOD_5(connection_mocks, , AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_create, FRAME_CODEC_HANDLE, frame_codec, AMQP_FRAME_RECEIVED_CALLBACK, frame_received_callback, AMQP_EMPTY_FRAME_RECEIVED_CALLBACK, empty_frame_received_callback, AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK, payload_bytes_received_callback, void*, frame_received_callback_context);
    DECLARE_GLOBAL_MOCK_METHOD_4(connection_mocks, , int, amqp_frame_codec_begin_encode_frame, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec, uint16_t, channel, const AMQP_VALUE, performative, uint32_t, payload_size);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, amqp_frame_codec_destroy, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, amqp_frame_codec_encode_payload_bytes, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec, const unsigned char*, bytes, uint32_t, count);

	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , AMQP_VALUE, amqpvalue_get_descriptor, AMQP_VALUE, value);
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

	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , void, test_frame_received_callback, void*, context, AMQP_VALUE, performative, uint32_t, frame_payload_size);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , void, test_frame_payload_bytes_received_callback, void*, context, const unsigned char*, payload_bytes, uint32_t, byte_count);

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
	performative_ulong = 0x10;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
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
	amqp_definitions_mocks amqp_definitions_mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);

	// assert
	ASSERT_IS_NOT_NULL(connection);
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_START, connection_state);
}

/* Tests_SRS_CONNECTION_01_001: [connection_create shall open a new connection to a specified host/port.] */
/* Tests_SRS_CONNECTION_01_082: [connection_create shall allocate a new frame_codec instance to be used for frame encoding/decoding.] */
/* Tests_SRS_CONNECTION_01_107: [connection_create shall create an amqp_frame_codec instance by calling amqp_frame_codec_create.] */
/* Tests_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
TEST_METHOD(connection_create_with_valid_args_but_NULL_host_name_succeeds)
{
	// arrange
	connection_mocks mocks;
	amqp_definitions_mocks amqp_definitions_mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqp_frame_codec_create(TEST_FRAME_CODEC_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, NULL, test_container_id);

	// assert
	ASSERT_IS_NOT_NULL(connection);
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_START, connection_state);
}

/* Tests_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
TEST_METHOD(when_allocating_memory_fails_then_connection_create_fails)
{
	// arrange
	connection_mocks mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

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
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG))
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
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG))
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
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG))
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
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG))
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

/* connection_create_endpoint */

/* Tests_SRS_CONNECTION_01_113: [If connection, frame_received_callback or frame_payload_bytes_received_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(connection_create_endpoint_with_NULL_conneciton_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	ENDPOINT_HANDLE result = connection_create_endpoint(NULL, test_frame_received_callback, test_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_CONNECTION_01_113: [If connection, frame_received_callback or frame_payload_bytes_received_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(connection_create_endpoint_with_NULL_frame_receive_callback_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE result = connection_create_endpoint(connection, NULL, test_frame_payload_bytes_received_callback, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(result);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_113: [If connection, frame_received_callback or frame_payload_bytes_received_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
TEST_METHOD(connection_create_endpoint_with_NULL_frame_payload_bytes_received_callback_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create(TEST_IO_HANDLE, "testhost", test_container_id);
	mocks.ResetAllCalls();

	// act
	ENDPOINT_HANDLE result = connection_create_endpoint(connection, NULL, test_frame_payload_bytes_received_callback, TEST_CONTEXT);

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
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_frame_received_callback, test_frame_payload_bytes_received_callback, TEST_CONTEXT);

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
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_frame_received_callback, test_frame_payload_bytes_received_callback, TEST_CONTEXT);

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
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_frame_received_callback, test_frame_payload_bytes_received_callback, TEST_CONTEXT);

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
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_frame_received_callback, test_frame_payload_bytes_received_callback, NULL);

	// assert
	ASSERT_IS_NOT_NULL(endpoint);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy_endpoint(endpoint);
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
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_frame_received_callback, test_frame_payload_bytes_received_callback, TEST_CONTEXT);
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
	ENDPOINT_HANDLE endpoint = connection_create_endpoint(connection, test_frame_received_callback, test_frame_payload_bytes_received_callback, TEST_CONTEXT);
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
		.SetReturn(IO_STATE_NOT_READY);
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
/* Tests_SRS_CONNECTION_01_087: [The protocol header consists of the upper case ASCII letters �AMQP� followed by a protocol id of zero, followed by three unsigned bytes representing the major, minor, and revision of the protocol version (currently 1 (MAJOR), 0 (MINOR), 0 (REVISION)). In total this is an 8-octet sequence] */
/* Tests_SRS_CONNECTION_01_091: [The AMQP peer which acted in the role of the TCP client (i.e. the peer that actively opened the connection) MUST immediately send its outgoing protocol header on establishment of the TCP connection.] */
/* Tests_SRS_CONNECTION_01_093: [_ When the client opens a new socket connection to a server, it MUST send a protocol header with the client�s preferred protocol version.] */
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
		.SetReturn(IO_STATE_READY);
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
		.SetReturn(IO_STATE_NOT_READY);
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
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
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
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
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
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
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
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
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
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_HDR_SENT, connection_state);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	connection_destroy(connection);
}

/* Tests_SRS_CONNECTION_01_134: [The container id field shall be filled with the container id specified in connection_create.] */
/* Tests_SRS_CONNECTION_01_135: [If hostname has been specified by a call to connection_set_hostname, then that value shall be stamped in the open frame.] */
/* Tests_SRS_CONNECTION_01_205: [Sending the AMQP OPEN frame shall be done by calling amqp_frame_codec_begin_encode_frame with channel number 0, the actual performative payload and 0 as payload_size.] */
/* Tests_SRS_CONNECTION_01_151: [Max_frame_size shall be passed down to the frame_codec when the Open frame is sent.] */
/* Tests_SRS_CONNECTION_01_138: [If no max_frame_size value has been specified, no value shall be stamped in the open frame (no call to open_set_max_frame_size shall be made).] */
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

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
	STRICT_EXPECTED_CALL(definition_mocks, open_set_hostname(test_open_handle, "testhost"));
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

/* Tests_SRS_CONNECTION_01_207: [If frame_codec_set_max_frame_size fails the connection shall be closed.] */
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

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and setto the END state.] */
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

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and setto the END state.] */
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

/* Tests_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and setto the END state.] */
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

/* tESTS_SRS_CONNECTION_01_136: [If no hostname value has been specified, no value shall be stamped in the open frame (no call to open_set_hostname shall be made).] */
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

	STRICT_EXPECTED_CALL(mocks, frame_codec_set_max_frame_size(TEST_FRAME_CODEC_HANDLE, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, open_create("1234"));
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

END_TEST_SUITE(connection_unittests)
