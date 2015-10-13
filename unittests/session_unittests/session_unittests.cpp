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
	MOCK_STATIC_METHOD_3(, ENDPOINT_HANDLE, connection_create_endpoint, CONNECTION_HANDLE, connection, ENDPOINT_FRAME_RECEIVED_CALLBACK, frame_received_callback, void*, context)
	MOCK_METHOD_END(ENDPOINT_HANDLE, TEST_ENDPOINT_HANDLE);
	MOCK_STATIC_METHOD_1(, void, connection_destroy_endpoint, ENDPOINT_HANDLE, endpoint)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, int, connection_encode_frame, ENDPOINT_HANDLE, endpoint, const AMQP_VALUE, performative, PAYLOAD*, payloads, size_t, payload_count)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, connection_get_state, CONNECTION_HANDLE, connection, CONNECTION_STATE*, connection_state)
	MOCK_METHOD_END(int, 0);
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

	DECLARE_GLOBAL_MOCK_METHOD_3(session_mocks, , ENDPOINT_HANDLE, connection_create_endpoint, CONNECTION_HANDLE, connection, ENDPOINT_FRAME_RECEIVED_CALLBACK, frame_received_callback, void*, context);
	DECLARE_GLOBAL_MOCK_METHOD_1(session_mocks, , void, connection_destroy_endpoint, ENDPOINT_HANDLE, endpoint);
	DECLARE_GLOBAL_MOCK_METHOD_4(session_mocks, , int, connection_encode_frame, ENDPOINT_HANDLE, endpoint, const AMQP_VALUE, performative, PAYLOAD*, payloads, size_t, payload_count)
	DECLARE_GLOBAL_MOCK_METHOD_2(session_mocks, , int, connection_get_state, CONNECTION_HANDLE, connection, CONNECTION_STATE*, connection_state);

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

END_TEST_SUITE(connection_unittests)
