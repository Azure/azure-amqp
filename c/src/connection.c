#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <string.h>
#include "connection.h"
#include "consolelogger.h"
#include "frame_codec.h"
#include "amqp_frame_codec.h"
#include "amqp_definitions.h"
#include "tlsio.h"
#include "saslio.h"
#include "amqpalloc.h"
#include "logger.h"
#include "amqpvalue_to_string.h"

/* Requirements satisfied by the virtue of implementing the ISO:*/
/* Codes_SRS_CONNECTION_01_088: [Any data appearing beyond the protocol header MUST match the version indicated by the protocol header.] */
/* Codes_SRS_CONNECTION_01_015: [Implementations SHOULD NOT expect to be able to reuse open TCP sockets after close performatives have been exchanged.] */

/* Codes_SRS_CONNECTION_01_087: [The protocol header consists of the upper case ASCII letters “AMQP” followed by a protocol id of zero, followed by three unsigned bytes representing the major, minor, and revision of the protocol version (currently 1 (MAJOR), 0 (MINOR), 0 (REVISION)). In total this is an 8-octet sequence] */
static unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_FRAME_DATA
} RECEIVE_FRAME_STATE;

typedef struct ENDPOINT_INSTANCE_TAG
{
	uint16_t incoming_channel;
	uint16_t outgoing_channel;
	ON_ENDPOINT_FRAME_RECEIVED on_endpoint_frame_received;
	ON_CONNECTION_STATE_CHANGED on_connection_state_changed;
	void* callback_context;
	CONNECTION_HANDLE connection;
} ENDPOINT_INSTANCE;

typedef struct CONNECTION_DATA_TAG
{
	IO_HANDLE io;
	size_t header_bytes_received;
	CONNECTION_STATE connection_state;
	FRAME_CODEC_HANDLE frame_codec;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec;
	ENDPOINT_INSTANCE** endpoints;
	uint32_t endpoint_count;
	char* host_name;
	char* container_id;
	uint32_t remote_max_frame_size;

	/* options */
	uint32_t max_frame_size;
	uint16_t channel_max;
	milliseconds idle_timeout;

	unsigned int is_io_open : 1;
	unsigned int idle_timeout_specified : 1;
} CONNECTION_INSTANCE;

/* Codes_SRS_CONNECTION_01_258: [on_connection_state_changed shall be invoked whenever the connection state changes.]*/
static void connection_set_state(CONNECTION_INSTANCE* connection_instance, CONNECTION_STATE connection_state)
{
	uint64_t i;

	CONNECTION_STATE previous_state = connection_instance->connection_state;
	connection_instance->connection_state = connection_state;

	/* Codes_SRS_CONNECTION_01_260: [Each endpoint’s on_connection_state_changed shall be called.] */
	for (i = 0; i < connection_instance->endpoint_count; i++)
	{
		/* Codes_SRS_CONNECTION_01_259: [The callback_context passed in connection_create_endpoint.] */
		connection_instance->endpoints[i]->on_connection_state_changed(connection_instance->endpoints[i]->callback_context, connection_state, previous_state);
	}
}

