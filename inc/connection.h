#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <stdint.h>
#include "amqp_frame_codec.h"
#include "amqp_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* CONNECTION_HANDLE;
	typedef void* CHANNEL_ENDPOINT_HANDLE;

	typedef enum CONNECTION_STATE_TAG
	{
		/* Codes_SRS_CONNECTION_01_039: [START In this state a connection exists, but nothing has been sent or received. This is the state an implementation would be in immediately after performing a socket connect or socket accept.] */
		CONNECTION_STATE_START,

		/* Codes_SRS_CONNECTION_01_040: [HDR RCVD In this state the connection header has been received from the peer but a connection header has not been sent.] */
		CONNECTION_STATE_HDR_RCVD,

		/* Codes_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
		CONNECTION_STATE_HDR_SENT,

		/* Codes_SRS_CONNECTION_01_042: [HDR EXCH In this state the connection header has been sent to the peer and a connection header has been received from the peer.] */
		CONNECTION_STATE_HDR_EXCH,

		/* Codes_SRS_CONNECTION_01_043: [OPEN PIPE In this state both the connection header and the open frame have been sent but nothing has been received.] */
		CONNECTION_STATE_OPEN_PIPE,

		/* Codes_SRS_CONNECTION_01_044: [OC PIPE In this state, the connection header, the open frame, any pipelined connection traffic, and the close frame have been sent but nothing has been received.] */
		CONNECTION_STATE_OC_PIPE,

		/* Codes_SRS_CONNECTION_01_045: [OPEN RCVD In this state the connection headers have been exchanged. An open frame has been received from the peer but an open frame has not been sent.] */
		CONNECTION_STATE_OPEN_RCVD,

		/* Codes_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
		CONNECTION_STATE_OPEN_SENT,

		/* Codes_SRS_CONNECTION_01_047: [CLOSE PIPE In this state the connection headers have been exchanged. An open frame, any pipelined connection traffic, and the close frame have been sent but no open frame has yet been received from the peer.] */
		CONNECTION_STATE_CLOSE_PIPE,

		/* Codes_SRS_CONNECTION_01_048: [OPENED In this state the connection header and the open frame have been both sent and received.] */
		CONNECTION_STATE_OPENED,

		/* Codes_SRS_CONNECTION_01_049: [CLOSE RCVD In this state a close frame has been received indicating that the peer has initiated an AMQP close.] */
		CONNECTION_STATE_CLOSE_RCVD,

		/* Codes_SRS_CONNECTION_01_053: [CLOSE SENT In this state a close frame has been sent to the peer. It is illegal to write anything more onto the connection, however there could potentially still be incoming frames.] */
		CONNECTION_STATE_CLOSE_SENT,

		/* Codes_SRS_CONNECTION_01_055: [DISCARDING The DISCARDING state is a variant of the CLOSE SENT state where the close is triggered by an error.] */
		CONNECTION_STATE_DISCARDING,

		/* Codes_SRS_CONNECTION_01_057: [END In this state it is illegal for either endpoint to write anything more onto the connection. The connection can be safely closed and discarded.] */
		CONNECTION_STATE_END
	} CONNECTION_STATE;

#define CONNECTION_OPTION_MAX_FRAME_SIZE	1 << 0
#define CONNECTION_OPTION_CHANNEL_MAX		1 << 1
#define CONNECTION_OPTION_IDLE_TIMEOUT		1 << 2

	typedef struct CONNECTION_OPTIONS_TAG
	{
		uint8_t use_options;
		uint32_t max_frame_size;
		uint16_t channel_max;
		milliseconds idle_timeout;
	} CONNECTION_OPTIONS;

	extern CONNECTION_HANDLE connection_create(const char* host, int port, CONNECTION_OPTIONS* options, AMQP_FRAME_RECEIVED_CALLBACK callback, void* context);
	extern void connection_destroy(CONNECTION_HANDLE connection);
	extern void connection_dowork(CONNECTION_HANDLE connection);
	extern int connection_get_state(CONNECTION_HANDLE connection, CONNECTION_STATE* connection_state);
	extern AMQP_FRAME_CODEC_HANDLE connection_get_amqp_frame_codec(CONNECTION_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONNECTION_H */
