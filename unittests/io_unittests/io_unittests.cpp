#include "testrunnerswitcher.h"
#include "micromock.h"
#include "io.h"

#define TEST_CONCRETE_IO_HANDLE (CONCRETE_IO_HANDLE)0x4242

TYPED_MOCK_CLASS(io_mocks, CGlobalMock)
{
public:
	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* io interface mocks */
	MOCK_STATIC_METHOD_4(, CONCRETE_IO_HANDLE, test_io_create, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context, LOGGER_LOG, logger_log)
	MOCK_METHOD_END(CONCRETE_IO_HANDLE, TEST_CONCRETE_IO_HANDLE);
	MOCK_STATIC_METHOD_1(, void, test_io_destroy, CONCRETE_IO_HANDLE, handle)
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_3(, int, test_io_send, CONCRETE_IO_HANDLE, handle, const void*, buffer, size_t, size)
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_1(, void, test_io_dowork, CONCRETE_IO_HANDLE, handle)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_4(io_mocks, , CONCRETE_IO_HANDLE, test_io_create, void*, io_create_parameters, IO_RECEIVE_CALLBACK, receive_callback, void*, receive_callback_context, LOGGER_LOG, logger_log);
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void, test_io_destroy, CONCRETE_IO_HANDLE, handle);
	DECLARE_GLOBAL_MOCK_METHOD_3(io_mocks, , int, test_io_send, CONCRETE_IO_HANDLE, handle, const void*, buffer, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(io_mocks, , void, test_io_dowork, CONCRETE_IO_HANDLE, handle);

	int test_receive_callback(void* context, const void* buffer, size_t size)
	{
		(void)context;
		(void)buffer;
		(void)size;

		return 0;
	}

	void test_logger_log(char* format, ...)
	{
		(void)format;
	}
}

const IO_INTERFACE_DESCRIPTION test_io_description =
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
/* Tests_SRS_IO_01_002: [In order to instantiate the concrete IO implementation the function concrete_io_create from the io_interface_description shall be called, passing the io_create_parameters, receive_callback, receive_callback_context and logger_log arguments.] */
TEST_METHOD(io_create_with_all_args_except_interface_description_NULL_succeeds)
{
	// arrange
	io_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, test_io_create(NULL, NULL, NULL, NULL));

	// act
	IO_HANDLE result = io_create(&test_io_description, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NOT_NULL(result);
}

/* Tests_SRS_IO_01_002: [In order to instantiate the concrete IO implementation the function concrete_io_create from the io_interface_description shall be called, passing the io_create_parameters, receive_callback, receive_callback_context and logger_log arguments.] */
TEST_METHOD(io_create_passes_the_args_to_the_concrete_io_implementation)
{
	// arrange
	io_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, test_io_create((void*)0x4243, test_receive_callback, (void*)0x4245, test_logger_log));

	// act
	IO_HANDLE result = io_create(&test_io_description, (void*)0x4243, test_receive_callback, (void*)0x4245, test_logger_log);

	// assert
	ASSERT_IS_NOT_NULL(result);
}

