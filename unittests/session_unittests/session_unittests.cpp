#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "session.h"
#include "io.h"
#include "socketio.h"
#include "frame_codec.h"
#include "amqp_frame_codec.h"
#include "amqp_definitions.h"
#include "amqp_definitions_mocks.h"
#include "list.h"

#define TEST_ENDPOINT_HANDLE			(ENDPOINT_HANDLE)0x4242
#define TEST_DESCRIBED_AMQP_VALUE		(AMQP_VALUE)0x4247
#define TEST_LIST_ITEM_AMQP_VALUE		(AMQP_VALUE)0x4246
#define TEST_DESCRIPTOR_AMQP_VALUE		(AMQP_VALUE)0x4245
#define TEST_CONNECTION_HANDLE			(CONNECTION_HANDLE)0x4248
#define TEST_CONTEXT					(void*)0x4444
#define TEST_ATTACH_PERFORMATIVE		(AMQP_VALUE)0x5000
#define TEST_BEGIN_PERFORMATIVE			(AMQP_VALUE)0x5001

static ENDPOINT_FRAME_RECEIVED_CALLBACK saved_frame_received_callback;
static CONNECTION_STATE_CHANGED_CALLBACK saved_connection_state_changed_callback;
static void* saved_callback_context;
static uint32_t remote_max_frame_size = 512;

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

uint64_t performative_ulong;

TYPED_MOCK_CLASS(session_mocks, CGlobalMock)
{
public:
	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_2(, void*, amqpalloc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, realloc(ptr, size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* amqpvalue mocks */
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value)
		*ulong_value = performative_ulong;
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, amqpvalue_get_uint, AMQP_VALUE, value, uint32_t*, uint_value)
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

	/* connection mocks */
	MOCK_STATIC_METHOD_4(, ENDPOINT_HANDLE, connection_create_endpoint, CONNECTION_HANDLE, connection, ENDPOINT_FRAME_RECEIVED_CALLBACK, frame_received_callback, CONNECTION_STATE_CHANGED_CALLBACK, connection_state_changed_callback, void*, context)
		saved_frame_received_callback = frame_received_callback;
		saved_connection_state_changed_callback = connection_state_changed_callback;
		saved_callback_context = context;
	MOCK_METHOD_END(ENDPOINT_HANDLE, TEST_ENDPOINT_HANDLE);
	MOCK_STATIC_METHOD_1(, void, connection_destroy_endpoint, ENDPOINT_HANDLE, endpoint)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, int, connection_encode_frame, ENDPOINT_HANDLE, endpoint, const AMQP_VALUE, performative, PAYLOAD*, payloads, size_t, payload_count)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, connection_get_remote_max_frame_size, CONNECTION_HANDLE, connection, uint32_t*, remote_max_frame_size)
	MOCK_METHOD_END(int, 0);

	MOCK_STATIC_METHOD_4(, void, test_frame_received_callback, void*, context, AMQP_VALUE, performative, uint32_t, frame_payload_size, const unsigned char*, payload_bytes)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, void, test_on_session_state_changed, void*, context, SESSION_STATE, new_session_state, SESSION_STATE, previous_session_state)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , void*, amqpalloc_realloc, void*, ptr, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , int, amqpvalue_get_ulong, AMQP_VALUE, value, uint64_t*, ulong_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , int, amqpvalue_get_uint, AMQP_VALUE, value, uint32_t*, uint_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , AMQP_VALUE, amqpvalue_get_inplace_descriptor, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , int, amqpvalue_get_string, AMQP_VALUE, value, const char**, string_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , AMQP_VALUE, amqpvalue_get_list_item, AMQP_VALUE, value, size_t, index);
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , AMQP_VALUE, amqpvalue_get_described_value, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , void, amqpvalue_destroy, AMQP_VALUE, amqp_value)

	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , char*, amqpvalue_to_string, AMQP_VALUE, amqp_value)

	DECLARE_GLOBAL_MOCK_METHOD_4(session_mocks, , ENDPOINT_HANDLE, connection_create_endpoint, CONNECTION_HANDLE, connection, ENDPOINT_FRAME_RECEIVED_CALLBACK, frame_received_callback, CONNECTION_STATE_CHANGED_CALLBACK, connection_state_changed_callback, void*, context);
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , void, connection_destroy_endpoint, ENDPOINT_HANDLE, endpoint);
	DECLARE_GLOBAL_MOCK_METHOD_4(session_mocks, , int, connection_encode_frame, ENDPOINT_HANDLE, endpoint, const AMQP_VALUE, performative, PAYLOAD*, payloads, size_t, payload_count)
	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , int, connection_get_remote_max_frame_size, CONNECTION_HANDLE, connection, uint32_t*, remote_max_frame_size);

	DECLARE_GLOBAL_MOCK_METHOD_4(session_mocks, , void, test_frame_received_callback, void*, context, AMQP_VALUE, performative, uint32_t, frame_payload_size, const unsigned char*, payload_bytes);
	DECLARE_GLOBAL_MOCK_METHOD_3(session_mocks, , void, test_on_session_state_changed, void*, context, SESSION_STATE, new_session_state, SESSION_STATE, previous_session_state);

	extern void consolelogger_log(char* format, ...)
	{
		(void)format;
	}
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(session_unittests)

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
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
	if (!MicroMockReleaseMutex(test_serialize_mutex))
	{
		ASSERT_FAIL("Could not release test serialization mutex.");
	}
}