static int send_header(CONNECTION_INSTANCE* connection_instance)
{
	int result;

	/* Codes_SRS_CONNECTION_01_093: [_ When the client opens a new socket connection to a server, it MUST send a protocol header with the client’s preferred protocol version.] */
	/* Codes_SRS_CONNECTION_01_104: [Sending the protocol header shall be done by using io_send.] */
	if (io_send(connection_instance->io, amqp_header, sizeof(amqp_header)) != 0)
	{
		/* Codes_SRS_CONNECTION_01_106: [When sending the protocol header fails, the connection shall be immediately closed.] */
		io_close(connection_instance->io);

		/* Codes_SRS_CONNECTION_01_057: [END In this state it is illegal for either endpoint to write anything more onto the connection. The connection can be safely closed and discarded.] */
		connection_set_state(connection_instance, CONNECTION_STATE_END);

		/* Codes_SRS_CONNECTION_01_105: [When io_send fails, connection_dowork shall return a non-zero value.] */
		result = __LINE__;
	}
	else
	{
		LOG(consolelogger_log, LOG_LINE, "-> Header (AMQP 0.1.0.0)");

		/* Codes_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
		connection_set_state(connection_instance, CONNECTION_STATE_HDR_SENT);
		result = 0;
	}

	return result;
}

static const char* get_frame_type_as_string(AMQP_VALUE descriptor)
{
	const char* result;

	if (is_open_type_by_descriptor(descriptor))
	{
		result = "[OPEN]";
	}
	else if (is_begin_type_by_descriptor(descriptor))
	{
		result = "[BEGIN]";
	}
	else if (is_attach_type_by_descriptor(descriptor))
	{
		result = "[ATTACH]";
	}
	else if (is_flow_type_by_descriptor(descriptor))
	{
		result = "[FLOW]";
	}
	else if (is_disposition_type_by_descriptor(descriptor))
	{
		result = "[DISPOSITION]";
	}
	else if (is_transfer_type_by_descriptor(descriptor))
	{
		result = "[TRANSFER]";
	}
	else if (is_detach_type_by_descriptor(descriptor))
	{
		result = "[DETACH]";
	}
	else if (is_end_type_by_descriptor(descriptor))
	{
		result = "[END]";
	}
	else if (is_close_type_by_descriptor(descriptor))
	{
		result = "[CLOSE]";
	}
	else
	{
		result = "[Unknown]";
	}

	return result;
}

void log_incoming_frame(AMQP_VALUE performative)
{
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
	if (descriptor != NULL)
	{
		LOG(consolelogger_log, 0, "<- ");
		LOG(consolelogger_log, 0, (char*)get_frame_type_as_string(descriptor));
		char* performative_as_string = NULL;
		LOG(consolelogger_log, LOG_LINE, (performative_as_string = amqpvalue_to_string(performative)));
		if (performative_as_string != NULL)
		{
			amqpalloc_free(performative_as_string);
		}
	}
}

void log_outgoing_frame(AMQP_VALUE performative)
{
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
	if (descriptor != NULL)
	{
		LOG(consolelogger_log, 0, "-> ");
		LOG(consolelogger_log, 0, (char*)get_frame_type_as_string(descriptor));
		char* performative_as_string = NULL;
		LOG(consolelogger_log, LOG_LINE, (performative_as_string = amqpvalue_to_string(performative)));
		if (performative_as_string != NULL)
		{
			amqpalloc_free(performative_as_string);
		}
	}
}

static int send_open_frame(CONNECTION_INSTANCE* connection_instance)
{
	int result;

	/* Codes_SRS_CONNECTION_01_151: [The connection max_frame_size setting shall be passed down to the frame_codec when the Open frame is sent.] */
	if (frame_codec_set_max_frame_size(connection_instance->frame_codec, connection_instance->max_frame_size) != 0)
	{
		/* Codes_SRS_CONNECTION_01_207: [If frame_codec_set_max_frame_size fails the connection shall be closed and the state set to END.] */
		io_close(connection_instance->io);
		connection_set_state(connection_instance, CONNECTION_STATE_END);
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_CONNECTION_01_134: [The container id field shall be filled with the container id specified in connection_create.] */
		OPEN_HANDLE open_performative = open_create(connection_instance->container_id);
		if (open_performative == NULL)
		{
			/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
			io_close(connection_instance->io);
			connection_set_state(connection_instance, CONNECTION_STATE_END);
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_137: [The max_frame_size connection setting shall be set in the open frame by using open_set_max_frame_size.] */
			if (open_set_max_frame_size(open_performative, connection_instance->max_frame_size) != 0)
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_set_state(connection_instance, CONNECTION_STATE_END);
				result = __LINE__;
			}
			/* Codes_SRS_CONNECTION_01_139: [The channel_max connection setting shall be set in the open frame by using open_set_channel_max.] */
			else if (open_set_channel_max(open_performative, connection_instance->channel_max) != 0)
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_set_state(connection_instance, CONNECTION_STATE_END);
				result = __LINE__;
			}
			/* Codes_SRS_CONNECTION_01_142: [If no idle_timeout value has been specified, no value shall be stamped in the open frame (no call to open_set_idle_time_out shall be made).] */
			else if ((connection_instance->idle_timeout_specified) &&
				/* Codes_SRS_CONNECTION_01_141: [If idle_timeout has been specified by a call to connection_set_idle_timeout, then that value shall be stamped in the open frame.] */
				(open_set_idle_time_out(open_performative, connection_instance->idle_timeout) != 0))
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_set_state(connection_instance, CONNECTION_STATE_END);
				result = __LINE__;
			}
			/* Codes_SRS_CONNECTION_01_136: [If no hostname value has been specified, no value shall be stamped in the open frame (no call to open_set_hostname shall be made).] */
			else if ((connection_instance->host_name != NULL) &&
				/* Codes_SRS_CONNECTION_01_135: [If hostname has been specified by a call to connection_set_hostname, then that value shall be stamped in the open frame.] */
				(open_set_hostname(open_performative, connection_instance->host_name) != 0))
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_set_state(connection_instance, CONNECTION_STATE_END);
				result = __LINE__;
			}
			else
			{
				AMQP_VALUE open_performative_value = amqpvalue_create_open(open_performative);
				if (open_performative_value == NULL)
				{
					/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
					io_close(connection_instance->io);
					connection_set_state(connection_instance, CONNECTION_STATE_END);
					result = __LINE__;
				}
				else
				{
					/* Codes_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.] */
					/* Codes_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
					/* Codes_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
					/* Codes_SRS_CONNECTION_01_205: [Sending the AMQP OPEN frame shall be done by calling amqp_frame_codec_begin_encode_frame with channel number 0, the actual performative payload and 0 as payload_size.] */
					/* Codes_SRS_CONNECTION_01_006: [The open frame can only be sent on channel 0.] */
					if (amqp_frame_codec_begin_encode_frame(connection_instance->amqp_frame_codec, 0, open_performative_value, 0) != 0)
					{
						/* Codes_SRS_CONNECTION_01_206: [If sending the frame fails, the connection shall be closed and state set to END.] */
						io_close(connection_instance->io);
						connection_set_state(connection_instance, CONNECTION_STATE_END);
						result = __LINE__;
					}
					else
					{
						log_outgoing_frame(open_performative_value);

						/* Codes_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
						connection_set_state(connection_instance, CONNECTION_STATE_OPEN_SENT);
						result = 0;
					}

					amqpvalue_destroy(open_performative_value);
				}
			}

			open_destroy(open_performative);
		}
	}

	return result;
}

static int send_close_frame(CONNECTION_INSTANCE* connection_instance, ERROR_HANDLE error_handle)
{
	int result;
	CLOSE_HANDLE close_performative;

	/* Codes_SRS_CONNECTION_01_217: [The CLOSE frame shall be constructed by using close_create.] */
	close_performative = close_create();
	if (close_performative == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if ((error_handle != NULL) &&
			/* Codes_SRS_CONNECTION_01_238: [If set, this field indicates that the connection is being closed due to an error condition.] */
			(close_set_error(close_performative, error_handle) != 0))
		{
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE close_performative_value = amqpvalue_create_close(close_performative);
			if (close_performative_value == NULL)
			{
				result = __LINE__;
			}
			else
			{
				/* Codes_SRS_CONNECTION_01_215: [Sending the AMQP CLOSE frame shall be done by calling amqp_frame_codec_begin_encode_frame with channel number 0, the actual performative payload and 0 as payload_size.] */
				/* Codes_SRS_CONNECTION_01_013: [However, implementations SHOULD send it on channel 0] */
				if (amqp_frame_codec_begin_encode_frame(connection_instance->amqp_frame_codec, 0, close_performative_value, 0) != 0)
				{
					result = __LINE__;
				}
				else
				{
					log_outgoing_frame(close_performative_value);
					result = 0;
				}

				amqpvalue_destroy(close_performative_value);
			}
		}

		close_destroy(close_performative);
	}

	return result;
}

static void close_connection_with_error(CONNECTION_INSTANCE* connection_instance, const char* condition_value, const char* description)
{
	ERROR_HANDLE error_handle = error_create(condition_value);
	if (error_handle == NULL)
	{
		/* Codes_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
		(void)io_close(connection_instance->io);
		connection_set_state(connection_instance, CONNECTION_STATE_END);
	}
	else
	{
		/* Codes_SRS_CONNECTION_01_219: [The error description shall be set to an implementation defined string.] */
		if ((error_set_description(error_handle, description) != 0) ||
			(send_close_frame(connection_instance, error_handle) != 0))
		{
			/* Codes_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
			(void)io_close(connection_instance->io);
			connection_set_state(connection_instance, CONNECTION_STATE_END);
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_213: [When passing the bytes to frame_codec fails, a CLOSE frame shall be sent and the state shall be set to DISCARDING.] */
			/* Codes_SRS_CONNECTION_01_055: [DISCARDING The DISCARDING state is a variant of the CLOSE SENT state where the close is triggered by an error.] */
			/* Codes_SRS_CONNECTION_01_010: [After writing this frame the peer SHOULD continue to read from the connection until it receives the partner’s close frame ] */
			connection_set_state(connection_instance, CONNECTION_STATE_DISCARDING);
		}

		error_destroy(error_handle);
	}
}

static ENDPOINT_INSTANCE* find_session_endpoint_by_outgoing_channel(CONNECTION_INSTANCE* connection, uint16_t outgoing_channel)
{
	uint32_t i;
	ENDPOINT_INSTANCE* result;

	for (i = 0; i < connection->endpoint_count; i++)
	{
		if (connection->endpoints[i]->outgoing_channel == outgoing_channel)
		{
			break;
		}
	}

	if (i == connection->endpoint_count)
	{
		result = NULL;
	}
	else
	{
		result = connection->endpoints[i];
	}

	return result;
}

static ENDPOINT_INSTANCE* find_session_endpoint_by_incoming_channel(CONNECTION_INSTANCE* connection, uint16_t incoming_channel)
{
	uint32_t i;
	ENDPOINT_INSTANCE* result;

	for (i = 0; i < connection->endpoint_count; i++)
	{
		if (connection->endpoints[i]->incoming_channel == incoming_channel)
		{
			break;
		}
	}

	if (i == connection->endpoint_count)
	{
		result = NULL;
	}
	else
	{
		result = connection->endpoints[i];
	}

	return result;
}

static int connection_byte_received(CONNECTION_INSTANCE* connection_instance, unsigned char b)
{
	int result;

	switch (connection_instance->connection_state)
	{
	default:
		result = __LINE__;
		break;

	/* Codes_SRS_CONNECTION_01_039: [START In this state a connection exists, but nothing has been sent or received. This is the state an implementation would be in immediately after performing a socket connect or socket accept.] */
	case CONNECTION_STATE_START:

	/* Codes_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
	case CONNECTION_STATE_HDR_SENT:
		if (b != amqp_header[connection_instance->header_bytes_received])
		{
			/* Codes_SRS_CONNECTION_01_089: [If the incoming and outgoing protocol headers do not match, both peers MUST close their outgoing stream] */
			io_close(connection_instance->io);
			connection_set_state(connection_instance, CONNECTION_STATE_END);
			result = __LINE__;
		}
		else
		{
			connection_instance->header_bytes_received++;
			if (connection_instance->header_bytes_received == sizeof(amqp_header))
			{
				LOG(consolelogger_log, LOG_LINE, "<- Header (AMQP 0.1.0.0)");

				connection_set_state(connection_instance, CONNECTION_STATE_HDR_EXCH);

				if (send_open_frame(connection_instance) != 0)
				{
					connection_set_state(connection_instance, CONNECTION_STATE_END);
				}
			}

			result = 0;
		}
		break;

	/* Codes_SRS_CONNECTION_01_040: [HDR RCVD In this state the connection header has been received from the peer but a connection header has not been sent.] */
	case CONNECTION_STATE_HDR_RCVD:

	/* Codes_SRS_CONNECTION_01_042: [HDR EXCH In this state the connection header has been sent to the peer and a connection header has been received from the peer.] */
	/* we should not really get into this state, but just in case, we would treat that in the same way as HDR_RCVD */
	case CONNECTION_STATE_HDR_EXCH:

	/* Codes_SRS_CONNECTION_01_045: [OPEN RCVD In this state the connection headers have been exchanged. An open frame has been received from the peer but an open frame has not been sent.] */
	case CONNECTION_STATE_OPEN_RCVD:

	/* Codes_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
	case CONNECTION_STATE_OPEN_SENT:

	/* Codes_SRS_CONNECTION_01_048: [OPENED In this state the connection header and the open frame have been both sent and received.] */
	case CONNECTION_STATE_OPENED:
		/* Codes_SRS_CONNECTION_01_212: [After the initial handshake has been done all bytes received from the io instance shall be passed to the frame_codec for decoding by calling frame_codec_receive_bytes.] */
		if (frame_codec_receive_bytes(connection_instance->frame_codec, &b, 1) != 0)
		{
			/* Codes_SRS_CONNECTION_01_218: [The error amqp:internal-error shall be set in the error.condition field of the CLOSE frame.] */
			/* Codes_SRS_CONNECTION_01_219: [The error description shall be set to an implementation defined string.] */
			close_connection_with_error(connection_instance, "amqp:internal-error", "connection_byte_received::frame_codec_receive_bytes failed");
			result = __LINE__;
		}
		else
		{
			result = 0;
		}

		break;
	}

	return result;
}

static void connection_on_bytes_received(void* context, const void* buffer, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++)
	{
		if (connection_byte_received((CONNECTION_INSTANCE*)context, ((unsigned char*)buffer)[i]) != 0)
		{
			break;
		}
	}
}

