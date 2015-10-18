#ifndef DELIVERY_QUEUE_H
#define DELIVERY_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include "amqp_definitions.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* DELIVERY_QUEUE_HANDLE;

	extern DELIVERY_QUEUE_HANDLE deliveryqueue_create(void);
	extern void deliveryqueue_destroy(DELIVERY_QUEUE_HANDLE delivery_queue);
	extern int deliveryqueue_transfer(DELIVERY_QUEUE_HANDLE delivery_queue, TRANSFER_HANDLE transfer, PAYLOAD* payloads, size_t payload_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DELIVERY_QUEUE_H */