/* session_create */

/* Tests_SRS_SESSION_01_030: [session_create shall create a new session instance and return a non-NULL handle to it.] */
/* Tests_SRS_SESSION_01_032: [session_create shall create a new session endpoint by calling connection_create_endpoint.] */
TEST_METHOD(session_create_with_valid_args_succeeds)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, connection_create_endpoint(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);

	// act
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);

	// assert
	ASSERT_IS_NOT_NULL(session);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_030: [session_create shall create a new session instance and return a non-NULL handle to it.] */
/* Tests_SRS_SESSION_01_032: [session_create shall create a new session endpoint by calling connection_create_endpoint.] */
TEST_METHOD(session_create_twice_on_the_same_connection_works)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, connection_create_endpoint(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, connection_create_endpoint(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1);

	// act
	SESSION_HANDLE session1 = session_create(TEST_CONNECTION_HANDLE);
	SESSION_HANDLE session2 = session_create(TEST_CONNECTION_HANDLE);

	// assert
	ASSERT_IS_NOT_NULL(session1);
	ASSERT_IS_NOT_NULL(session2);
	ASSERT_ARE_NOT_EQUAL(void_ptr, session1, session2);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session1);
	session_destroy(session2);
}

/* Tests_SRS_SESSION_01_031: [If connection is NULL, session_create shall fail and return NULL.] */
TEST_METHOD(session_create_with_NULL_connection_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	// act
	SESSION_HANDLE session = session_create(NULL);

	// assert
	ASSERT_IS_NULL(session);
}

/* Tests_SRS_SESSION_01_042: [If allocating memory for the session fails, session_create shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_for_the_session_fails_session_create_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);

	// assert
	ASSERT_IS_NULL(session);
}

/* Tests_SRS_SESSION_01_033: [If connection_create_endpoint fails, session_create shall fail and return NULL.] */
TEST_METHOD(when_connection_create_endpoint_fails_session_create_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, connection_create_endpoint(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
		.ValidateArgument(1)
		.SetReturn((ENDPOINT_HANDLE)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);

	// assert
	ASSERT_IS_NULL(session);
}

/* session_destroy */

/* Tests_SRS_SESSION_01_034: [session_destroy shall free all resources allocated by session_create.] */
/* Tests_SRS_SESSION_01_035: [The endpoint created in session_create shall be freed by calling connection_destroy_endpoint.] */
TEST_METHOD(when_session_destroy_is_called_then_the_underlying_endpoint_is_freed)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, connection_destroy_endpoint(TEST_ENDPOINT_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	session_destroy(session);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_SESSION_01_036: [If session is NULL, session_destroy shall do nothing.] */
TEST_METHOD(session_destroy_with_NULL_session_does_nothing)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	// act
	session_destroy(NULL);

	// assert
	// uMock checks the calls
}

/* session_create_link_endpoint */

/* Tests_SRS_SESSION_01_043: [session_create_link_endpoint shall create a link endpoint associated with a given session and return a non-NULL handle to it.] */
/* Tests_SRS_SESSION_01_046: [An unused handle shall be assigned to the link endpoint.] */
TEST_METHOD(session_create_link_endpoint_creates_a_link_endpoint)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);

	// assert
	ASSERT_IS_NOT_NULL(link_endpoint);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_044: [If session, name or frame_received_callback is NULL, session_create_link_endpoint shall fail and return NULL.] */