static void connection_on_io_state_changed(void* context, IO_STATE new_io_state, IO_STATE previous_io_state)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)context;
	switch (new_io_state)
	{
	default:
	case IO_STATE_NOT_OPEN:
		break;

	case IO_STATE_ERROR:
		/* Codes_SRS_CONNECTION_01_202: [If the io notifies the connection instance of an IO_STATE_ERROR state the connection shall be closed and the state set to END.] */
		io_close(connection_instance->io);
		connection_set_state(connection_instance, CONNECTION_STATE_END);
		break;

	case IO_STATE_OPENING:
		break;
	case IO_STATE_OPEN:
		/* Codes_SRS_CONNECTION_01_084: [The connection_instance state machine implementing the protocol requirements shall be run as part of connection_dowork.] */
		switch (connection_instance->connection_state)
		{
		default:
			break;

		case CONNECTION_STATE_START:
			if (new_io_state == IO_STATE_OPEN)
			{
				/* Codes_SRS_CONNECTION_01_086: [Prior to sending any frames on a connection_instance, each peer MUST start by sending a protocol header that indicates the protocol version used on the connection_instance.] */
				/* Codes_SRS_CONNECTION_01_091: [The AMQP peer which acted in the role of the TCP client (i.e. the peer that actively opened the connection_instance) MUST immediately send its outgoing protocol header on establishment of the TCP connection_instance.] */
				(void)send_header(connection_instance);
			}
			break;

		case CONNECTION_STATE_HDR_SENT:
		case CONNECTION_STATE_OPEN_SENT:
		case CONNECTION_STATE_OPENED:
			break;

		case CONNECTION_STATE_HDR_EXCH:
			/* Codes_SRS_CONNECTION_01_002: [Each AMQP connection_instance begins with an exchange of capabilities and limitations, including the maximum frame size.] */
			/* Codes_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection_instance and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
			/* Codes_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
			if (send_open_frame(connection_instance) != 0)
			{
				connection_set_state(connection_instance, CONNECTION_STATE_END);
			}
			break;

		case CONNECTION_STATE_OPEN_RCVD:
			break;
		}
	}
}

static void on_empty_amqp_frame_received(void* context, uint16_t channel)
{
}

static void on_amqp_frame_received(void* context, uint16_t channel, AMQP_VALUE performative, const unsigned char* payload_bytes, uint32_t payload_size)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)context;

	if (connection_instance->is_io_open)
	{
		switch (connection_instance->connection_state)
		{
		default:
			if (performative == NULL)
			{
				/* Codes_SRS_CONNECTION_01_223: [If the on_endpoint_frame_received is called with a NULL performative then the connection shall be closed with the error condition amqp:internal-error and an implementation defined error description.] */
				close_connection_with_error(connection_instance, "amqp:internal-error", "connection_endpoint_frame_received::NULL performative");
			}
			else
			{
				AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
				uint64_t performative_ulong;

				log_incoming_frame(performative);

				if (is_open_type_by_descriptor(descriptor))
				{
					if (channel != 0)
					{
						/* Codes_SRS_CONNECTION_01_006: [The open frame can only be sent on channel 0.] */
						/* Codes_SRS_CONNECTION_01_222: [If an Open frame is received in a manner violating the ISO specification, the connection shall be closed with condition amqp:not-allowed and description being an implementation defined string.] */
						close_connection_with_error(connection_instance, "amqp:not-allowed", "OPEN frame received on a channel that is not 0");
					}

					if (connection_instance->connection_state == CONNECTION_STATE_OPENED)
					{
						/* Codes_SRS_CONNECTION_01_239: [If an Open frame is received in the Opened state the connection shall be closed with condition amqp:illegal-state and description being an implementation defined string.] */
						close_connection_with_error(connection_instance, "amqp:illegal-state", "OPEN frame received in the OPENED state");
					}
					else if ((connection_instance->connection_state == CONNECTION_STATE_OPEN_SENT) ||
							 (connection_instance->connection_state == CONNECTION_STATE_HDR_EXCH))
					{
						OPEN_HANDLE open_handle;
						if (amqpvalue_get_open(performative, &open_handle) != 0)
						{
							/* Codes_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
							/* Codes_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
							close_connection_with_error(connection_instance, "amqp:invalid-field", "connection_endpoint_frame_received::failed parsing OPEN frame");
						}
						else
						{
							if ((open_get_max_frame_size(open_handle, &connection_instance->remote_max_frame_size) != 0) ||
								/* Codes_SRS_CONNECTION_01_167: [Both peers MUST accept frames of up to 512 (MIN-MAX-FRAME-SIZE) octets.] */
								(connection_instance->remote_max_frame_size < 512))
							{
								/* Codes_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
								/* Codes_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
								close_connection_with_error(connection_instance, "amqp:invalid-field", "connection_endpoint_frame_received::failed parsing OPEN frame");
							}
							else
							{
								if (connection_instance->connection_state == CONNECTION_STATE_OPEN_SENT)
								{
									connection_set_state(connection_instance, CONNECTION_STATE_OPENED);
								}
								else
								{
									if (send_open_frame(connection_instance) != 0)
									{
										connection_set_state(connection_instance, CONNECTION_STATE_END);
									}
									else
									{
										connection_set_state(connection_instance, CONNECTION_STATE_OPENED);
									}
								}
							}

							open_destroy(open_handle);
						}
					}
					else
					{

					}
				}
				else if (is_close_type_by_descriptor(descriptor))
				{
					/* Codes_SRS_CONNECTION_01_012: [A close frame MAY be received on any channel up to the maximum channel number negotiated in open.] */
					/* Codes_SRS_CONNECTION_01_242: [The connection module shall accept CLOSE frames even if they have extra payload bytes besides the Close performative.] */

					/* Codes_SRS_CONNECTION_01_225: [HDR_RCVD HDR OPEN] */
					if ((connection_instance->connection_state == CONNECTION_STATE_HDR_RCVD) ||
						/* Codes_SRS_CONNECTION_01_227: [HDR_EXCH OPEN OPEN] */
						(connection_instance->connection_state == CONNECTION_STATE_HDR_EXCH) ||
						/* Codes_SRS_CONNECTION_01_228: [OPEN_RCVD OPEN *] */
						(connection_instance->connection_state == CONNECTION_STATE_OPEN_RCVD) ||
						/* Codes_SRS_CONNECTION_01_235: [CLOSE_SENT - * TCP Close for Write] */
						(connection_instance->connection_state == CONNECTION_STATE_CLOSE_SENT) ||
						/* Codes_SRS_CONNECTION_01_236: [DISCARDING - * TCP Close for Write] */
						(connection_instance->connection_state == CONNECTION_STATE_DISCARDING))
					{
						io_close(connection_instance->io);
					}
					else
					{
						CLOSE_HANDLE close_handle;

						/* Codes_SRS_CONNECTION_01_012: [A close frame MAY be received on any channel up to the maximum channel number negotiated in open.] */
						if (channel > connection_instance->channel_max)
						{
							close_connection_with_error(connection_instance, "amqp:invalid-field", "connection_endpoint_frame_received::failed parsing CLOSE frame");
						}
						else
						{
							if (amqpvalue_get_close(performative, &close_handle) != 0)
							{
								close_connection_with_error(connection_instance, "amqp:invalid-field", "connection_endpoint_frame_received::failed parsing CLOSE frame");
							}
							else
							{
								close_destroy(close_handle);

								connection_set_state(connection_instance, CONNECTION_STATE_CLOSE_RCVD);

								(void)send_close_frame(connection_instance, NULL);
								/* Codes_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
								(void)io_close(connection_instance->io);

								connection_set_state(connection_instance, CONNECTION_STATE_END);
							}
						}
					}
				}
				else
				{
					amqpvalue_get_ulong(descriptor, &performative_ulong);

					switch (performative_ulong)
					{
					default:
						LOG(consolelogger_log, LOG_LINE, "Bad performative: %02x", performative);
						break;

					case AMQP_BEGIN:
					{
						ENDPOINT_INSTANCE* session_endpoint = find_session_endpoint_by_outgoing_channel(connection_instance, 0);
						if (session_endpoint == NULL)
						{
							/* error */
						}
						else
						{
							session_endpoint->incoming_channel = channel;
							session_endpoint->on_endpoint_frame_received(session_endpoint->callback_context, performative, payload_size, payload_bytes);
						}

						break;
					}

					case AMQP_FLOW:
					case AMQP_TRANSFER:
					case AMQP_DISPOSITION:
					case AMQP_END:
					case AMQP_ATTACH:
					case AMQP_DETACH:
					{
						ENDPOINT_INSTANCE* session_endpoint = find_session_endpoint_by_incoming_channel(connection_instance, channel);
						if (session_endpoint == NULL)
						{
							/* error */
						}
						else
						{
							session_endpoint->on_endpoint_frame_received(session_endpoint->callback_context, performative, payload_size, payload_bytes);
						}

						break;
					}
					}
				}
			}
			break;

		/* Codes_SRS_CONNECTION_01_224: [START HDR HDR] */
		case CONNECTION_STATE_START:
		/* Codes_SRS_CONNECTION_01_226: [HDR_SENT OPEN HDR] */
		case CONNECTION_STATE_HDR_SENT:
		/* Codes_SRS_CONNECTION_01_230: [OPEN_PIPE ** HDR] */
		case CONNECTION_STATE_OPEN_PIPE:
		/* Codes_SRS_CONNECTION_01_232: [OC_PIPE - HDR TCP Close for Write] */
		case CONNECTION_STATE_OC_PIPE:
		/* Codes_SRS_CONNECTION_01_234: [CLOSE_RCVD * - TCP Close for Read] */
		case CONNECTION_STATE_CLOSE_RCVD:
		/* Codes_SRS_CONNECTION_01_237: [END - - TCP Close] */
		case CONNECTION_STATE_END:
			io_close(connection_instance->io);
			break;
		}
	}
}

