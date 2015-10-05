#include <stdlib.h>
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
	ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback;
	ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback;
	void* frame_received_callback_context;
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
	uint16_t frame_receive_channel;
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
		connection_instance->connection_state = CONNECTION_STATE_END;

		/* Codes_SRS_CONNECTION_01_105: [When io_send fails, connection_dowork shall return a non-zero value.] */
		result = __LINE__;
	}
	else
	{
		LOG(consolelogger_log, LOG_LINE, "-> Header (AMQP 0.1.0.0)");

		/* Codes_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
		connection_instance->connection_state = CONNECTION_STATE_HDR_SENT;
		result = 0;
	}

	return result;
}

static int send_open_frame(CONNECTION_INSTANCE* connection_instance)
{
	int result;

	/* Codes_SRS_CONNECTION_01_151: [The connection max_frame_size setting shall be passed down to the frame_codec when the Open frame is sent.] */
	if (frame_codec_set_max_frame_size(connection_instance->frame_codec, connection_instance->max_frame_size) != 0)
	{
		/* Codes_SRS_CONNECTION_01_207: [If frame_codec_set_max_frame_size fails the connection shall be closed and the state set to END.] */
		io_close(connection_instance->io);
		connection_instance->connection_state = CONNECTION_STATE_END;
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
			connection_instance->connection_state = CONNECTION_STATE_END;
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_137: [The max_frame_size connection setting shall be set in the open frame by using open_set_max_frame_size.] */
			if (open_set_max_frame_size(open_performative, connection_instance->max_frame_size) != 0)
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_instance->connection_state = CONNECTION_STATE_END;
				result = __LINE__;
			}
			/* Codes_SRS_CONNECTION_01_139: [The channel_max connection setting shall be set in the open frame by using open_set_channel_max.] */
			else if (open_set_channel_max(open_performative, connection_instance->channel_max) != 0)
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_instance->connection_state = CONNECTION_STATE_END;
				result = __LINE__;
			}
			/* Codes_SRS_CONNECTION_01_142: [If no idle_timeout value has been specified, no value shall be stamped in the open frame (no call to open_set_idle_time_out shall be made).] */
			else if ((connection_instance->idle_timeout_specified) &&
				/* Codes_SRS_CONNECTION_01_141: [If idle_timeout has been specified by a call to connection_set_idle_timeout, then that value shall be stamped in the open frame.] */
				(open_set_idle_time_out(open_performative, connection_instance->idle_timeout) != 0))
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_instance->connection_state = CONNECTION_STATE_END;
				result = __LINE__;
			}
			/* Codes_SRS_CONNECTION_01_136: [If no hostname value has been specified, no value shall be stamped in the open frame (no call to open_set_hostname shall be made).] */
			else if ((connection_instance->host_name != NULL) &&
				/* Codes_SRS_CONNECTION_01_135: [If hostname has been specified by a call to connection_set_hostname, then that value shall be stamped in the open frame.] */
				(open_set_hostname(open_performative, connection_instance->host_name) != 0))
			{
				/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
				io_close(connection_instance->io);
				connection_instance->connection_state = CONNECTION_STATE_END;
				result = __LINE__;
			}
			else
			{
				AMQP_VALUE open_performative_value = amqpvalue_create_open(open_performative);
				if (open_performative_value == NULL)
				{
					/* Codes_SRS_CONNECTION_01_208: [If the open frame cannot be constructed, the connection shall be closed and set to the END state.] */
					io_close(connection_instance->io);
					connection_instance->connection_state = CONNECTION_STATE_END;
					result = __LINE__;
				}
				else
				{
					/* Codes_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.] */
					/* Codes_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
					/* Codes_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
					/* Codes_SRS_CONNECTION_01_205: [Sending the AMQP OPEN frame shall be done by calling amqp_frame_codec_begin_encode_frame with channel number 0, the actual performative payload and 0 as payload_size.] */
					if (amqp_frame_codec_begin_encode_frame(connection_instance->amqp_frame_codec, 0, open_performative_value, 0) != 0)
					{
						/* Codes_SRS_CONNECTION_01_206: [If sending the frame fails, the connection shall be closed and state set to END.] */
						io_close(connection_instance->io);
						connection_instance->connection_state = CONNECTION_STATE_END;
						result = __LINE__;
					}
					else
					{
						LOG(consolelogger_log, LOG_LINE, "-> [OPEN]");

						/* Codes_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
						connection_instance->connection_state = CONNECTION_STATE_OPEN_SENT;
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
		if (close_set_error(close_performative, error_handle) != 0)
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
				if (amqp_frame_codec_begin_encode_frame(connection_instance->amqp_frame_codec, 0, close_performative_value, 0) != 0)
				{
					result = __LINE__;
				}
				else
				{
					LOG(consolelogger_log, LOG_LINE, "-> [CLOSE]");
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
		connection_instance->connection_state = CONNECTION_STATE_END;
	}
	else
	{
		/* Codes_SRS_CONNECTION_01_219: [The error description shall be set to an implementation defined string.] */
		if ((error_set_description(error_handle, description) != 0) ||
			(send_close_frame(connection_instance, error_handle) != 0))
		{
			/* Codes_SRS_CONNECTION_01_214: [If the close frame cannot be constructed or sent, the connection shall be closed and set to the END state.] */
			(void)io_close(connection_instance->io);
			connection_instance->connection_state = CONNECTION_STATE_END;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_213: [When passing the bytes to frame_codec fails, a CLOSE frame shall be sent and the state shall be set to DISCARDING.] */
			connection_instance->connection_state = CONNECTION_STATE_DISCARDING;
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
			connection_instance->connection_state = CONNECTION_STATE_END;
			result = __LINE__;
		}
		else
		{
			connection_instance->header_bytes_received++;
			if (connection_instance->header_bytes_received == sizeof(amqp_header))
			{
				LOG(consolelogger_log, LOG_LINE, "<- Header (AMQP 0.1.0.0)");

				if (connection_instance->connection_state == CONNECTION_STATE_START)
				{
					if (send_header(connection_instance) != 0)
					{
						io_destroy(connection_instance->io);
						connection_instance->io = NULL;
						connection_instance->connection_state = CONNECTION_STATE_END;
						result = __LINE__;
					}
					else
					{
						result = send_open_frame(connection_instance);
					}
				}
				else
				{
					result = send_open_frame(connection_instance);
				}
			}
			else
			{
				result = 0;
			}
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

static void connection_receive_callback(void* context, const void* buffer, size_t size)
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

static void connection_empty_frame_received(void* context, uint16_t channel)
{
}

static void connection_frame_received(void* context, uint16_t channel, AMQP_VALUE performative, uint32_t payload_size)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
	uint64_t performative_ulong;

	if (is_open_type_by_descriptor(descriptor))
	{
		LOG(consolelogger_log, 0, "<- [OPEN] ");
		//LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(performative));

		if ((connection_instance->connection_state == CONNECTION_STATE_OPEN_SENT) ||
			(connection_instance->connection_state == CONNECTION_STATE_HDR_EXCH))
		{
			OPEN_HANDLE open_handle;
			if (amqpvalue_get_open(performative, &open_handle) != 0)
			{
				/* Codes_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
				/* Codes_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
				close_connection_with_error(connection_instance, "amqp:invalid-field", "connection_frame_received::failed parsing OPEN frame");
			}
			else
			{
				if ((open_get_max_frame_size(open_handle, &connection_instance->remote_max_frame_size) != 0) ||
					/* Codes_SRS_CONNECTION_01_167: [Both peers MUST accept frames of up to 512 (MIN-MAX-FRAME-SIZE) octets.] */
					(connection_instance->remote_max_frame_size < 512))
				{
					/* Codes_SRS_CONNECTION_01_143: [If any of the values in the received open frame are invalid then the connection shall be closed.] */
					/* Codes_SRS_CONNECTION_01_220: [The error amqp:invalid-field shall be set in the error.condition field of the CLOSE frame.] */
					close_connection_with_error(connection_instance, "amqp:invalid-field", "connection_frame_received::failed parsing OPEN frame");
				}
				else
				{
					if (connection_instance->connection_state == CONNECTION_STATE_OPEN_SENT)
					{
						connection_instance->connection_state = CONNECTION_STATE_OPENED;
					}
					else
					{
						connection_instance->connection_state = CONNECTION_STATE_OPEN_RCVD;
					}
				}
			}
		}
		else
		{

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

		case AMQP_CLOSE:
		{
			const char* error = NULL;
			AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
			AMQP_VALUE error_value = amqpvalue_get_list_item(described_value, 0);
			AMQP_VALUE error_described_value = amqpvalue_get_described_value(error_value);
			AMQP_VALUE error_description_value = amqpvalue_get_list_item(error_described_value, 1);
			amqpvalue_get_string(error_description_value, &error);

			LOG(consolelogger_log, LOG_LINE, "<- [CLOSE:%s]", error);
			break;
		}

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
				session_endpoint->frame_received_callback(session_endpoint->frame_received_callback_context, performative, payload_size);
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
				session_endpoint->frame_received_callback(session_endpoint->frame_received_callback_context, performative, payload_size);
				connection_instance->frame_receive_channel = channel;
			}

			break;
		}
		}
	}
}

