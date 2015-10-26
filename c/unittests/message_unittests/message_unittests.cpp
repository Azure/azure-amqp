#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "message.h"
#include "io.h"
#include "socketio.h"
#include "frame_codec.h"
#include "amqp_frame_codec.h"
#include "amqp_definitions.h"
#include "amqp_definitions_mocks.h"

static const HEADER_HANDLE custom_message_header = (HEADER_HANDLE)0x4242;
static const AMQP_VALUE custom_delivery_annotations = (HEADER_HANDLE)0x4243;
static const AMQP_VALUE custom_message_annotations = (HEADER_HANDLE)0x4244;
static const AMQP_VALUE cloned_delivery_annotations = (HEADER_HANDLE)0x4245;
static const AMQP_VALUE cloned_message_annotations = (HEADER_HANDLE)0x4246;
static const AMQP_VALUE test_cloned_amqp_value = (AMQP_VALUE)0x4300;

TYPED_MOCK_CLASS(message_mocks, CGlobalMock)
{
public:
	/* amqpalloc mocks */
	MOCK_STATIC_METHOD_1(, void*, amqpalloc_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
	MOCK_STATIC_METHOD_1(, void, amqpalloc_free, void*, ptr)
		free(ptr);
	MOCK_VOID_METHOD_END();

	/* amqpvalue mocks */
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_clone, AMQP_VALUE, value)
	MOCK_METHOD_END(AMQP_VALUE, test_cloned_amqp_value);
	MOCK_STATIC_METHOD_1(, void, amqpvalue_destroy, AMQP_VALUE, value)
	MOCK_VOID_METHOD_END();
};

extern "C"
{
	DECLARE_GLOBAL_MOCK_METHOD_1(message_mocks, , void*, amqpalloc_malloc, size_t, size);
	DECLARE_GLOBAL_MOCK_METHOD_1(message_mocks, , void, amqpalloc_free, void*, ptr);

	DECLARE_GLOBAL_MOCK_METHOD_1(message_mocks, , AMQP_VALUE, amqpvalue_clone, AMQP_VALUE, value);
	DECLARE_GLOBAL_MOCK_METHOD_1(message_mocks, , void, amqpvalue_destroy, AMQP_VALUE, value);
}

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(message_unittests)

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

/* message_create */

/* Tests_SRS_MESSAGE_01_001: [message_create shall create a new AMQP message instance and on success it shall return a non-NULL handle for the newly created message instance.] */
TEST_METHOD(message_create_succeeds)
{
	// arrange
	message_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	// act
	MESSAGE_HANDLE message = message_create();

	// assert
	ASSERT_IS_NOT_NULL(message);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_001: [message_create shall create a new AMQP message instance and on success it shall return a non-NULL handle for the newly created message instance.] */
TEST_METHOD(message_create_2_times_yields_2_different_message_instances)
{
	// arrange
	message_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));

	// act
	MESSAGE_HANDLE message1 = message_create();
	MESSAGE_HANDLE message2 = message_create();

	// assert
	ASSERT_IS_NOT_NULL(message1);
	ASSERT_IS_NOT_NULL(message2);
	ASSERT_ARE_NOT_EQUAL(void_ptr, message1, message2);
	mocks.AssertActualAndExpectedCalls();

	// cleanup
	message_destroy(message1);
	message_destroy(message2);
}

/* Tests_SRS_MESSAGE_01_002: [If allocating memory for the message fails, message_create shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_for_the_message_fails_then_message_create_fails)
{
	// arrange
	message_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	MESSAGE_HANDLE message = message_create();

	// assert
	ASSERT_IS_NULL(message);
}

/* message_clone */

/* Tests_SRS_MESSAGE_01_003: [message_clone shall clone a message entirely and on success return a non-NULL handle to the cloned message.] */
/* Tests_SRS_MESSAGE_01_005: [If a header exists on the source message it shall be cloned by using header_clone.] */
/* Tests_SRS_MESSAGE_01_006: [If delivery annotations exist on the source message they shall be cloned by using annotations_clone.] */
/* Tests_SRS_MESSAGE_01_007: [If message annotations exist on the source message they shall be cloned by using annotations_clone.] */
TEST_METHOD(message_clone_with_a_valid_argument_succeeds)
{
	// arrange
	message_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	MESSAGE_HANDLE source_message = message_create();

	(void)message_set_header(source_message, custom_message_header);
	STRICT_EXPECTED_CALL(mocks, annotations_clone(custom_delivery_annotations))
		.SetReturn(cloned_delivery_annotations);
	(void)message_set_delivery_annotations(source_message, custom_delivery_annotations);
	STRICT_EXPECTED_CALL(mocks, annotations_clone(custom_message_annotations))
		.SetReturn(cloned_message_annotations);
	(void)message_set_message_annotations(source_message, custom_message_annotations);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(definition_mocks, header_clone(test_header_handle));
	STRICT_EXPECTED_CALL(mocks, annotations_clone(cloned_delivery_annotations));
	STRICT_EXPECTED_CALL(mocks, annotations_clone(cloned_message_annotations));

	// act
	MESSAGE_HANDLE message = message_clone(source_message);

	// assert
	ASSERT_IS_NOT_NULL(message);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	message_destroy(source_message);
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_062: [If source_message is NULL, message_clone shall fail and return NULL.] */
TEST_METHOD(message_clone_with_NULL_message_source_fails)
{
	// arrange
	message_mocks mocks;
	amqp_definitions_mocks definition_mocks;

	// act
	MESSAGE_HANDLE message = message_clone(NULL);

	// assert
	ASSERT_IS_NULL(message);
}

/* Tests_SRS_MESSAGE_01_004: [If allocating memory for the new cloned message fails, message_clone shall fail and return NULL.] */
TEST_METHOD(when_allocating_memory_fails_then_message_clone_fails)
{
	// arrange
	message_mocks mocks;
	amqp_definitions_mocks definition_mocks;
	MESSAGE_HANDLE source_message = message_create();
	(void)message_set_header(source_message, custom_message_header);
	mocks.ResetAllCalls();
	definition_mocks.ResetAllCalls();

	EXPECTED_CALL(mocks, amqpalloc_malloc(IGNORED_NUM_ARG))
		.SetReturn((void*)NULL);

	// act
	MESSAGE_HANDLE message = message_clone(source_message);

	// assert
	ASSERT_IS_NULL(message);
	mocks.AssertActualAndExpectedCalls();
	definition_mocks.AssertActualAndExpectedCalls();

	// cleanup
	message_destroy(source_message);
}

END_TEST_SUITE(message_unittests)
