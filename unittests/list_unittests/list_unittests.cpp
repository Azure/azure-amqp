#include "TestRunnerSwitcher.h"
#include "list.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