TEST_METHOD(session_create_with_NULL_session_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(NULL, "1", test_frame_received_callback, test_on_session_state_changed,TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(link_endpoint);
}

/* Tests_SRS_SESSION_01_044: [If session, name or frame_received_callback is NULL, session_create_link_endpoint shall fail and return NULL.] */
TEST_METHOD(session_create_with_NULL_name_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, NULL, test_frame_received_callback, test_on_session_state_changed,TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(link_endpoint);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_044: [If session, name or frame_received_callback is NULL, session_create_link_endpoint shall fail and return NULL.] */
TEST_METHOD(session_create_with_NULL_frame_received_callback_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", NULL, test_on_session_state_changed, TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(link_endpoint);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_045: [If allocating memory for the link endpoint fails, session_create_link_endpoint shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_for_the_link_endpoint_fails_then_session_create_link_endpoint_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed,TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(link_endpoint);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_045: [If allocating memory for the link endpoint fails, session_create_link_endpoint shall fail and return NULL.] */
TEST_METHOD(when_allocating_the_link_name_fails_then_session_create_link_endpoint_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed,TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(link_endpoint);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_045: [If allocating memory for the link endpoint fails, session_create_link_endpoint shall fail and return NULL.] */
TEST_METHOD(when_reallocating_the_endpoint_array_for_the_link_endpoint_fails_then_session_create_link_endpoint_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed,TEST_CONTEXT);

	// assert
	ASSERT_IS_NULL(link_endpoint);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* session_destroy_link_endpoint */

/* Tests_SRS_SESSION_01_050: [If endpoint is NULL, session_destroy_link_endpoint shall do nothing.] */
TEST_METHOD(session_destroy_link_endpoint_with_NULL_handle_does_nothing)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	// act
	session_destroy_link_endpoint(NULL);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_SESSION_01_049: [session_destroy_link_endpoint shall free all resources associated with the endpoint.] */
TEST_METHOD(session_destroy_link_endpoint_frees_the_resources)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	session_destroy_link_endpoint(link_endpoint);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_049: [session_destroy_link_endpoint shall free all resources associated with the endpoint.] */
TEST_METHOD(session_destroy_link_endpoint_when_2_endpoints_are_there_frees_the_resources)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint1 = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	LINK_ENDPOINT_HANDLE link_endpoint2 = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

	// act
	session_destroy_link_endpoint(link_endpoint1);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint2);
	session_destroy(session);
}

/* session_encode_frame */

/* Tests_SRS_SESSION_01_037: [session_encode_frame shall encode an AMQP frame.] */
/* Tests_SRS_SESSION_01_038: [On success it shall return 0.] */
/* Tests_SRS_SESSION_01_039: [The encoding shall be done by passing the performative and the payloads to the connection_encode_frame function.] */
TEST_METHOD(session_encode_frame_encodes_the_frame_by_giving_it_to_the_connection)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, TEST_ATTACH_PERFORMATIVE, NULL, 0));

	// act
	int result = session_encode_frame(link_endpoint, TEST_ATTACH_PERFORMATIVE, NULL, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_037: [session_encode_frame shall encode an AMQP frame.] */
/* Tests_SRS_SESSION_01_038: [On success it shall return 0.] */
/* Tests_SRS_SESSION_01_039: [The encoding shall be done by passing the performative and the payloads to the connection_encode_frame function.] */
TEST_METHOD(session_encode_frame_passes_the_paylaod_to_the_connection_encode_frame_function)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	unsigned char payload_bytes[] = { 0x42 };
	PAYLOAD payload = { payload_bytes, sizeof(payload_bytes) };

	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, TEST_ATTACH_PERFORMATIVE, &payload, 1));

	// act
	int result = session_encode_frame(link_endpoint, TEST_ATTACH_PERFORMATIVE, &payload, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_040: [If link_endpoint or performative is NULL, session_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(session_encode_frame_with_NULL_performative_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	unsigned char payload_bytes[] = { 0x42 };
	PAYLOAD payload = { payload_bytes, sizeof(payload_bytes) };

	// act
	int result = session_encode_frame(link_endpoint, NULL, &payload, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_040: [If link_endpoint or performative is NULL, session_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(session_encode_frame_with_NULL_link_endpoint_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	unsigned char payload_bytes[] = { 0x42 };
	PAYLOAD payload = { payload_bytes, sizeof(payload_bytes) };

	// act
	int result = session_encode_frame(NULL, TEST_ATTACH_PERFORMATIVE, &payload, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SESSION_01_041: [If connection_encode_frame fails, session_encode_frame shall fail and return a non-zero value.] */
TEST_METHOD(when_connection_encode_frame_then_session_encode_frame_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	unsigned char payload_bytes[] = { 0x42 };
	PAYLOAD payload = { payload_bytes, sizeof(payload_bytes) };

	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, TEST_ATTACH_PERFORMATIVE, &payload, 1))
		.SetReturn(1);

	// act
	int result = session_encode_frame(link_endpoint, TEST_ATTACH_PERFORMATIVE, &payload, 1);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* session_transfer */

/* Tests_SRS_SESSION_01_051: [session_transfer shall send a transfer frame with the performative indicated in the transfer argument.] */
/* Tests_SRS_SESSION_01_053: [On success, session_transfer shall return 0.] */
/* Tests_SRS_SESSION_01_055: [The encoding of the frame shall be done by calling connection_encode_frame and passing as arguments: the connection handle associated with the session, the transfer performative and the payload chunks passed to session_transfer.] */
/* Tests_SRS_SESSION_01_057: [The delivery ids shall be assigned starting at 0.] */
TEST_METHOD(session_transfer_sends_the_frame_to_the_connection)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_transfer_amqp_value, NULL, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_transfer_amqp_value));

	// act
	delivery_number delivery_id;
	int result = session_transfer(link_endpoint, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_054: [If link_endpoint or transfer is NULL, session_transfer shall fail and return a non-zero value.] */
TEST_METHOD(session_transfer_with_NULL_transfer_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	// act
	delivery_number delivery_id;
	int result = session_transfer(link_endpoint, NULL, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_054: [If link_endpoint or transfer is NULL, session_transfer shall fail and return a non-zero value.] */
TEST_METHOD(session_transfer_with_NULL_link_endpoint_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	// act
	delivery_number delivery_id;
	int result = session_transfer(NULL, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SESSION_01_058: [When any other error occurs, session_transfer shall fail and return a non-zero value.] */
TEST_METHOD(when_transfer_set_delivery_id_fails_then_session_transfer_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0))
		.SetReturn(1);

	// act
	delivery_number delivery_id;
	int result = session_transfer(link_endpoint, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_058: [When any other error occurs, session_transfer shall fail and return a non-zero value.] */
TEST_METHOD(when_amqpvalue_create_transfer_fails_then_session_transfer_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle))
		.SetReturn((AMQP_VALUE)NULL);

	// act
	delivery_number delivery_id;
	int result = session_transfer(link_endpoint, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_056: [If connection_encode_frame fails then session_transfer shall fail and return a non-zero value.] */
TEST_METHOD(when_connection_encode_frame_fails_then_session_transfer_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_transfer_amqp_value, NULL, 0))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_transfer_amqp_value));

	// act
	delivery_number delivery_id;
	int result = session_transfer(link_endpoint, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_059: [When session_transfer is called while the session is not in the MAPPED state, session_transfer shall fail and return a non-zero value.] */
TEST_METHOD(when_session_is_not_MAPPED_the_transfer_fails)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	// act
	delivery_number delivery_id;
	int result = session_transfer(link_endpoint, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* connection_state_changed_callback */

/* Tests_SRS_SESSION_01_060: [If the previous connection state is not OPENED and the new connection state is OPENED, the BEGIN frame shall be sent out and the state shall be switched to BEGIN_SENT.] */
TEST_METHOD(connection_state_changed_callback_with_OPENED_triggers_sending_the_BEGIN_frame)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, begin_create(0, 1, 1));
	STRICT_EXPECTED_CALL(definition_mocks, begin_set_handle_max(test_begin_handle, 4294967295));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_begin(test_begin_handle));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_begin_amqp_value, NULL, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_begin_amqp_value));
	STRICT_EXPECTED_CALL(definition_mocks, begin_destroy(test_begin_handle));

	STRICT_EXPECTED_CALL(mocks, test_on_session_state_changed(NULL, SESSION_STATE_BEGIN_SENT, SESSION_STATE_UNMAPPED));

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_060: [If the previous connection state is not OPENED and the new connection state is OPENED, the BEGIN frame shall be sent out and the state shall be switched to BEGIN_SENT.] */
TEST_METHOD(connection_state_changed_callback_and_new_state_is_not_OPENED_does_not_trigger_sending_the_BEGIN_frame)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPEN_SENT, CONNECTION_STATE_START);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_060: [If the previous connection state is not OPENED and the new connection state is OPENED, the BEGIN frame shall be sent out and the state shall be switched to BEGIN_SENT.] */
TEST_METHOD(connection_state_changed_callback_and_from_OPENED_to_OPENED_does_not_trigger_sending_the_BEGIN_frame)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPENED);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_060: [If the previous connection state is not OPENED and the new connection state is OPENED, the BEGIN frame shall be sent out and the state shall be switched to BEGIN_SENT.] */
TEST_METHOD(connection_state_changed_callback_to_OPENED_twice_only_triggers_sending_the_BEGIN_frame_once)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_-SRS_SESSION_01_061: [If the previous connection state is OPENED and the new connection state is not OPENED anymore, the state shall be switched to DISCARDING.] */
TEST_METHOD(connection_state_changed_callback_to_different_than_OPENED_when_in_UNMAPPED_sets_the_session_state_to_END)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_on_session_state_changed(NULL, SESSION_STATE_DISCARDING, SESSION_STATE_UNMAPPED));

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_CLOSE_RCVD, CONNECTION_STATE_OPENED);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_-SRS_SESSION_01_061: [If the previous connection state is OPENED and the new connection state is not OPENED anymore, the state shall be switched to DISCARDING.] */
TEST_METHOD(connection_state_changed_callback_to_different_than_OPENED_when_in_BEGIN_SENT_sets_the_session_state_to_END)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_on_session_state_changed(NULL, SESSION_STATE_DISCARDING, SESSION_STATE_BEGIN_SENT));

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_CLOSE_RCVD, CONNECTION_STATE_OPENED);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Tests_-SRS_SESSION_01_061: [If the previous connection state is OPENED and the new connection state is not OPENED anymore, the state shall be switched to DISCARDING.] */
TEST_METHOD(connection_state_changed_callback_to_different_than_OPENED_when_in_MAPPED_sets_the_session_state_to_END)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_on_session_state_changed(NULL, SESSION_STATE_DISCARDING, SESSION_STATE_MAPPED));

	// act
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_CLOSE_RCVD, CONNECTION_STATE_OPENED);

	// assert
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint);
	session_destroy(session);
}

