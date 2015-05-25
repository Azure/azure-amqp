#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "connection.h"
#include "io.h"
#include "socketio.h"
#include "open_frame.h"
#include "close_frame.h"
#include "frame_codec.h"
#include "amqp_frame_codec.h"

/* Requirements implictly tested */
/* Tests_SRS_CONNECTION_01_088: [Any data appearing beyond the protocol header MUST match the version indicated by the protocol header.] */

#define TEST_IO_HANDLE					(IO_HANDLE)0x4242
#define TEST_FRAME_CODEC_HANDLE			(FRAME_CODEC_HANDLE)0x4243
#define TEST_AMQP_FRAME_CODEC_HANDLE	(AMQP_FRAME_CODEC_HANDLE)0x4244

const IO_INTERFACE_DESCRIPTION test_io_interface_description = { 0 };

static IO_RECEIVE_CALLBACK io_receive_callback;
static void* io_receive_callback_context;

TYPED_MOCK_CLASS(connection_mocks, CGlobalMock)
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

	/* socketio mocks */
	MOCK_STATIC_METHOD_0(, const IO_INTERFACE_DESCRIPTION*, socketio_get_interface_description)
	MOCK_METHOD_END(const IO_INTERFACE_DESCRIPTION*, &test_io_interface_description);

	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* amqp_frame_codec */
	MOCK_STATIC_METHOD_2(, int, open_frame_encode, FRAME_CODEC_HANDLE, frame_codec, const char*, container_id)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, int, close_frame_encode, FRAME_CODEC_HANDLE, frame_codec)
	MOCK_METHOD_END(int, 0);

	/* frame_codec */
	MOCK_STATIC_METHOD_4(, FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, FRAME_RECEIVED_CALLBACK, frame_received_callback, void*, context, LOGGER_LOG, logger_log)
	MOCK_METHOD_END(FRAME_CODEC_HANDLE, TEST_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, frame_codec_destroy, FRAME_CODEC_HANDLE, handle)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, frame_codec_receive_bytes, FRAME_CODEC_HANDLE, handle, const void*, buffer, size_t, size)
	MOCK_METHOD_END(int, 0);

	/* amqp_frame_codec */
	MOCK_STATIC_METHOD_1(, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_create, FRAME_CODEC_HANDLE, frame_codec_handle)
	MOCK_METHOD_END(AMQP_FRAME_CODEC_HANDLE, TEST_AMQP_FRAME_CODEC_HANDLE);
	MOCK_STATIC_METHOD_1(, void, amqp_frame_codec_destroy, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_handle)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_5(connection_mocks, , IO_HANDLE, io_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, context, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, io_destroy, IO_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, io_send, IO_HANDLE, handle, const void*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , int, io_dowork, IO_HANDLE, handle);

	DECLARE_GLOBAL_MOCK_METHOD_0(connection_mocks, , const IO_INTERFACE_DESCRIPTION*, socketio_get_interface_description);

	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_2(connection_mocks, , int, open_frame_encode, FRAME_CODEC_HANDLE, frame_codec, const char*, container_id);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , int, close_frame_encode, FRAME_CODEC_HANDLE, frame_codec);

	DECLARE_GLOBAL_MOCK_METHOD_4(connection_mocks, , FRAME_CODEC_HANDLE, frame_codec_create, IO_HANDLE, io, FRAME_RECEIVED_CALLBACK, frame_received_callback, void*, context, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, frame_codec_destroy, FRAME_CODEC_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(connection_mocks, , int, frame_codec_receive_bytes, FRAME_CODEC_HANDLE, handle, const void*, buffer, size_t, size);

	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_create, FRAME_CODEC_HANDLE, frame_codec_handle);
	DECLARE_GLOBAL_MOCK_METHOD_1(connection_mocks, , void, amqp_frame_codec_destroy, AMQP_FRAME_CODEC_HANDLE, amqp_frame_codec_handle);

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

/* connection_create */

