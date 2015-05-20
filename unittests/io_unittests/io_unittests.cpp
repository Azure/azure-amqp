#include "testrunnerswitcher.h"
#include "micromock.h"
#include "io.h"

#define TEST_CONCRETE_IO_HANDLE (CONCRETE_IO_HANDLE)0x4242;

TYPED_MOCK_CLASS(io_mocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_4(, CONCRETE_IO_HANDLE, test_io_create, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context, LOGGER_LOG, logger_log)
	MOCK_METHOD_END(CONCRETE_IO_HANDLE, TEST_CONCRETE_IO_HANDLE);
	MOCK_STATIC_METHOD_1(, void, test_io_destroy, CONCRETE_IO_HANDLE, handle)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, test_io_send, CONCRETE_IO_HANDLE, handle, const void*, buffer, size_t, size)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, int, test_io_dowork, CONCRETE_IO_HANDLE, handle)
	MOCK_METHOD_END(int, 0);
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void, amqpalloc_free, void*, ptr);
	DECLARE_GLOBAL_MOCK_METHOD_4(io_mocks, , CONCRETE_IO_HANDLE, test_io_create, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void, test_io_destroy, CONCRETE_IO_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(io_mocks, , int, test_io_send, CONCRETE_IO_HANDLE, handle, const void*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , int, test_io_dowork, CONCRETE_IO_HANDLE, handle);
}

const IO_INTERFACE_DESCRIPTION TestIODescription =
{
	test_io_create,
	test_io_destroy,
	test_io_send,
	test_io_dowork
};

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(io_unittests)

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

/* io_create */

/* Tests_SRS_IO_01_001: [io_create shall return on success a non-NULL handle to a new IO interface.] */
TEST_METHOD(io_create_with_all_args_except_interface_description_NULL_succeeds)
{
	// arrange
	io_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, test_io_create(NULL, NULL, NULL, NULL));

	// act
	IO_HANDLE result = io_create(&TestIODescription, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NOT_NULL(result);
}

END_TEST_SUITE(io_unittests)

