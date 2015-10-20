#include "message_sender.h"
#include "amqpalloc.h"

typedef struct MESSAGE_WITH_CALLBACK_TAG
{
	MESSAGE_HANDLE message;
	ON_MESSAGE_SEND_COMPLETE on_message_send_complete;
	const void* context;
} MESSAGE_WITH_CALLBACK;

typedef struct MESSAGE_SENDER_INSTANCE_TAG
{
	LINK_HANDLE link;
} MESSAGE_SENDER_INSTANCE;

MESSAGE_SENDER_HANDLE messagesender_create(LINK_HANDLE link)
{
	MESSAGE_SENDER_INSTANCE* result = amqpalloc_malloc(sizeof(MESSAGE_SENDER_INSTANCE));
	if (result != NULL)
	{
		result->link = link;
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
		result = 0;
	}

	return result;
}
