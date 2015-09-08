#ifndef LINK_H
#define LINK_H

#include <stddef.h>
#include "session.h"
#include "amqpvalue.h"

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

typedef void(*DELIVERY_SETTLED_CALLBACK)(void* context, delivery_number delivery_no);

extern LINK_HANDLE link_create(SESSION_HANDLE session, const char* name, AMQP_VALUE source, AMQP_VALUE target, DELIVERY_SETTLED_CALLBACK delivery_settled_callback, void* callback_context);
extern void link_destroy(LINK_HANDLE handle);
extern void link_dowork(LINK_HANDLE handle);
extern int link_get_state(LINK_HANDLE handle, LINK_STATE* link_state);
extern int link_transfer(LINK_HANDLE link, AMQP_VALUE payload);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINK_H */