static void connection_frame_payload_bytes_received(void* context, const unsigned char* payload_bytes, uint32_t byte_count)
{
	CONNECTION_INSTANCE* connection_instance = context;
	ENDPOINT_INSTANCE* endpoint_instance = find_session_endpoint_by_incoming_channel(connection_instance, connection_instance->frame_receive_channel);
	if (endpoint_instance == NULL)
	{
		/* error */
	}
	else
	{
		endpoint_instance->frame_payload_bytes_received_callback(context, payload_bytes, byte_count);
	}
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
			result->frame_codec = frame_codec_create(result->io, consolelogger_log);
			if (result->frame_codec == NULL)
			{
				/* Codes_SRS_CONNECTION_01_083: [If frame_codec_create fails then connection_create shall return NULL.] */
				io_destroy(result->io);
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->amqp_frame_codec = amqp_frame_codec_create(result->frame_codec, connection_frame_received, connection_empty_frame_received, connection_frame_payload_bytes_received, result);
				if (result->amqp_frame_codec == NULL)
				{
					/* Codes_SRS_CONNECTION_01_108: [If amqp_frame_codec_create fails, connection_create shall return NULL.] */
					frame_codec_destroy(result->frame_codec);
					io_destroy(result->io);
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
							io_destroy(result->io);
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
							io_destroy(result->io);
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

							/* Codes_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
							result->connection_state = CONNECTION_STATE_START;
							result->header_bytes_received = 0;
							result->endpoint_count = 0;
							result->endpoints = NULL;
							result->is_io_open = 0;
							result->remote_max_frame_size = 512;

							/* Mark that settings have not yet been set by the user */
							result->idle_timeout_specified = 0;
						}
					}
				}
			}
		}
	}

	return result;
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

