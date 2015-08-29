#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdint.h>
#include "amqpvalue.h"
#include "amqp_frame_codec.h"
#include "connection.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* SESSION_MANAGER_HANDLE;

	extern SESSION_MANAGER_HANDLE session_manager_create(const char* host, int port, CONNECTION_OPTIONS* options);
	extern void session_manager_destroy(SESSION_MANAGER_HANDLE session_manager);
	extern void session_manager_dowork(SESSION_MANAGER_HANDLE session_manager);
	extern SESSION_HANDLE session_manager_create_endpoint(SESSION_MANAGER_HANDLE session_manager, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback, void* context);
	extern void session_manager_destroy_endpoint(SESSION_HANDLE session);
	extern AMQP_FRAME_CODEC_HANDLE session_manager_get_amqp_frame_codec(SESSION_MANAGER_HANDLE session_manager);
	extern CONNECTION_HANDLE session_manager_get_connection(SESSION_MANAGER_HANDLE session_manager);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_MANAGER_H */
