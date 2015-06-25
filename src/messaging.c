#include <stdlib.h>
#include <string.h>
#include "messaging.h"
#include "message.h"
#include "amqpvalue.h"
#include "link.h"
#include "list.h"
#include "amqpalloc.h"

typedef struct MESSAGE_WITH_CALLBACK_TAG
{
	MESSAGE_HANDLE message;
	MESSAGE_SEND_COMPLETE_CALLBACK callback;
	const void* context;
} MESSAGE_WITH_CALLBACK;

typedef struct MESSAGING_DATA_TAG
{
	LIST_HANDLE connections;
	CONNECTION_HANDLE* connection;
	SESSION_HANDLE session;
	LINK_HANDLE link;
	MESSAGE_WITH_CALLBACK* outgoing_messages;
	size_t outgoing_message_count;
} MESSAGING_DATA;

MESSAGING_HANDLE messaging_create(void)
{
	MESSAGING_DATA* result = (MESSAGING_DATA*)amqpalloc_malloc(sizeof(MESSAGING_DATA));
	if (result != NULL)
	{
		result->connections = list_create();
		result->connection = NULL;
		result->session = NULL;
		result->link = NULL;
		result->outgoing_messages = NULL;
		result->outgoing_message_count = 0;
	}

	return result;
}

void messaging_destroy(MESSAGING_HANDLE handle)
{
	if (handle != NULL)
	{
		MESSAGING_DATA* messaging = (MESSAGING_DATA*)handle;
		list_destroy(messaging->connections);
		link_destroy(messaging->link);
		session_destroy(messaging->session);
		connection_destroy(messaging->connection);
		amqpalloc_free(messaging->outgoing_messages);
		amqpalloc_free(handle);
	}
}

AMQP_VALUE messaging_create_source(AMQP_VALUE address)
{
	AMQP_VALUE result = amqpvalue_create_composite_with_ulong_descriptor(0x28, 1);
	if (result != NULL)
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(result);
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
		AMQP_VALUE list_value = amqpvalue_get_described_value(result);
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

int messaging_send(MESSAGING_HANDLE handle, MESSAGE_HANDLE message, MESSAGE_SEND_COMPLETE_CALLBACK callback, const void* callback_context)
{
	int result;
	MESSAGING_DATA* messaging = (MESSAGING_DATA*)handle;
	CONNECTION_HANDLE connection = NULL;

	if (messaging == NULL)
	{
		result = __LINE__;
	}
	else
	{
		const char* to = message_get_to(message);

		if (true)
		{
			/* create connection */
			connection = connection_create(to, 5672);
			messaging->connection = connection;
		}

		if (connection == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (messaging->session == NULL)
			{
				messaging->session = session_create(connection);
				if (messaging->session == NULL)
				{
					result = __LINE__;
				}
			}

			if (messaging->session == NULL)
			{
				result = __LINE__;
			}
			else
			{
				AMQP_VALUE source_address = amqpvalue_create_string("/");
				AMQP_VALUE target_address = amqpvalue_create_string("/");

				if (messaging->link == NULL)
				{
					messaging->link = link_create(messaging->session, source_address, target_address);
					if (messaging->link == NULL)
					{
						result = __LINE__;
					}
				}

				if (messaging->link == NULL)
				{
					result = __LINE__;
				}
				else
				{
					MESSAGE_WITH_CALLBACK* messages = (MESSAGE_WITH_CALLBACK*)amqpalloc_realloc(messaging->outgoing_messages, sizeof(MESSAGE_WITH_CALLBACK) * (messaging->outgoing_message_count + 1));
					if (messages == NULL)
					{
						result = __LINE__;
					}
					else
					{
						messaging->outgoing_messages = messages;
						messaging->outgoing_messages[messaging->outgoing_message_count].message = message;
						messaging->outgoing_messages[messaging->outgoing_message_count].callback = callback;
						messaging->outgoing_messages[messaging->outgoing_message_count].context = callback_context;
						messaging->outgoing_message_count++;

						result = 0;
					}
				}

				amqpvalue_destroy(source_address);
				amqpvalue_destroy(target_address);
			}
		}
	}

	return result;
}

int messaging_dowork(MESSAGING_HANDLE handle)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		MESSAGING_DATA* messaging = (MESSAGING_DATA*)handle;
		LINK_STATE link_state;

		if (link_get_state(messaging->link, &link_state) != 0)
		{
			result = __LINE__;
		}
		else
		{
			if (link_state == LINK_STATE_ATTACHED)
			{
				size_t i;

				for (i = 0; i < messaging->outgoing_message_count; i++)
				{
					AMQP_VALUE message_payload = message_get_body(messaging->outgoing_messages[i].message);
					if (message_payload == NULL)
					{
						result = __LINE__;
					}
					else
					{
						result = link_transfer(messaging->link, &message_payload, 1);
						if (result == 0)
						{
							messaging->outgoing_messages[i].callback(MESSAGING_OK, messaging->outgoing_messages[i].context);
						}
					}
				}

				messaging->outgoing_message_count = 0;
			}

			connection_dowork(messaging->connection);
			if (session_dowork(messaging->session) != 0)
			{
				result = __LINE__;
			}
			else
			{
				if (link_dowork(messaging->link) != 0)
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}
			}
		}
	}

	return result;
}
