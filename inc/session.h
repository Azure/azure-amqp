#ifndef SESSION_H
#define SESSION_H

#include "connection.h"
#include "frame_codec.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

	typedef void* SESSION_HANDLE;
	
	typedef enum SESION_STATE_TAG
	{
		SESSION_STATE_UNMAPPED,
		SESSION_STATE_BEGIN_SENT,
		SESSION_STATE_BEGIN_RCVD,
		SESSION_STATE_MAPPED,
		SESSION_STATE_END_SENT,
		SESSION_STATE_END_RCVD,
		SESSION_STATE_DISCARDING
	} SESSION_STATE;

	typedef void(*SESSION_FRAME_RECEIVED_CALLBACK)(void* context, uint64_t performative, AMQP_VALUE frame_list_value);

	extern SESSION_HANDLE session_create(CONNECTION_HANDLE connection);
	extern void session_destroy(SESSION_HANDLE handle);
	extern int session_dowork(SESSION_HANDLE handle);
	extern int session_set_frame_received_callback(SESSION_HANDLE handle, SESSION_FRAME_RECEIVED_CALLBACK callback, void* context);
	extern int session_get_state(SESSION_HANDLE handle, SESSION_STATE* session_state);
	extern FRAME_CODEC_HANDLE session_get_frame_codec(SESSION_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_H */
