#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdint.h>
#include "amqpvalue.h"
#include "amqp_frame_codec.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* SESSION_MANAGER_HANDLE;
	typedef void* SESSION_HANDLE;

	typedef void(*SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK)(void* context, AMQP_VALUE performative, uint32_t frame_payload_size);
	typedef void(*SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK)(void* context, const unsigned char* payload_bytes, uint32_t byte_count);

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

	extern SESSION_MANAGER_HANDLE session_manager_create(const char* host, int port, CONNECTION_OPTIONS* options);
	extern void session_manager_destroy(SESSION_MANAGER_HANDLE session_manager);
	extern void session_manager_dowork(SESSION_MANAGER_HANDLE session_manager);
	extern SESSION_HANDLE session_manager_create_endpoint(SESSION_MANAGER_HANDLE session_manager, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback, void* context);
	extern void session_manager_destroy_endpoint(SESSION_HANDLE session);
	extern int session_get_endpoint_state(SESSION_HANDLE session, SESSION_STATE* session_state);
	extern AMQP_FRAME_CODEC_HANDLE session_manager_get_amqp_frame_codec(SESSION_MANAGER_HANDLE session_manager);
	extern CONNECTION_HANDLE session_manager_get_connection(SESSION_MANAGER_HANDLE session_manager);
	extern AMQP_FRAME_CODEC_HANDLE session_get_amqp_frame_codec(SESSION_HANDLE session);
	extern int session_set_frame_received_callback(SESSION_HANDLE session, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, void* context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_MANAGER_H */