/* Session flow control */

/* Tests_SRS_SESSION_01_012: [The session endpoint assigns each outgoing transfer frame an implicit transfer-id from a session scoped sequence.] */
/* Tests_SRS_SESSION_01_027: [sending a transfer Upon sending a transfer, the sending endpoint will increment its next-outgoing-id] */
/* Tests_SRS_SESSION_01_016: [next-outgoing-id The next-outgoing-id is the transfer-id to assign to the next transfer frame.] */
/* Tests_SRS_SESSION_01_017: [The nextoutgoing-id MAY be initialized to an arbitrary value ] */
/* Tests_SRS_SESSION_01_018: [is incremented after each successive transfer according to RFC-1982 [RFC1982] serial number arithmetic.] */
TEST_METHOD(when_2_transfers_happen_on_2_different_endpoints_2_different_delivery_ids_are_assigned)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint0 = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	LINK_ENDPOINT_HANDLE link_endpoint1 = session_create_link_endpoint(session, "2", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_transfer_amqp_value, NULL, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_transfer_amqp_value));

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 1));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_transfer_amqp_value, NULL, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_transfer_amqp_value));

	// act
	delivery_number delivery_id0;
	(void)session_transfer(link_endpoint0, test_transfer_handle, NULL, 0, &delivery_id0);
	delivery_number delivery_id1;
	(void)session_transfer(link_endpoint0, test_transfer_handle, NULL, 0, &delivery_id1);

	// assert
	ASSERT_ARE_EQUAL(delivery_number, 0, delivery_id0);
	ASSERT_ARE_EQUAL(delivery_number, 1, delivery_id1);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint0);
	session_destroy_link_endpoint(link_endpoint1);
	session_destroy(session);
}

