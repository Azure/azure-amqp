#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "message_receiver.h"
#include "amqpalloc.h"

typedef struct MESSAGE_RECEIVER_INSTANCE_TAG
{
	LINK_HANDLE link;
	ON_MESSAGE_RECEIVED on_message_received;
	const void* callback_context;
} MESSAGE_RECEIVER_INSTANCE;

static void on_transfer_received(void* context, TRANSFER_HANDLE transfer, uint32_t payload_size, const unsigned char* payload_bytes)
{
    MESSAGE_RECEIVER_INSTANCE* message_receiver_instance = (MESSAGE_RECEIVER_INSTANCE*)context;
    if (message_receiver_instance->on_message_received != NULL)
    {
        MESSAGE_HANDLE message = NULL;
        message_receiver_instance->on_message_received(message_receiver_instance->callback_context, message);
    }
}

static void on_link_state_changed(void* context, LINK_STATE new_link_state, LINK_STATE previous_link_state)
{
}

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
        if (link_subscribe_events(message_receiver_instance->link, on_transfer_received, on_link_state_changed, message_receiver_instance) != 0)
        {
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
	}

	return result;
}
