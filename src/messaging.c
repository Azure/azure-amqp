#include <stdlib.h>
#include <string.h>
#include "messaging.h"
#include "message.h"
#include "amqpvalue.h"
#include "link.h"

typedef struct MESSAGING_DATA_TAG
{
	CONNECTION_HANDLE* connections;
	size_t connection_count;
	SESSION_HANDLE session;
	LINK_HANDLE link;
} MESSAGING_DATA;

MESSAGING_HANDLE messaging_create(void)
{
	MESSAGING_DATA* result = (MESSAGING_DATA*)malloc(sizeof(MESSAGING_DATA));
	if (result != NULL)
	{
		result->connections = NULL;
		result->connection_count = 0;
	}

	return result;
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

int messaging_send(MESSAGING_HANDLE handle, MESSAGE_HANDLE message)
{
	int result;
	MESSAGING_DATA* messaging = (MESSAGING_DATA*)handle;
	CONNECTION_HANDLE connection;

	if (messaging == NULL)
	{
		result = __LINE__;
	}
	else
	{
		size_t i;
		const char* to = message_get_to(message);

		for (i = 0; i < messaging->connection_count; i++)
		{
			if (strcmp(connection_get_address(messaging->connections[i]), to) == 0)
			{
				connection = messaging->connections[i];
				break;
			}
		}

		if (i == messaging->connection_count)
		{
			/* create connection */
			connection = connection_create(to, 5672);
		}

		if (connection == NULL)
		{
			result = __LINE__;
		}
		else
		{
			messaging->session = session_create(result->connection);
			if (result->session == NULL)
			{
				connection_destroy(result->connection);
				free(result);
				result = NULL;
			}
			else
			{
				AMQP_VALUE source_address = amqpvalue_create_string("/");
				AMQP_VALUE target_address = amqpvalue_create_string("/");

				if ((source_address == NULL) ||
					(target_address == NULL))
				{
					connection_destroy(result->connection);
					link_destroy(result->link);
					free(result);
					result = NULL;
				}
				else
				{
					result->link = link_create(result->session, messaging_create_source(source_address), messaging_create_target(amqpvalue_create_string(target_address)));
				}

				amqpvalue_destroy(source_address);
				amqpvalue_destroy(target_address);
			}
		}
	}
}