#include "message_receiver.h"
#include "amqpalloc.h"

typedef struct MESSAGE_RECEIVER_INSTANCE_TAG
{
	LINK_HANDLE link;
	ON_MESSAGE_RECEIVED on_message_received;
	const void* callback_context;
} MESSAGE_RECEIVER_INSTANCE;

MESSAGE_RECEIVER_HANDLE messagereceiver_create(LINK_HANDLE link)
{
	MESSAGE_RECEIVER_INSTANCE* result = (MESSAGE_RECEIVER_INSTANCE*)amqpalloc_malloc(sizeof(MESSAGE_RECEIVER_INSTANCE));
	if (result != NULL)
	{
		result->link = link;
	}

	return result;
}

void messagereceiver_destroy(MESSAGE_RECEIVER_HANDLE message_receiver)
{
	if (message_receiver != NULL)
	{
		amqpalloc_free(message_receiver);
	}
}

int messagereceiver_subscribe(MESSAGE_RECEIVER_HANDLE message_receiver, ON_MESSAGE_RECEIVED on_message_received, const void* callback_context)
{
	int result;

	if (message_receiver == NULL)
	{
		result = __LINE__;
	}
	else
	{
		MESSAGE_RECEIVER_INSTANCE* message_receiver_instance = (MESSAGE_RECEIVER_INSTANCE*)message_receiver;

		message_receiver_instance->on_message_received = on_message_received;
		message_receiver_instance->callback_context = callback_context;

		result = 0;
	}

	return result;
}
