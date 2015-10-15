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
	typedef void* LINK_ENDPOINT_HANDLE;

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

	typedef void(*LINK_ENDPOINT_FRAME_RECEIVED_CALLBACK)(void* context, AMQP_VALUE performative, uint32_t frame_payload_size, const unsigned char* payload_bytes);

	extern SESSION_HANDLE session_create(CONNECTION_HANDLE connection);
	extern void session_destroy(SESSION_HANDLE session);
	extern LINK_ENDPOINT_HANDLE session_create_link_endpoint(SESSION_HANDLE session, const char* name, LINK_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, void* context);
	extern void session_destroy_link_endpoint(LINK_ENDPOINT_HANDLE endpoint);
	extern int session_encode_frame(LINK_ENDPOINT_HANDLE link_endpoint, const AMQP_VALUE performative, PAYLOAD* payloads, size_t payload_count);
	extern int session_transfer(LINK_ENDPOINT_HANDLE link_endpoint, TRANSFER_HANDLE transfer, PAYLOAD* payloads, size_t payload_count, delivery_number* delivery_id);
	extern int session_get_state(SESSION_HANDLE session, SESSION_STATE* session_state);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_H */
