#ifndef SESSION_H
#define SESSION_H

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

	typedef void* SESSION_HANDLE;
	typedef void(*SESSION_FRAME_RECEIVED_CALLBACK)(void* context, uint64_t performative, AMQP_VALUE frame_list_value);

	extern SESSION_HANDLE session_create(CONNECTION_HANDLE connection);
	extern void session_destroy(SESSION_HANDLE handle);
	extern int session_dowork(SESSION_HANDLE handle);
	extern int session_set_frame_received_callback(SESSION_HANDLE handle, SESSION_FRAME_RECEIVED_CALLBACK callback, void* context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_H */