/* Tests_SRS_CONNECTION_01_001: [connection_create shall open a new connection to a specified host/port.] */
/* Tests_SRS_CONNECTION_01_067: [connection_create shall call io_create to create its TCP IO interface.] */
/* Tests_SRS_CONNECTION_01_068: [connection_create shall pass to io_create the interface obtained by a call to socketio_get_interface_description.] */
/* Tests_SRS_CONNECTION_01_069: [The socket_io parameters shall be filled in with the host and port information passed to connection_create.] */
/* Tests_SRS_CONNECTION_01_082: [connection_create shall allocate a new frame_codec instance to be used for frame encoding/decoding.] */
/* Tests_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
TEST_METHOD(connection_create_with_valid_args_succeeds)
{
	// arrange
	connection_mocks mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, socketio_get_interface_description());
	EXPECTED_CALL(mocks, io_create(&test_io_interface_description, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);

	// act
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);

	// assert
	ASSERT_IS_NOT_NULL(connection);
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_START, connection_state);
}

/* Tests_SRS_CONNECTION_01_070: [If io_create fails then connection_create shall return NULL.] */
TEST_METHOD(when_io_create_fails_then_connection_create_fails)
{
	// arrange
	connection_mocks mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, socketio_get_interface_description());
	EXPECTED_CALL(mocks, io_create(&test_io_interface_description, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn((IO_HANDLE)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_070: [If io_create fails then connection_create shall return a non-zero value.] */
TEST_METHOD(when_frame_codec_create_fails_then_connection_create_fails)
{
	// arrange
	connection_mocks mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, socketio_get_interface_description());
	EXPECTED_CALL(mocks, io_create(&test_io_interface_description, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, frame_codec_create(TEST_IO_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn((FRAME_CODEC_HANDLE)NULL);
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_071: [If host is NULL, connection_create shall return NULL.] */
TEST_METHOD(connection_create_with_NULL_host_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	CONNECTION_HANDLE connection = connection_create(NULL, 5672);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_080: [If socketio_get_interface_description fails, connection_create shall return NULL.] */
TEST_METHOD(when_getting_socketio_interface_description_fails_connection_create_fails)
{
	// arrange
	connection_mocks mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, socketio_get_interface_description())
		.SetReturn((IO_INTERFACE_DESCRIPTION*)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);

	// assert
	ASSERT_IS_NULL(connection);
}

/* Tests_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
TEST_METHOD(when_allocating_memory_fails_connection_create_fails)
{
	// arrange
	connection_mocks mocks;
	SOCKETIO_CONFIG config = { "testhost", 5672 };

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);

	// assert
	ASSERT_IS_NULL(connection);
}

/* connection_destroy */

/* Tests_SRS_CONNECTION_01_073: [connection_destroy shall free all resources associated with a connection.] */
TEST_METHOD(connection_destroy_frees_resources)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();

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

/* connection_dowork */

/* Tests_SRS_CONNECTION_01_076: [connection_dowork shall schedule the underlying IO interface to do its work by calling io_dowork.] */
/* Tests_SRS_CONNECTION_01_085: [On success, connection_dowork shall return 0.] */
/* Tests_SRS_CONNECTION_01_084: [The connection state machine implementing the protocol requirements shall be run as part of connection_dowork.] */
/* Tests_SRS_CONNECTION_01_086: [Prior to sending any frames on a connection, each peer MUST start by sending a protocol header that indicates the protocol version used on the connection.] */
/* Tests_SRS_CONNECTION_01_087: [The protocol header consists of the upper case ASCII letters “AMQP” followed by a protocol id of zero, followed by three unsigned bytes representing the major, minor, and revision of the protocol version (currently 1 (MAJOR), 0 (MINOR), 0 (REVISION)). In total this is an 8-octet sequence] */
/* Tests_SRS_CONNECTION_01_091: [The AMQP peer which acted in the role of the TCP client (i.e. the peer that actively opened the connection) MUST immediately send its outgoing protocol header on establishment of the TCP connection.] */
/* Tests_SRS_CONNECTION_01_093: [_ When the client opens a new socket connection to a server, it MUST send a protocol header with the client’s preferred protocol version.] */
/* Tests_SRS_CONNECTION_01_104: [Sending the protocol header shall be done by using io_send.] */
/* Tests_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
TEST_METHOD(connection_dowork_when_state_is_start_sends_the_AMQP_header_and_triggers_io_dowork)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	int result = connection_dowork(connection);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_HDR_SENT, connection_state);
}

/* Tests_SRS_CONNECTION_01_077: [If io_dowork fails, connection_dowork shall return a non-zero value.] */
TEST_METHOD(when_io_dowork_fails_then_connection_dowork_fails)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE))
		.SetReturn(1);

	// act
	int result = connection_dowork(connection);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_078: [If handle is NULL, connection_dowork shall return a non-zero value.] */
TEST_METHOD(connection_dowork_with_NULL_handle_fails)
{
	// arrange
	connection_mocks mocks;

	// act
	int result = connection_dowork(NULL);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_CONNECTION_01_057: [END In this state it is illegal for either endpoint to write anything more onto the connection. The connection can be safely closed and discarded.] */
/* Tests_SRS_CONNECTION_01_106: [When sending the protocol header fails, the connection shall be immediately closed.] */
/* Tests_SRS_CONNECTION_01_105: [When io_send fails, connection_dowork shall return a non-zero value.] */
TEST_METHOD(when_sending_the_header_fails_connection_dowork_fails_and_io_is_destroyed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));

	// act
	int result = connection_dowork(connection);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_headers_do_not_match_connection_gets_closed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'Q', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_header_first_byte_does_not_match_connection_gets_closed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'B' };

	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_header_last_byte_does_not_match_connection_gets_closed)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 1 };

	STRICT_EXPECTED_CALL(mocks, io_destroy(TEST_IO_HANDLE));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_END, connection_state);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
