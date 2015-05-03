#ifndef LINK_H
#define LINK_H

#include "session.h"

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

extern LINK_HANDLE link_create(SESSION_HANDLE session, AMQP_VALUE source, AMQP_VALUE target);
extern void link_destroy(LINK_HANDLE handle);
extern int link_dowork(LINK_HANDLE handle);
extern int link_get_state(LINK_HANDLE handle, LINK_STATE* link_state);
extern int link_transfer(LINK_HANDLE link, const AMQP_VALUE* payload_chunks, size_t payload_chunk_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINK_H */
