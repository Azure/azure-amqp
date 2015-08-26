#ifndef SESSION_MONITOR_H
#define SESSION_MONITOR_H

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* SESSION_MONITOR_HANDLE;
	typedef void* SESSION_ENDPOINT_HANDLE;

	typedef void(*SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK)(void* context, AMQP_VALUE performative, uint32_t frame_payload_size);
	typedef void(*SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK)(void* context, const unsigned char* payload_bytes, uint32_t byte_count);

	extern SESSION_MONITOR_HANDLE session_monitor_create(CONNECTION_HANDLE connection);
	extern void session_monitor_destroy(SESSION_MONITOR_HANDLE session_monitor);
	extern SESSION_ENDPOINT_HANDLE session_monitor_register_endpoint(SESSION_MONITOR_HANDLE session_monitor, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback, void* context);
	extern void session_monitor_unregister_endpoint(SESSION_ENDPOINT_HANDLE session_endpoint);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_MONITOR_H */
