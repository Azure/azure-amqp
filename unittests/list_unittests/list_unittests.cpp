#include "TestRunnerSwitcher.h"
#include "MicroMock.h"
#include "list.h"

TYPED_MOCK_CLASS(amqp_alloc_mocks, CGlobalMock)
{
public:
	MOCK_STATIC_METHOD_1(, void*, amqp_malloc, size_t, size)
	MOCK_METHOD_END(void*, malloc(size));
};

DECLARE_GLOBAL_MOCK_METHOD_1(amqp_alloc_mocks, , void*, amqp_malloc, size_t, size);

MICROMOCK_MUTEX_HANDLE test_serialize_mutex;

namespace amqpvalue_unittests
{
	TEST_CLASS(list_unittests)
	{
	public:
		TEST_METHOD(when_underlying_calls_suceed_list_create_succeeds)
		{
			// arrange

			// act
			LIST_HANDLE result = list_create();

			// assert
			ASSERT_IS_NOT_NULL(result);
		}
	};
}