static void frame_codec_error(void* context)
{
}

static void amqp_frame_codec_error(void* context)
{
}

/* Codes_SRS_CONNECTION_01_001: [connection_create shall open a new connection to a specified host/port.] */
CONNECTION_HANDLE connection_create(IO_HANDLE io, const char* hostname, const char* container_id)
{
	CONNECTION_INSTANCE* result;

	if ((io == NULL) ||
		(container_id == NULL))
	{
		/* Codes_SRS_CONNECTION_01_071: [If io or container_id is NULL, connection_create shall return NULL.] */
		result = NULL;
	}
	else
	{
		result = (CONNECTION_INSTANCE*)amqpalloc_malloc(sizeof(CONNECTION_INSTANCE));
		/* Codes_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
		if (result != NULL)
		{
			result->io = io;

			/* Codes_SRS_CONNECTION_01_082: [connection_create shall allocate a new frame_codec instance to be used for frame encoding/decoding.] */
			result->frame_codec = frame_codec_create(result->io, frame_codec_error, result, consolelogger_log);
			if (result->frame_codec == NULL)
			{
				/* Codes_SRS_CONNECTION_01_083: [If frame_codec_create fails then connection_create shall return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->amqp_frame_codec = amqp_frame_codec_create(result->frame_codec, on_amqp_frame_received, on_empty_amqp_frame_received, amqp_frame_codec_error, result);
				if (result->amqp_frame_codec == NULL)
				{
					/* Codes_SRS_CONNECTION_01_108: [If amqp_frame_codec_create fails, connection_create shall return NULL.] */
					frame_codec_destroy(result->frame_codec);
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					if (hostname != NULL)
					{
						result->host_name = (char*)amqpalloc_malloc(strlen(hostname) + 1);
						if (result->host_name == NULL)
						{
							/* Codes_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
							amqp_frame_codec_destroy(result->amqp_frame_codec);
							frame_codec_destroy(result->frame_codec);
							amqpalloc_free(result);
							result = NULL;
						}
						else
						{
							strcpy(result->host_name, hostname);
						}
					}
					else
					{
						result->host_name = NULL;
					}

					if (result != NULL)
					{
						result->container_id = (char*)amqpalloc_malloc(strlen(container_id) + 1);
						if (result->container_id == NULL)
						{
							/* Codes_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
							amqpalloc_free(result->host_name);
							amqp_frame_codec_destroy(result->amqp_frame_codec);
							frame_codec_destroy(result->frame_codec);
							amqpalloc_free(result);
							result = NULL;
						}
						else
						{
							strcpy(result->container_id, container_id);

							/* Codes_SRS_CONNECTION_01_173: [<field name="max-frame-size" type="uint" default="4294967295"/>] */
							result->max_frame_size = 4294967295;
							/* Codes: [<field name="channel-max" type="ushort" default="65535"/>] */
							result->channel_max = 65535;

							/* Codes_SRS_CONNECTION_01_175: [<field name="idle-time-out" type="milliseconds"/>] */
							/* Codes_SRS_CONNECTION_01_192: [A value of zero is the same as if it was not set (null).] */
							result->idle_timeout = 0;

							result->header_bytes_received = 0;
							result->endpoint_count = 0;
							result->endpoints = NULL;
							result->is_io_open = 0;
							result->remote_max_frame_size = 512;

							/* Mark that settings have not yet been set by the user */
							result->idle_timeout_specified = 0;

							/* Codes_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
							connection_set_state(result, CONNECTION_STATE_START);
						}
					}
				}
			}
		}
	}

	return result;
}

void connection_destroy(CONNECTION_HANDLE connection)
{
	/* Codes_SRS_CONNECTION_01_079: [If handle is NULL, connection_destroy shall do nothing.] */
	if (connection != NULL)
	{
		/* Codes_SRS_CONNECTION_01_073: [connection_destroy shall free all resources associated with a connection.] */
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
		amqp_frame_codec_destroy(connection_instance->amqp_frame_codec);
		frame_codec_destroy(connection_instance->frame_codec);

		amqpalloc_free(connection_instance->host_name);
		amqpalloc_free(connection_instance->container_id);

		/* Codes_SRS_CONNECTION_01_074: [connection_destroy shall close the socket connection.] */
		amqpalloc_free(connection_instance);
	}
}

int connection_set_max_frame_size(CONNECTION_HANDLE connection, uint32_t max_frame_size)
{
	int result;

	/* Codes_SRS_CONNECTION_01_163: [If connection is NULL, connection_set_max_frame_size shall fail and return a non-zero value.] */
	if ((connection == NULL) ||
		/* Codes_SRS_CONNECTION_01_150: [If the max_frame_size is invalid then connection_set_max_frame_size shall fail and return a non-zero value.] */
		/* Codes_SRS_CONNECTION_01_167: [Both peers MUST accept frames of up to 512 (MIN-MAX-FRAME-SIZE) octets.] */
		(max_frame_size < 512))
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_157: [If connection_set_max_frame_size is called after the initial Open frame has been sent, it shall fail and return a non-zero value.] */
		if (connection_instance->connection_state != CONNECTION_STATE_START)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_148: [connection_set_max_frame_size shall set the max_frame_size associated with a connection.] */
			/* Codes_SRS_CONNECTION_01_164: [If connection_set_max_frame_size fails, the previous max_frame_size setting shall be retained.] */
			connection_instance->max_frame_size = max_frame_size;

			/* Codes_SRS_CONNECTION_01_149: [On success connection_set_max_frame_size shall return 0.] */
			result = 0;
		}
	}

	return result;
}

int connection_get_max_frame_size(CONNECTION_HANDLE connection, uint32_t* max_frame_size)
{
	int result;

	/* Codes_SRS_CONNECTION_01_170: [If connection or max_frame_size is NULL, connection_get_max_frame_size shall fail and return a non-zero value.] */
	if ((connection == NULL) ||
		(max_frame_size == NULL))
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_168: [connection_get_max_frame_size shall return in the max_frame_size argument the current max frame size setting.] */
		*max_frame_size = connection_instance->max_frame_size;

		/* Codes_SRS_CONNECTION_01_169: [On success, connection_get_max_frame_size shall return 0.] */
		result = 0;
	}

	return result;
}

int connection_set_channel_max(CONNECTION_HANDLE connection, uint16_t channel_max)
{
	int result;

	/* Codes_SRS_CONNECTION_01_181: [If connection is NULL then connection_set_channel_max shall fail and return a non-zero value.] */
	if (connection == NULL)
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_156: [If connection_set_channel_max is called after the initial Open frame has been sent, it shall fail and return a non-zero value.] */
		if (connection_instance->connection_state != CONNECTION_STATE_START)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_153: [connection_set_channel_max shall set the channel_max associated with a connection.] */
			/* Codes_SRS_CONNECTION_01_165: [If connection_set_channel_max fails, the previous channel_max setting shall be retained.] */
			connection_instance->channel_max = channel_max;

			/* Codes_SRS_CONNECTION_01_154: [On success connection_set_channel_max shall return 0.] */
			result = 0;
		}
	}

	return result;
}

