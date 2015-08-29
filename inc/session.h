#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include "amqpvalue.h"
#include "amqp_frame_codec.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
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

	extern SESSION_HANDLE session_create(CONNECTION_HANDLE connection);
	extern int session_get_state(SESSION_HANDLE session, SESSION_STATE* session_state);
	extern void session_dowork(SESSION_HANDLE session);
	extern AMQP_FRAME_CODEC_HANDLE session_get_amqp_frame_codec(SESSION_HANDLE session);
	extern int session_set_frame_received_callback(SESSION_HANDLE session, ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, void* context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_H */
