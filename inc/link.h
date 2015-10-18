#ifndef LINK_H
#define LINK_H

#include <stddef.h>
#include "session.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* LINK_HANDLE;

typedef enum LINK_STATE_TAG
{
	LINK_STATE_DETACHED,
	LINK_STATE_HALF_ATTACHED,
	LINK_STATE_ATTACHED
} LINK_STATE;

typedef void(*ON_DELIVERY_SETTLED)(void* context, delivery_number delivery_no);
typedef void(*ON_TRANSFER_RECEIVED)(void* context, TRANSFER_HANDLE transfer, uint32_t payload_size, const unsigned char* payload_bytes);

extern LINK_HANDLE link_create(SESSION_HANDLE session, const char* name, AMQP_VALUE source, AMQP_VALUE target, ON_TRANSFER_RECEIVED on_transfer_received, void* callback_context);
extern void link_destroy(LINK_HANDLE handle);
extern int link_get_state(LINK_HANDLE handle, LINK_STATE* link_state);
extern int link_transfer(LINK_HANDLE handle, PAYLOAD* payloads, size_t payload_count, ON_DELIVERY_SETTLED on_delivery_settled, void* callback_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINK_H */