int connection_get_channel_max(CONNECTION_HANDLE connection, uint16_t* channel_max)
{
	int result;

	/* Codes_SRS_CONNECTION_01_184: [If connection or channel_max is NULL, connection_get_channel_max shall fail and return a non-zero value.] */
	if ((connection == NULL) ||
		(channel_max == NULL))
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_182: [connection_get_channel_max shall return in the channel_max argument the current channel_max setting.] */
		*channel_max = connection_instance->channel_max;

		/* Codes_SRS_CONNECTION_01_183: [On success, connection_get_channel_max shall return 0.] */
		result = 0;
	}

	return result;
}

int connection_set_idle_timeout(CONNECTION_HANDLE connection, milliseconds idle_timeout)
{
	int result;

	/* Codes_SRS_CONNECTION_01_191: [If connection is NULL, connection_set_idle_timeout shall fail and return a non-zero value.] */
	if (connection == NULL)
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_158: [If connection_set_idle_timeout is called after the initial Open frame has been sent, it shall fail and return a non-zero value.] */
		if (connection_instance->connection_state != CONNECTION_STATE_START)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_159: [connection_set_idle_timeout shall set the idle_timeout associated with a connection.] */
			/* Codes_SRS_CONNECTION_01_166: [If connection_set_idle_timeout fails, the previous idle_timeout setting shall be retained.] */
			connection_instance->idle_timeout = idle_timeout;
			connection_instance->idle_timeout_specified = true;

			/* Codes_SRS_CONNECTION_01_160: [On success connection_set_idle_timeout shall return 0.] */
			result = 0;
		}
	}

	return result;
}