TEST_METHOD(when_protocol_header_first_byte_matches_but_only_1st_byte_received_no_io_destroy_is_done)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A' };

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_HDR_SENT, connection_state);
}

/* Tests_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
/* Tests_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
/* Tests_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.]  */
/* Tests_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
/* Tests_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
TEST_METHOD(when_protocol_header_matches_open_is_sent_and_connection_state_is_OPEN_SENT)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	connection_dowork(connection);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, open_frame_encode(TEST_FRAME_CODEC_HANDLE, "1"));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_OPEN_SENT, connection_state);
}

/* Tests_SRS_CONNECTION_01_040: [HDR RCVD In this state the connection header has been received from the peer but a connection header has not been sent.] */
/* Tests_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.]  */
/* Tests_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
/* Tests_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
TEST_METHOD(when_protocol_header_is_received_before_it_is_sent_sends_the_protocol_header_and_open_frame)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header));
	STRICT_EXPECTED_CALL(mocks, open_frame_encode(TEST_FRAME_CODEC_HANDLE, "1"));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_OPEN_SENT, connection_state);
}

/* Tests_SRS_CONNECTION_01_040: [HDR RCVD In this state the connection header has been received from the peer but a connection header has not been sent.] */
/* Tests_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.]  */
/* Tests_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
/* Tests_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
TEST_METHOD(when_protocol_header_is_received_in_2_calls_before_it_is_sent_sends_the_protocol_header_and_open_frame)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, amqp_header, sizeof(amqp_header)))
		.ValidateArgumentBuffer(2, amqp_header, sizeof(amqp_header));
	STRICT_EXPECTED_CALL(mocks, open_frame_encode(TEST_FRAME_CODEC_HANDLE, "1"));

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header) - 1);
	io_receive_callback(io_receive_callback_context, amqp_header + sizeof(amqp_header) - 1, 1);

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_OPEN_SENT, connection_state);
}

/* Tests_SRS_CONNECTION_01_040: [HDR RCVD In this state the connection header has been received from the peer but a connection header has not been sent.] */
TEST_METHOD(when_only_one_byte_is_received_before_sending_protocol_header_state_does_not_change)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	mocks.ResetAllCalls();
	const unsigned char amqp_header[] = { 'A' };

	// act
	io_receive_callback(io_receive_callback_context, amqp_header, sizeof(amqp_header));

	// assert
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_START, connection_state);
}

/* Tests_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.]  */
TEST_METHOD(when_only_one_byte_is_received_and_do_work_is_called_state_is_set_to_HDR_SENT)
{
	// arrange
	connection_mocks mocks;
	CONNECTION_HANDLE connection = connection_create("testhost", 5672);
	const unsigned char recv_amqp_header[] = { 'A' };
	const unsigned char sent_amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	io_receive_callback(io_receive_callback_context, recv_amqp_header, sizeof(recv_amqp_header));
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, io_send(TEST_IO_HANDLE, sent_amqp_header, sizeof(sent_amqp_header)))
		.ValidateArgumentBuffer(2, sent_amqp_header, sizeof(sent_amqp_header));
	STRICT_EXPECTED_CALL(mocks, io_dowork(TEST_IO_HANDLE));

	// act
	int result = connection_dowork(connection);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	CONNECTION_STATE connection_state;
	(void)connection_get_state(connection, &connection_state);
	ASSERT_ARE_EQUAL(int, (int)CONNECTION_STATE_HDR_SENT, connection_state);
}

/* Tests_SRS_CONNECTION_01_006: [The open frame can only be sent on channel 0.] */

END_TEST_SUITE(amqpvalue_unittests)