void connection_destroy(CONNECTION_HANDLE connection)
{
	/* Codes_SRS_CONNECTION_01_079: [If handle is NULL, connection_destroy shall do nothing.] */
	if (connection != NULL)
	{
		/* Codes_SRS_CONNECTION_01_073: [connection_destroy shall free all resources associated with a connection.] */
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
		amqp_frame_codec_destroy(connection_instance->amqp_frame_codec);
		frame_codec_destroy(connection_instance->frame_codec);

		/* Codes_SRS_CONNECTION_01_074: [connection_destroy shall close the socket connection.] */
		io_destroy(connection_instance->io);
		amqpalloc_free(connection_instance);
	}
}

void connection_dowork(CONNECTION_HANDLE connection)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

	/* Codes_SRS_CONNECTION_01_078: [If handle is NULL, connection_dowork shall do nothing.] */
	if (connection_instance != NULL)
	{
		if (!connection_instance->is_io_open)
		{
			/* Codes_SRS_CONNECTION_01_203: [If the io has not been open before is IO_STATE_NOT_OPEN, connection_dowork shall attempt to open the io by calling io_open.] */
			if (io_open(connection_instance->io, connection_receive_callback, connection_instance) != 0)
			{
				/* Codes_SRS_CONNECTION_01_204: [If io_open_fails, no more work shall be done by connection_dowork and the connection shall be consideren in the END state.] */
				connection_instance->connection_state = CONNECTION_STATE_END;
			}
			else
			{
				connection_instance->connection_state = CONNECTION_STATE_START;
				connection_instance->is_io_open = 1;
			}
		}

		/* Codes_SRS_CONNECTION_01_201: [The IO interface state shall be queried by using io_get_state.] */
		IO_STATE io_state = io_get_state(connection_instance->io);

		switch (io_state)
		{
		default:
		case IO_STATE_NOT_OPEN:
			break;

		case IO_STATE_ERROR:
			/* Codes_SRS_CONNECTION_01_202: [If the io_get_state call returns IO_STATE_ERROR the connection shall be closed and the state set to END.] */
			io_close(connection_instance->io);
			connection_instance->connection_state = CONNECTION_STATE_END;
			break;

		case IO_STATE_NOT_READY:
			break;
		/* Codes_SRS_CONNECTION_01_200: [The connection state machine processing shall only be done when the IO interface state is ready.] */
		case IO_STATE_READY:
			/* Codes_SRS_CONNECTION_01_084: [The connection_instance state machine implementing the protocol requirements shall be run as part of connection_dowork.] */
			switch (connection_instance->connection_state)
			{
			default:
				break;

			case CONNECTION_STATE_START:
				if (io_state == IO_STATE_READY)
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
				if (io_get_state(connection_instance->io) == IO_STATE_READY)
				{
					/* Codes_SRS_CONNECTION_01_002: [Each AMQP connection_instance begins with an exchange of capabilities and limitations, including the maximum frame size.] */
					/* Codes_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection_instance and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
					/* Codes_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
					if (send_open_frame(connection) != 0)
					{
						io_destroy(connection_instance->io);
						connection_instance->io = NULL;
						connection_instance->connection_state = CONNECTION_STATE_END;
					}
				}
				break;

			case CONNECTION_STATE_OPEN_RCVD:
				break;
			}
		}

		/* Codes_SRS_CONNECTION_01_076: [connection_dowork shall schedule the underlying IO interface to do its work by calling io_dowork.] */
		io_dowork(connection_instance->io);
	}
}

int connection_get_state(CONNECTION_HANDLE connection, CONNECTION_STATE* connection_state)
{
	int result;
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

	if (connection_instance == NULL)
	{
		result = __LINE__;
	}
	else
	{
		*connection_state = connection_instance->connection_state;
		result = 0;
	}

	return result;
}

ENDPOINT_HANDLE connection_create_endpoint(CONNECTION_HANDLE connection, ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback, void* context)
{
	ENDPOINT_INSTANCE* result;

	/* Codes_SRS_CONNECTION_01_113: [If connection, frame_received_callback or frame_payload_bytes_received_callback is NULL, connection_create_endpoint shall fail and return NULL.] */
	/* Codes_SRS_CONNECTION_01_193: [The context argument shall be allowed to be NULL.] */
	if ((connection == NULL) ||
		(frame_received_callback == NULL) ||
		(frame_payload_bytes_received_callback == NULL))
	{
		result = NULL;
	}
	else
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
		uint16_t channel_no = 0;

		while (channel_no < connection_instance->channel_max)
		{
			break;
			channel_no++;
		}

		/* Codes_SRS_CONNECTION_01_127: [On success, connection_create_endpoint shall return a non-NULL handle to the newly created endpoint.] */
		result = amqpalloc_malloc(sizeof(ENDPOINT_INSTANCE));
		/* Codes_SRS_CONNECTION_01_196: [If memory cannot be allocated for the new endpoint, connection_create_endpoint shall fail and return NULL.] */
		if (result != NULL)
		{
			ENDPOINT_INSTANCE** new_endpoints;

			result->frame_received_callback = frame_received_callback;
			result->frame_payload_bytes_received_callback = frame_payload_bytes_received_callback;
			result->frame_received_callback_context = context;
			result->outgoing_channel = channel_no;
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
				connection_instance->endpoints[connection_instance->endpoint_count] = result;
				connection_instance->endpoint_count++;

				/* Codes_SRS_CONNECTION_01_112: [connection_create_endpoint shall create a new endpoint that can be used by a session.] */
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

int connection_begin_encode_frame(ENDPOINT_HANDLE endpoint, const AMQP_VALUE performative, uint32_t payload_size)
{
	int result;

	if (endpoint == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENDPOINT_INSTANCE* endpoint_instance = (ENDPOINT_INSTANCE*)endpoint;
		CONNECTION_INSTANCE* connection = endpoint_instance->connection;
		AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = connection->amqp_frame_codec;

		if (amqp_frame_codec_begin_encode_frame(amqp_frame_codec, endpoint_instance->outgoing_channel, performative, payload_size) != 0)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

int connection_encode_payload_bytes(ENDPOINT_HANDLE endpoint, const unsigned char* bytes, uint32_t count)
{
	int result;

	if (endpoint == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENDPOINT_INSTANCE* endpoint_instance = (ENDPOINT_INSTANCE*)endpoint;
		CONNECTION_INSTANCE* connection = endpoint_instance->connection;
		AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = connection->amqp_frame_codec;

		if (amqp_frame_codec_encode_payload_bytes(amqp_frame_codec, bytes, count) != 0)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}