int connection_get_idle_timeout(CONNECTION_HANDLE connection, milliseconds* idle_timeout)
{
	int result;

	/* Codes_SRS_CONNECTION_01_190: [If connection or idle_timeout is NULL, connection_get_idle_timeout shall fail and return a non-zero value.] */
	if ((connection == NULL) ||
		(idle_timeout == NULL))
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_188: [connection_get_idle_timeout shall return in the idle_timeout argument the current idle_timeout setting.] */
		*idle_timeout = connection_instance->idle_timeout;

		/* Codes_SRS_CONNECTION_01_189: [On success, connection_get_idle_timeout shall return 0.] */
		result = 0;
	}

	return result;
}

int connection_get_remote_max_frame_size(CONNECTION_HANDLE connection, uint32_t* remote_max_frame_size)
{
	int result;

	if ((connection == NULL) ||
		(remote_max_frame_size == NULL))
	{
		result = __LINE__;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		*remote_max_frame_size = connection_instance->remote_max_frame_size;

		result = 0;
	}

	return result;
}

void connection_dowork(CONNECTION_HANDLE connection)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

	/* Codes_SRS_CONNECTION_01_078: [If handle is NULL, connection_dowork shall do nothing.] */
	if (connection_instance != NULL)
	{
		if (!connection_instance->is_io_open)
		{
			/* Codes_: [If the io has not been opened before, connection_dowork shall attempt to open the io by calling io_open.] */
			if (io_open(connection_instance->io, connection_on_bytes_received, connection_on_io_state_changed, connection_instance) != 0)
			{
				/* Codes_SRS_CONNECTION_01_204: [If io_open_fails, no more work shall be done by connection_dowork and the connection shall be considered in the END state.] */
				connection_set_state(connection_instance, CONNECTION_STATE_END);
			}
			else
			{
				connection_set_state(connection_instance, CONNECTION_STATE_START);
				connection_instance->is_io_open = 1;
			}
		}

		/* Codes_SRS_CONNECTION_01_076: [connection_dowork shall schedule the underlying IO interface to do its work by calling io_dowork.] */
		io_dowork(connection_instance->io);
	}
}

