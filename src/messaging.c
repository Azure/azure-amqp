#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "messaging.h"
#include "connection.h"
#include "message.h"
#include "amqpvalue.h"
#include "link.h"
#include "list.h"
#include "amqpalloc.h"
#include "session.h"
#include "tlsio.h"
#include "saslio.h"
#include "consolelogger.h"
#include "message_sender.h"

typedef struct MESSAGE_WITH_CALLBACK_TAG
{
	MESSAGE_HANDLE message;
	ON_MESSAGE_SEND_COMPLETE on_message_send_complete;
	const void* context;
} MESSAGE_WITH_CALLBACK;

typedef struct MESSAGING_INSTANCE_TAG
{
	LIST_HANDLE connections;
	IO_HANDLE io;
	CONNECTION_HANDLE connection;
	SESSION_HANDLE session;
	LINK_HANDLE link;
	MESSAGE_WITH_CALLBACK* outgoing_messages;
	size_t outgoing_message_count;
} MESSAGING_INSTANCE;

MESSAGING_HANDLE messaging_create(void)
{
	MESSAGING_INSTANCE* result = (MESSAGING_INSTANCE*)amqpalloc_malloc(sizeof(MESSAGING_INSTANCE));
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
		MESSAGING_INSTANCE* messaging = (MESSAGING_INSTANCE*)handle;
		list_destroy(messaging->connections);
		link_destroy(messaging->link);
		connection_destroy(messaging->connection);
		amqpalloc_free(messaging->outgoing_messages);
		amqpalloc_free(handle);
	}
}

AMQP_VALUE messaging_create_source(AMQP_VALUE address)
{
	AMQP_VALUE result;
	SOURCE_HANDLE source = source_create();
	source_set_address(source, amqpvalue_create_string(address));
	result = amqpvalue_create_source(source);
	source_destroy(source);
	return result;
}

AMQP_VALUE messaging_create_target(AMQP_VALUE address)
{
	AMQP_VALUE result;
	TARGET_HANDLE target = target_create();
	source_set_address(target, amqpvalue_create_string(address));
	result = amqpvalue_create_source(target);
	source_destroy(target);
	return result;
}

static void delivery_settled_callback(void* context, delivery_number delivery_no)
{
	MESSAGING_INSTANCE* messaging_instance = (MESSAGING_INSTANCE*)context;
	size_t i;

	for (i = 0; i < messaging_instance->outgoing_message_count; i++)
	{
		messaging_instance->outgoing_messages[i].on_message_send_complete(messaging_instance->outgoing_messages[i].context, MESSAGE_SEND_OK);
	}
}

static void on_transfer_received(void* context, TRANSFER_HANDLE transfer, uint32_t payoad_size, const unsigned char* payload_bytes)
{
}

static void on_link_state_changed(void* context, LINK_STATE new_link_state, LINK_STATE previous_link_state)
{
	MESSAGING_INSTANCE* messaging_instance = (MESSAGING_INSTANCE*)context;

	if (new_link_state == LINK_STATE_HALF_ATTACHED)
	{
		size_t i;

		for (i = 0; i < messaging_instance->outgoing_message_count; i++)
		{
			BINARY_DATA binary_data;
			(void)message_get_body_amqp_data(messaging_instance->outgoing_messages[i].message, &binary_data);
			PAYLOAD payload = { binary_data.bytes, binary_data.length };
			if (link_transfer(messaging_instance->link, &payload, 1, delivery_settled_callback, messaging_instance) != 0)
			{
				/* error */
			}
		}
	}
}

int messaging_send(MESSAGING_HANDLE handle, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, const void* callback_context)
{
	int result;
	MESSAGING_INSTANCE* messaging = (MESSAGING_INSTANCE*)handle;
	CONNECTION_HANDLE connection = NULL;

	if (messaging == NULL)
	{
		result = __LINE__;
	}
	else
	{
		const char* to = message_get_to(message);

		if (messaging->connection == NULL)
		{
			/* create io */
			const TLSIO_CONFIG socket_io_config = { to, 5671 };
			const IO_INTERFACE_DESCRIPTION* io_interface_description;

			io_interface_description = tlsio_get_interface_description();
			if (io_interface_description == NULL)
			{
				result = __LINE__;
			}
			else
			{
				SASLIO_CONFIG sasl_io_config = { io_interface_description, &socket_io_config };

				io_interface_description = saslio_get_interface_description();
				if (io_interface_description == NULL)
				{
					result = __LINE__;
				}
				else
				{
					messaging->io = io_create(io_interface_description, &sasl_io_config, NULL);
					connection = connection_create(messaging->io, to, "1234");
					messaging->connection = connection;
				}
			}
		}

		if (messaging->connection == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (messaging->session == NULL)
			{
				messaging->session = session_create(messaging->connection);
				if (messaging->session == NULL)
				{
					result = __LINE__;
				}
				else
				{
					session_set_outgoing_window(messaging->session, 10);
				}
			}

			if (messaging->session == NULL)
			{
				result = __LINE__;
			}
			else
			{
				SOURCE_HANDLE source = source_create();
				TARGET_HANDLE target = target_create();

				source_set_address(source, amqpvalue_create_string("ingress"));
				source_set_durable(source, false);
				source_set_timeout(source, 0);
				source_set_dynamic(source, false);
				target_set_address(target, amqpvalue_create_string("amqps://pupupupu.servicebus.windows.net/ingress"));
				target_set_durable(target, false);
				target_set_timeout(target, 0);
				target_set_dynamic(target, false);

				AMQP_VALUE source_value = amqpvalue_create_source(source);
				AMQP_VALUE target_value = amqpvalue_create_target(target);

				if (messaging->link == NULL)
				{
					messaging->link = link_create(messaging->session, "voodoo", source_value, target_value);
					if (messaging->link == NULL)
					{
						result = __LINE__;
					}
					else
					{
						(void)link_subscribe_events(messaging->link, on_transfer_received, on_link_state_changed, messaging);
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
						messaging->outgoing_messages[messaging->outgoing_message_count].on_message_send_complete = on_message_send_complete;
						messaging->outgoing_messages[messaging->outgoing_message_count].context = callback_context;
						messaging->outgoing_message_count++;

						result = 0;
					}
				}

				amqpvalue_destroy(source_value);
				amqpvalue_destroy(target_value);
			}
		}
	}

	return result;
}

int messaging_receive(MESSAGING_HANDLE handle, const char* source, ON_MESSAGE_RECEIVE on_message_receive, const void* callback_context)
{
	MESSAGING_INSTANCE* messaging = (MESSAGING_INSTANCE*)handle;

	return 0;
}

void messaging_dowork(MESSAGING_HANDLE handle)
{
	if (handle != NULL)
	{
		MESSAGING_INSTANCE* messaging = (MESSAGING_INSTANCE*)handle;

		if (messaging->connection != NULL)
		{
			connection_dowork(messaging->connection);
		}
	}
}