/* Tests_SRS_SESSION_01_018: [is incremented after each successive transfer according to RFC-1982 [RFC1982] serial number arithmetic.] */
TEST_METHOD(when_if_sending_the_frame_to_the_connection_fails_the_next_outgoing_id_is_not_incremented)
{
	// arrange
	session_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	SESSION_HANDLE session = session_create(TEST_CONNECTION_HANDLE);
	LINK_ENDPOINT_HANDLE link_endpoint0 = session_create_link_endpoint(session, "1", test_frame_received_callback, test_on_session_state_changed, NULL);
	LINK_ENDPOINT_HANDLE link_endpoint1 = session_create_link_endpoint(session, "2", test_frame_received_callback, test_on_session_state_changed, NULL);
	saved_connection_state_changed_callback(saved_callback_context, CONNECTION_STATE_OPENED, CONNECTION_STATE_OPEN_SENT);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_get_inplace_descriptor(TEST_BEGIN_PERFORMATIVE));
	STRICT_EXPECTED_CALL(definition_mocks, is_begin_type_by_descriptor(TEST_DESCRIPTOR_AMQP_VALUE));
	saved_frame_received_callback(saved_callback_context, TEST_BEGIN_PERFORMATIVE, NULL, 0);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpvalue_to_string(IGNORED_PTR_ARG)).IgnoreAllCalls();

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_transfer_amqp_value, NULL, 0))
		.SetReturn(1);
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_transfer_amqp_value));

	STRICT_EXPECTED_CALL(definition_mocks, transfer_set_delivery_id(test_transfer_handle, 0));
	STRICT_EXPECTED_CALL(mocks, connection_get_remote_max_frame_size(TEST_CONNECTION_HANDLE, IGNORED_PTR_ARG))
		.CopyOutArgumentBuffer(2, &remote_max_frame_size, sizeof(remote_max_frame_size));
	STRICT_EXPECTED_CALL(definition_mocks, amqpvalue_create_transfer(test_transfer_handle));
	STRICT_EXPECTED_CALL(mocks, connection_encode_frame(TEST_ENDPOINT_HANDLE, test_transfer_amqp_value, NULL, 0));
	STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(test_transfer_amqp_value));

	// act
	delivery_number delivery_id;
	(void)session_transfer(link_endpoint0, test_transfer_handle, NULL, 0, &delivery_id);
	(void)session_transfer(link_endpoint0, test_transfer_handle, NULL, 0, &delivery_id);

	// assert
	ASSERT_ARE_EQUAL(delivery_number, 0, delivery_id);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	session_destroy_link_endpoint(link_endpoint0);
	session_destroy_link_endpoint(link_endpoint1);
	session_destroy(session);
}

END_TEST_SUITE(connection_unittests)
