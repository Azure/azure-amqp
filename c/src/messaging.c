#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "amqpvalue.h"
#include "amqp_definitions.h"

AMQP_VALUE messaging_create_source(AMQP_VALUE address)
{
	AMQP_VALUE result;
	SOURCE_HANDLE source = source_create();
	if (source == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE address_value = amqpvalue_create_string(address);
		if (address_value == NULL)
		{
			result = NULL;
		}
		else
		{
			if (source_set_address(source, amqpvalue_create_string(address)) != 0)
			{
				result = NULL;
			}
			else
			{
				result = amqpvalue_create_source(source);
			}

			amqpvalue_destroy(address_value);
		}

		source_destroy(source);
	}

	return result;
}

AMQP_VALUE messaging_create_target(AMQP_VALUE address)
{
	AMQP_VALUE result;
	SOURCE_HANDLE target = target_create();
	if (target == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE address_value = amqpvalue_create_string(address);
		if (address_value == NULL)
		{
			result = NULL;
		}
		else
		{
			if (target_set_address(target, amqpvalue_create_string(address)) != 0)
			{
				result = NULL;
			}
			else
			{
				result = amqpvalue_create_source(target);
			}

			amqpvalue_destroy(address_value);
		}

		source_destroy(target);
	}

	return result;
}
