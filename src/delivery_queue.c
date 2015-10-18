#include "delivery_queue.h"
#include "amqpalloc.h"

typedef struct DELIVERY_QUEUE_INSTANCE_TAG
{
	size_t pending_delivery_count;
} DELIVERY_QUEUE_INSTANCE;

DELIVERY_QUEUE_HANDLE deliveryqueue_create(void)
{
	DELIVERY_QUEUE_INSTANCE* result = (DELIVERY_QUEUE_INSTANCE*)amqpalloc_malloc(sizeof(DELIVERY_QUEUE_INSTANCE));
	if (result != NULL)
	{
		result->pending_delivery_count = 0;
	}

	return result;
}

void deliveryqueue_destroy(DELIVERY_QUEUE_HANDLE delivery_queue)
{
	if (delivery_queue != NULL)
	{
		amqpalloc_free(delivery_queue);
	}
}

int deliveryqueue_transfer(DELIVERY_QUEUE_HANDLE delivery_queue, TRANSFER_HANDLE transfer, PAYLOAD* payloads, size_t payload_count)
{
	int result;

	if ((delivery_queue == NULL) ||
		(transfer == NULL))
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}