/* Tests_SRS_IO_01_016: [If the underlying concrete_io_create call fails, io_create shall return NULL.] */
TEST_METHOD(when_concrete_io_create_fails_then_io_create_fails)
{
	// arrange
	io_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE));
	STRICT_EXPECTED_CALL(mocks, test_io_create(NULL, NULL, NULL, NULL))
		.SetReturn((CONCRETE_IO_HANDLE)NULL);

	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	IO_HANDLE result = io_create(&test_io_description, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_IO_01_003: [If the argument io_interface_description is NULL, io_create shall return NULL.] */
TEST_METHOD(when_io_interface_description_is_NULL_then_io_create_fails)
{
	// arrange
	io_mocks mocks;

	// act
	IO_HANDLE result = io_create(NULL, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_IO_01_004: [If any io_interface_description member is NULL, io_create shall return NULL.] */
TEST_METHOD(when_concrete_io_create_is_NULL_then_io_create_fails)
{
	// arrange
	io_mocks mocks;
	const IO_INTERFACE_DESCRIPTION io_description_null =
	{
		NULL,
		test_io_destroy,
		test_io_send,
		test_io_dowork
	};

	// act
	IO_HANDLE result = io_create(&io_description_null, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_IO_01_004: [If any io_interface_description member is NULL, io_create shall return NULL.] */
TEST_METHOD(when_concrete_io_destroy_is_NULL_then_io_create_fails)
{
	// arrange
	io_mocks mocks;
	const IO_INTERFACE_DESCRIPTION io_description_null =
	{
		test_io_create,
		NULL,
		test_io_send,
		test_io_dowork
	};

	// act
	IO_HANDLE result = io_create(&io_description_null, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_IO_01_004: [If any io_interface_description member is NULL, io_create shall return NULL.] */
TEST_METHOD(when_concrete_io_send_is_NULL_then_io_create_fails)
{
	// arrange
	io_mocks mocks;
	const IO_INTERFACE_DESCRIPTION io_description_null =
	{
		test_io_create,
		test_io_destroy,
		NULL,
		test_io_dowork
	};

	// act
	IO_HANDLE result = io_create(&io_description_null, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_IO_01_004: [If any io_interface_description member is NULL, io_create shall return NULL.] */
TEST_METHOD(when_concrete_io_dowork_is_NULL_then_io_create_fails)
{
	// arrange
	io_mocks mocks;
	const IO_INTERFACE_DESCRIPTION io_description_null =
	{
		test_io_create,
		test_io_destroy,
		test_io_send,
		NULL
	};

	// act
	IO_HANDLE result = io_create(&io_description_null, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* Tests_SRS_IO_01_017: [If allocating the memory needed for the IO interface fails then io_create shall return NULL.] */
TEST_METHOD(when_allocating_memory_Fails_then_io_create_fails)
{
	// arrange
	io_mocks mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORE))
		.SetReturn((void*)NULL);

	// act
	IO_HANDLE result = io_create(&test_io_description, NULL, NULL, NULL, NULL);

	// assert
	ASSERT_IS_NULL(result);
}

/* io_destroy */

/* Tests_SRS_IO_01_005: [io_destroy shall free all resources associated with the IO handle.] */
/* Tests_SRS_IO_01_006: [io_destroy shall also call the concrete_io_destroy function that is member of the io_interface_description argument passed to io_create, while passing as argument to concrete_io_destroy the result of the underlying concrete_io_create handle that was called as part of the io_create call.] */
TEST_METHOD(io_destroy_calls_concrete_io_destroy_and_frees_memory)
{
	// arrange
	io_mocks mocks;
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_destroy(TEST_CONCRETE_IO_HANDLE));
	EXPECTED_CALL(mocks, amqpalloc_free(IGNORED_PTR_ARG));

	// act
	io_destroy(handle);

	// assert
}

/* Tests_SRS_IO_01_007: [If handle is NULL, io_destroy shall do nothing.] */
TEST_METHOD(io_destroy_with_null_handle_does_nothing)
{
	// arrange
	io_mocks mocks;

	// act
	io_destroy(NULL);

	// assert
}

/* io_send */

/* Tests_SRS_IO_01_008: [io_send shall pass the sequence of bytes pointed to by buffer to the concrete IO implementation specified in io_create, by calling the concrete_io_send function while passing down the buffer and size arguments to it.] */
/* Tests_SRS_IO_01_009: [On success, io_send shall return 0.] */
TEST_METHOD(io_send_calls_the_underlying_concrete_io_send_an_succeeds)
{
	// arrange
	io_mocks mocks;
	unsigned char send_data[] = { 0x42, 43 };
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_send(TEST_CONCRETE_IO_HANDLE, send_data, sizeof(send_data)));

	// act
	int result = io_send(handle, send_data, sizeof(send_data));

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_IO_01_010: [If handle is NULL, io_send shall return a non-zero value.] */
TEST_METHOD(io_send_with_NULL_handle_fails)
{
	// arrange
	io_mocks mocks;
	unsigned char send_data[] = { 0x42, 43 };
	mocks.ResetAllCalls();

	// act
	int result = io_send(NULL, send_data, sizeof(send_data));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_IO_01_015: [If the underlying concrete_io_send fails, io_send shall return a non-zero value.] */
TEST_METHOD(when_the_concrete_io_send_fails_then_io_send_fails)
{
	// arrange
	io_mocks mocks;
	unsigned char send_data[] = { 0x42, 43 };
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_send(TEST_CONCRETE_IO_HANDLE, send_data, sizeof(send_data)))
		.SetReturn(42);

	// act
	int result = io_send(handle, send_data, sizeof(send_data));

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_IO_01_011: [No error check shall be performed on buffer and size.] */
TEST_METHOD(io_send_with_NULL_buffer_and_nonzero_length_passes_the_args_down_and_succeeds)
{
	// arrange
	io_mocks mocks;
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_send(TEST_CONCRETE_IO_HANDLE, NULL, 1));

	// act
	int result = io_send(handle, NULL, 1);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_IO_01_011: [No error check shall be performed on buffer and size.] */
TEST_METHOD(io_send_with_NULL_buffer_and_zero_length_passes_the_args_down_and_succeeds)
{
	// arrange
	io_mocks mocks;
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_send(TEST_CONCRETE_IO_HANDLE, NULL, 0));

	// act
	int result = io_send(handle, NULL, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_IO_01_011: [No error check shall be performed on buffer and size.] */
TEST_METHOD(io_send_with_non_NULL_buffer_and_zero_length_passes_the_args_down_and_succeeds)
{
	// arrange
	io_mocks mocks;
	unsigned char send_data[] = { 0x42, 43 };
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_send(TEST_CONCRETE_IO_HANDLE, send_data, 0));

	// act
	int result = io_send(handle, send_data, 0);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
}

/* io_dowork */

/* Tests_SRS_IO_01_012: [io_dowork shall call the concrete IO implementation specified in io_create, by calling the concrete_io_dowork function.] */
TEST_METHOD(io_dowork_calls_the_concrete_dowork_and_succeeds)
{
	// arrange
	io_mocks mocks;
	unsigned char send_data[] = { 0x42, 43 };
	IO_HANDLE handle = io_create(&test_io_description, NULL, NULL, NULL, NULL);
	mocks.ResetAllCalls();

	STRICT_EXPECTED_CALL(mocks, test_io_dowork(TEST_CONCRETE_IO_HANDLE));

	// act
	io_dowork(handle);

	// assert
	// uMock checks the calls
}

/* Tests_SRS_IO_01_018: [When the handle argument is NULL, io_dowork shall do nothing.] */
TEST_METHOD(io_dowork_with_NULL_handle_does_nothing)
{
	// arrange
	io_mocks mocks;

	// act
	io_dowork(NULL);

	// assert
	// uMock checks the calls
}

END_TEST_SUITE(io_unittests)