ENDPOINT_HANDLE connection_create_endpoint(CONNECTION_HANDLE connection, ON_ENDPOINT_FRAME_RECEIVED on_endpoint_frame_received, ON_CONNECTION_STATE_CHANGED on_connection_state_changed, void* context)
{
	ENDPOINT_INSTANCE* result;

	/* Codes_SRS_CONNECTION_01_113: [If connection, on_endpoint_frame_received or on_connection_state_changed is NULL, connection_create_endpoint shall fail and return NULL.] */
	/* Codes_SRS_CONNECTION_01_193: [The context argument shall be allowed to be NULL.] */
	if ((connection == NULL) ||
		(on_endpoint_frame_received == NULL) ||
		(on_connection_state_changed == NULL))
	{
		result = NULL;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

		/* Codes_SRS_CONNECTION_01_115: [If no more endpoints can be created due to all channels being used, connection_create_endpoint shall fail and return NULL.] */
		if (connection_instance->endpoint_count >= connection_instance->channel_max)
		{
			result = NULL;
		}
		else
		{
			uint32_t i = 0;

			/* Codes_SRS_CONNECTION_01_128: [The lowest number outgoing channel shall be associated with the newly created endpoint.] */
			for (i = 0; i < connection_instance->endpoint_count; i++)
			{
				if (connection_instance->endpoints[i]->outgoing_channel > i)
				{
					/* found a gap in the sorted endpoint array */
					break;
				}
			}

			/* Codes_SRS_CONNECTION_01_127: [On success, connection_create_endpoint shall return a non-NULL handle to the newly created endpoint.] */
			result = amqpalloc_malloc(sizeof(ENDPOINT_INSTANCE));
			/* Codes_SRS_CONNECTION_01_196: [If memory cannot be allocated for the new endpoint, connection_create_endpoint shall fail and return NULL.] */
			if (result != NULL)
			{
				ENDPOINT_INSTANCE** new_endpoints;

				result->on_endpoint_frame_received = on_endpoint_frame_received;
				result->on_connection_state_changed = on_connection_state_changed;
				result->callback_context = context;
				result->outgoing_channel = i;
				result->connection = connection;

				/* Codes_SRS_CONNECTION_01_197: [The newly created endpoint shall be added to the endpoints list, so that it can be tracked.] */
				new_endpoints = (ENDPOINT_INSTANCE**)amqpalloc_realloc(connection_instance->endpoints, sizeof(ENDPOINT_INSTANCE*) * (connection_instance->endpoint_count + 1));
				if (new_endpoints == NULL)
				{
					/* Tests_SRS_CONNECTION_01_198: [If adding the endpoint to the endpoints list tracked by the connection fails, connection_create_endpoint shall fail and return NULL.] */
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					connection_instance->endpoints = new_endpoints;

					if (i < connection_instance->endpoint_count)
					{
						(void)memmove(&connection_instance->endpoints[i + 1], &connection_instance->endpoints[i], sizeof(ENDPOINT_INSTANCE*) * (connection_instance->endpoint_count - i));
					}

					connection_instance->endpoints[i] = result;
					connection_instance->endpoint_count++;

					/* Codes_SRS_CONNECTION_01_112: [connection_create_endpoint shall create a new endpoint that can be used by a session.] */
				}
			}
		}
	}

	return result;
}

