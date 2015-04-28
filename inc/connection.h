#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <stdint.h>
#include "io.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* CONNECTION_HANDLE;

	typedef enum CONNECTION_STATE_TAG
	{
		CONNECTION_STATE_START,
		CONNECTION_STATE_HDR_RCVD,
		CONNECTION_STATE_HDR_SENT,
		CONNECTION_STATE_HDR_EXCH,
		CONNECTION_STATE_OPEN_PIPE,
		CONNECTION_STATE_OC_PIPE,
		CONNECTION_STATE_OPEN_RCVD,
		CONNECTION_STATE_OPEN_SENT,
		CONNECTION_STATE_CLOSE_PIPE,
		CONNECTION_STATE_OPENED,
		CONNECTION_STATE_CLOSE_RCVD,
		CONNECTION_STATE_CLOSE_SENT,
		CONNECTION_STATE_DISCARDING,
		CONNECTION_STATE_END
	} CONNECTION_STATE;

	extern CONNECTION_HANDLE connection_create(const char* host, int port);
	extern void connection_destroy(CONNECTION_HANDLE handle);
	extern int connection_dowork(CONNECTION_HANDLE handle);
	extern int connection_get_state(CONNECTION_HANDLE handle, CONNECTION_STATE* connection_state);
	extern IO_HANDLE connection_get_io(CONNECTION_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONNECTION_H */
