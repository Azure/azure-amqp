#include <stdlib.h>
#include "messaging.h"
#include "amqpvalue.h"
#include "link.h"

typedef struct MESSAGING_DATA_TAG
{
	LINK_HANDLE link;
} MESSAGING_DATA;

MESSAGING_HANDLE messaging_create(void)
{
	MESSAGING_DATA* messaging = (MESSAGING_DATA*)malloc(sizeof(MESSAGING_DATA));
	if (messaging != NULL)
	{

	}

	return messaging;
}

void messaging_destroy(MESSAGING_HANDLE handle)
{
	free(handle);
}

AMQP_VALUE messaging_create_source(AMQP_VALUE address)
{
	AMQP_VALUE result = amqpvalue_create_composite_with_ulong_descriptor(0x28, 1);
	if (result != NULL)
	{
		AMQP_VALUE list_value = amqpvalue_get_composite_list(result);
		if (list_value == NULL)
		{
			amqpvalue_destroy(result);
			result = NULL;
		}
		else
		{
			AMQP_VALUE address_copy_value = amqpvalue_clone(address);
			if (address_copy_value == NULL)
			{
				amqpvalue_destroy(result);
				result = NULL;
			}
			else
			{
				if (amqpvalue_set_list_item(list_value, 0, address_copy_value) != 0)
				{
					amqpvalue_destroy(address_copy_value);
					amqpvalue_destroy(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}

AMQP_VALUE messaging_create_target(AMQP_VALUE address)
{
	AMQP_VALUE result = amqpvalue_create_composite_with_ulong_descriptor(0x29, 1);
	if (result != NULL)
	{
		AMQP_VALUE list_value = amqpvalue_get_composite_list(result);
		if (list_value == NULL)
		{
			amqpvalue_destroy(result);
			result = NULL;
		}
		else
		{
			AMQP_VALUE address_copy_value = amqpvalue_clone(address);
			if (address_copy_value == NULL)
			{
				amqpvalue_destroy(result);
				result = NULL;
			}
			else
			{
				if (amqpvalue_set_list_item(list_value, 0, address_copy_value) != 0)
				{
					amqpvalue_destroy(address_copy_value);
					amqpvalue_destroy(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}
