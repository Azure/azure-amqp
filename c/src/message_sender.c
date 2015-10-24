#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <string.h>
#include "message_sender.h"
#include "amqpalloc.h"
#include "logger.h"
#include "consolelogger.h"

typedef enum MESSAGE_SENDER_STATE_TAG
{
	MESSAGE_SENDER_STATE_IDLE,
	MESSAGE_SENDER_STATE_CONNECTED
} MESSAGE_SENDER_STATE;

typedef enum MESSAGE_SEND_STATE_TAG
{
	MESSAGE_SEND_STATE_NOT_SENT,
	MESSAGE_SEND_STATE_PENDING
} MESSAGE_SEND_STATE;

typedef struct MESSAGE_WITH_CALLBACK_TAG
{
	MESSAGE_HANDLE message;
	ON_MESSAGE_SEND_COMPLETE on_message_send_complete;
	const void* context;
	MESSAGE_SENDER_HANDLE message_sender;
	MESSAGE_SEND_STATE message_send_state;
} MESSAGE_WITH_CALLBACK;

typedef struct MESSAGE_SENDER_INSTANCE_TAG
{
	LINK_HANDLE link;
	size_t message_count;
	MESSAGE_WITH_CALLBACK* messages;
	MESSAGE_SENDER_STATE message_sender_state;
} MESSAGE_SENDER_INSTANCE;

static void on_delivery_settled(void* context, delivery_number delivery_no)
{
	MESSAGE_WITH_CALLBACK* message_with_callback = (MESSAGE_WITH_CALLBACK*)context;
	message_with_callback->on_message_send_complete(message_with_callback->context, MESSAGE_SEND_OK);
}

static void remove_pending_message(MESSAGE_SENDER_INSTANCE* message_sender_instance, size_t index)
{
	MESSAGE_WITH_CALLBACK* new_messages;

	if (message_sender_instance->messages[index].message != NULL)
	{
		message_destroy(message_sender_instance->messages[index].message);
		message_sender_instance->messages[index].message = NULL;
	}

	if (message_sender_instance->message_count - index > 1)
	{
		(void)memmove(&message_sender_instance->messages[index], &message_sender_instance->messages[index + 1], sizeof(MESSAGE_WITH_CALLBACK) * (message_sender_instance->message_count - index - 1));
	}

	message_sender_instance->message_count--;
	new_messages = (MESSAGE_WITH_CALLBACK*)realloc(message_sender_instance->messages, sizeof(MESSAGE_WITH_CALLBACK) * (message_sender_instance->message_count));
	if (new_messages != NULL)
	{
		message_sender_instance->messages = new_messages;
	}
}

static void send_all_pending_messages(MESSAGE_SENDER_INSTANCE* message_sender_instance)
{
	size_t i;

	for (i = 0; i < message_sender_instance->message_count; i++)
	{
		if (message_sender_instance->messages[i].message_send_state == MESSAGE_SEND_STATE_NOT_SENT)
		{
			BINARY_DATA binary_data;
			if (message_get_body_amqp_data(message_sender_instance->messages[i].message, &binary_data) != 0)
			{
				message_sender_instance->messages[i].on_message_send_complete(message_sender_instance->messages[i].context, MESSAGE_SEND_ERROR);
				remove_pending_message(message_sender_instance, i);
				i--;
			}
			else
			{
				PAYLOAD payload = { binary_data.bytes, binary_data.length };
				if (link_transfer(message_sender_instance->link, &payload, 1, on_delivery_settled, &message_sender_instance->messages[i]) != 0)
				{
					message_sender_instance->messages[i].on_message_send_complete(message_sender_instance->messages[i].context, MESSAGE_SEND_ERROR);
					remove_pending_message(message_sender_instance, i);
					i--;
				}
				else
				{
					message_sender_instance->messages[message_sender_instance->message_count].message_send_state = MESSAGE_SEND_STATE_PENDING;
				}
			}
		}
	}
}

static void on_link_state_changed(void* context, LINK_STATE new_link_state, LINK_STATE previous_link_state)
{
	MESSAGE_SENDER_INSTANCE* message_sender_instance = (MESSAGE_SENDER_INSTANCE*)context;

	if ((new_link_state == LINK_STATE_HALF_ATTACHED) ||
		(new_link_state == LINK_STATE_ATTACHED))
	{
		message_sender_instance->message_sender_state = MESSAGE_SENDER_STATE_CONNECTED;
		send_all_pending_messages(message_sender_instance);
	}
	else
	{
		message_sender_instance->message_sender_state = MESSAGE_SENDER_STATE_IDLE;
	}
}

MESSAGE_SENDER_HANDLE messagesender_create(LINK_HANDLE link)
{
	MESSAGE_SENDER_INSTANCE* result = amqpalloc_malloc(sizeof(MESSAGE_SENDER_INSTANCE));
	if (result != NULL)
	{
		if (link_subscribe_events(link, NULL, on_link_state_changed, result) != 0)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			result->messages = NULL;
			result->message_count = 0;
			result->link = link;
			result->message_sender_state = MESSAGE_SENDER_STATE_IDLE;
		}
	}

	return result;
}

void messagesender_destroy(MESSAGE_SENDER_HANDLE message_sender)
{
	if (message_sender != NULL)
	{
		amqpalloc_free(message_sender);
	}
}

int messagesender_send(MESSAGE_SENDER_HANDLE message_sender, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, const void* callback_context)
{
	int result;

	if ((message_sender == NULL) ||
		(message == NULL))
	{
		result = __LINE__;
	}
	else
	{
		MESSAGE_SENDER_INSTANCE* message_sender_instance = (MESSAGE_SENDER_INSTANCE*)message_sender;
		MESSAGE_WITH_CALLBACK* new_messages = (MESSAGE_WITH_CALLBACK*)realloc(message_sender_instance->messages, sizeof(MESSAGE_WITH_CALLBACK) * (message_sender_instance->message_count + 1));
		if (new_messages == NULL)
		{
			result = __LINE__;
		}
		else
		{
			message_sender_instance->messages = new_messages;
			if (message_sender_instance->message_sender_state == MESSAGE_SENDER_STATE_IDLE)
			{
				message_sender_instance->messages[message_sender_instance->message_count].message = message_clone(message);
				message_sender_instance->messages[message_sender_instance->message_count].message_send_state = MESSAGE_SEND_STATE_NOT_SENT;
			}
			else
			{
				message_sender_instance->messages[message_sender_instance->message_count].message = NULL;
				message_sender_instance->messages[message_sender_instance->message_count].message_send_state = MESSAGE_SEND_STATE_PENDING;
			}

			message_sender_instance->messages[message_sender_instance->message_count].on_message_send_complete = on_message_send_complete;
			message_sender_instance->messages[message_sender_instance->message_count].context = callback_context;
			message_sender_instance->messages[message_sender_instance->message_count].message_sender = message_sender_instance;
			message_sender_instance->message_count++;

			if (message_sender_instance->message_sender_state != MESSAGE_SENDER_STATE_IDLE)
			{
				BINARY_DATA binary_data;
				if (message_get_body_amqp_data(message, &binary_data) != 0)
				{
					remove_pending_message(message_sender_instance, message_sender_instance->message_count - 1);
					result = __LINE__;
				}
				else
				{
					PAYLOAD payload = { binary_data.bytes, binary_data.length };
					if (link_transfer(message_sender_instance->link, &payload, 1, on_delivery_settled, message_sender_instance) != 0)
					{
						remove_pending_message(message_sender_instance, message_sender_instance->message_count - 1);
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}