/* Codes_SRS_CONNECTION_01_129: [connection_destroy_endpoint shall free all resources associated with an endpoint created by connection_create_endpoint.] */
void connection_destroy_endpoint(ENDPOINT_HANDLE endpoint)
{
	if (endpoint != NULL)
	{
		ENDPOINT_INSTANCE* endpoint_instance = (ENDPOINT_INSTANCE*)endpoint;
		CONNECTION_INSTANCE* connection_instance = endpoint_instance->connection;
		size_t i;

		for (i = 0; i < connection_instance->endpoint_count; i++)
		{
			if (connection_instance->endpoints[i] == endpoint)
			{
				break;
			}
		}

		/* Codes_SRS_CONNECTION_01_130: [The outgoing channel associated with the endpoint shall be released by removing the endpoint from the endpoint list.] */
		/* Codes_SRS_CONNECTION_01_131: [Any incoming channel number associated with the endpoint shall be released.] */
		if (i < connection_instance->endpoint_count)
		{
			(void)memmove(connection_instance->endpoints + i, connection_instance->endpoints + i + 1, sizeof(ENDPOINT_INSTANCE*) * (connection_instance->endpoint_count - i - 1));

			ENDPOINT_INSTANCE** new_endpoints = (ENDPOINT_INSTANCE**)amqpalloc_realloc(connection_instance->endpoints, (connection_instance->endpoint_count - 1) * sizeof(ENDPOINT_INSTANCE*));
			if (new_endpoints != NULL)
			{
				connection_instance->endpoints = new_endpoints;
			}

			connection_instance->endpoint_count--;
		}

		amqpalloc_free(endpoint_instance);
	}
}

/* Codes_SRS_CONNECTION_01_247: [connection_encode_frame shall send a frame for a certain endpoint.] */
int connection_encode_frame(ENDPOINT_HANDLE endpoint, const AMQP_VALUE performative, PAYLOAD* payloads, size_t payload_count)
{
	int result;

	/* Codes_SRS_CONNECTION_01_249: [If endpoint or performative are NULL, connection_encode_frame shall fail and return a non-zero value.] */
	if ((endpoint == NULL) ||
		(performative == NULL))
	{
		result = __LINE__;
	}
	else
	{
		ENDPOINT_INSTANCE* endpoint_instance = (ENDPOINT_INSTANCE*)endpoint;
		CONNECTION_INSTANCE* connection = endpoint_instance->connection;
		AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = connection->amqp_frame_codec;
		uint32_t payload_size = 0;
		size_t i;

		/* Codes_SRS_CONNECTION_01_254: [If connection_encode_frame is called before the connection is in the OPENED state, connection_encode_frame shall fail and return a non-zero value.] */
		if (connection->connection_state != CONNECTION_STATE_OPENED)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_255: [The payload size shall be computed based on all the payload chunks passed as argument in payloads.] */
			for (i = 0; i < payload_count; i++)
			{
				payload_size += payloads[i].length;
			}

			/* Codes_SRS_CONNECTION_01_250: [connection_encode_frame shall initiate the frame send by calling amqp_frame_codec_begin_encode_frame.] */
			/* Codes_SRS_CONNECTION_01_251: [The channel number passed to amqp_frame_codec_begin_encode_frame shall be the outgoing channel number associated with the endpoint by connection_create_endpoint.] */
			/* Codes_SRS_CONNECTION_01_252: [The performative passed to amqp_frame_codec_begin_encode_frame shall be the performative argument of connection_encode_frame.] */
			if (amqp_frame_codec_begin_encode_frame(amqp_frame_codec, endpoint_instance->outgoing_channel, performative, payload_size) != 0)
			{
				/* Codes_SRS_CONNECTION_01_253: [If amqp_frame_codec_begin_encode_frame or amqp_frame_codec_encode_payload_bytes fails, then connection_encode_frame shall fail and return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				for (i = 0; i < payload_count; i++)
				{
					/* Codes_SRS_CONNECTION_01_256: [Each payload passed in the payloads array shall be passed to amqp_frame_codec by calling amqp_frame_codec_encode_payload_bytes.] */
					if (amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, payloads[i].bytes, payloads[i].length) != 0)
					{
						break;
					}
				}

				if (i < payload_count)
				{
					/* Codes_SRS_CONNECTION_01_253: [If amqp_frame_codec_begin_encode_frame or amqp_frame_codec_encode_payload_bytes fails, then connection_encode_frame shall fail and return a non-zero value.] */
					result = __LINE__;
				}
				else
				{
					log_outgoing_frame(performative);

					/* Codes_SRS_CONNECTION_01_248: [On success it shall return 0.] */
					result = 0;
				}
			}
		}
	}

	return result;
}
