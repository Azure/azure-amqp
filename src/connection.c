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
	OPEN_HANDLE open_performative;
	size_t header_bytes_received;
	CONNECTION_STATE connection_state;
	FRAME_CODEC_HANDLE frame_codec;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec;
	ENDPOINT_INSTANCE* endpoints;
	uint32_t endpoint_count;
	uint16_t frame_receive_channel;
	char* host_name;

	/* options */
	uint32_t max_frame_size;
	uint16_t channel_max;
	milliseconds idle_timeout;
} CONNECTION_INSTANCE;

static int send_header(CONNECTION_INSTANCE* connection_instance)
{
	int result;

	/* Codes_SRS_CONNECTION_01_093: [_ When the client opens a new socket connection to a server, it MUST send a protocol header with the client’s preferred protocol version.] */
	/* Codes_SRS_CONNECTION_01_104: [Sending the protocol header shall be done by using io_send.] */
	if (io_send(connection_instance->io, amqp_header, sizeof(amqp_header)) != 0)
	{
		/* Codes_SRS_CONNECTION_01_106: [When sending the protocol header fails, the connection shall be immediately closed.] */
		io_destroy(connection_instance->io);
		connection_instance->io = NULL;

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
	connection_instance->open_performative = open_create("123456");
	frame_codec_set_max_frame_size(connection_instance->frame_codec, connection_instance->max_frame_size);
	open_set_hostname(connection_instance->open_performative, connection_instance->host_name);
	AMQP_VALUE open_performative_value = amqpvalue_create_open(connection_instance->open_performative);

	if (open_performative_value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		/* handshake done, send open frame */
		/* Codes_SRS_CONNECTION_01_002: [Each AMQP connection begins with an exchange of capabilities and limitations, including the maximum frame size.] */
		/* Codes_SRS_CONNECTION_01_004: [After establishing or accepting a TCP connection and sending the protocol header, each peer MUST send an open frame before sending any other frames.] */
		/* Codes_SRS_CONNECTION_01_005: [The open frame describes the capabilities and limits of that peer.] */
		if (amqp_frame_codec_begin_encode_frame(connection_instance->amqp_frame_codec, 0, open_performative_value, 0) != 0)
		{
			io_destroy(connection_instance->io);
			connection_instance->io = NULL;
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

	return result;
}

static ENDPOINT_INSTANCE* find_session_endpoint_by_outgoing_channel(CONNECTION_INSTANCE* connection, uint16_t outgoing_channel)
{
	uint32_t i;
	ENDPOINT_INSTANCE* result;

	for (i = 0; i < connection->endpoint_count; i++)
	{
		if (connection->endpoints[i].outgoing_channel == outgoing_channel)
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
		result = &connection->endpoints[i];
	}

	return result;
}

static ENDPOINT_INSTANCE* find_session_endpoint_by_incoming_channel(CONNECTION_INSTANCE* connection, uint16_t incoming_channel)
{
	uint32_t i;
	ENDPOINT_INSTANCE* result;

	for (i = 0; i < connection->endpoint_count; i++)
	{
		if (connection->endpoints[i].incoming_channel == incoming_channel)
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
		result = &connection->endpoints[i];
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
			io_destroy(connection_instance->io);
			connection_instance->io = NULL;
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
	/* receiving in HDR_RCVD could be because pipelined open, so the best we can do is to let the bytes flow */
	case CONNECTION_STATE_HDR_RCVD:

	/* Codes_SRS_CONNECTION_01_042: [HDR EXCH In this state the connection header has been sent to the peer and a connection header has been received from the peer.] */
	/* we should not really get into this state, but just in case, we would treat that in the same way as HDR_RCVD */
	case CONNECTION_STATE_HDR_EXCH:

	/* Codes_SRS_CONNECTION_01_045: [OPEN RCVD In this state the connection headers have been exchanged. An open frame has been received from the peer but an open frame has not been sent.] */
	case CONNECTION_STATE_OPEN_RCVD:
	{
		/* receiving in OPEN_RCVD is not good, as we did not send out an OPEN frame */
		/* normally this would never happen, but in case it does, we should close the connection */
		CLOSE_HANDLE close_performative = close_create();
		AMQP_VALUE close_performative_value = amqpvalue_create_close(close_performative);
		if (amqp_frame_codec_begin_encode_frame(connection_instance->amqp_frame_codec, 0, close_performative_value, 0) != 0)
		{
			io_destroy(connection_instance->io);
			connection_instance->io = NULL;
			connection_instance->connection_state = CONNECTION_STATE_END;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_055: [DISCARDING The DISCARDING state is a variant of the CLOSE SENT state where the close is triggered by an error.] */
			connection_instance->connection_state = CONNECTION_STATE_DISCARDING;
		}
		result = __LINE__;
		break;
	}

	/* Codes_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
	case CONNECTION_STATE_OPEN_SENT:

	/* Codes_SRS_CONNECTION_01_048: [OPENED In this state the connection header and the open frame have been both sent and received.] */
	case CONNECTION_STATE_OPENED:
		result = frame_codec_receive_bytes(connection_instance->frame_codec, &b, 1);
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
	CONNECTION_INSTANCE* connection = (CONNECTION_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_descriptor(performative);
	uint64_t performative_ulong;

	amqpvalue_get_ulong(descriptor, &performative_ulong);
	switch (performative_ulong)
	{
	default:
		LOG(consolelogger_log, LOG_LINE, "Bad performative: %02x", performative);
		break;

	case AMQP_OPEN:
		LOG(consolelogger_log, 0, "<- [OPEN] ");
		LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(performative));

		switch (connection->connection_state)
		{
		default:
			break;

		case CONNECTION_STATE_OPEN_SENT:
			connection->connection_state = CONNECTION_STATE_OPENED;
			break;

		case CONNECTION_STATE_HDR_EXCH:
			connection->connection_state = CONNECTION_STATE_OPEN_RCVD;

			/* respond with an open here */

			break;
		}
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
		ENDPOINT_INSTANCE* session_endpoint = find_session_endpoint_by_outgoing_channel(connection, 0);
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
		ENDPOINT_INSTANCE* session_endpoint = find_session_endpoint_by_incoming_channel(connection, channel);
		if (session_endpoint == NULL)
		{
			/* error */
		}
		else
		{
			session_endpoint->frame_received_callback(session_endpoint->frame_received_callback_context, performative, payload_size);
			connection->frame_receive_channel = channel;
		}

		break;
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
CONNECTION_HANDLE connection_create(const char* host, int port, const char* container_id)
{
	CONNECTION_INSTANCE* result;

	if (host == NULL)
	{
		/* Codes_SRS_CONNECTION_01_071: [If host is NULL, connection_create shall return NULL.] */
		result = NULL;
	}
	else
	{
		result = (CONNECTION_INSTANCE*)amqpalloc_malloc(sizeof(CONNECTION_INSTANCE));
		/* Codes_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
		if (result != NULL)
		{
			/* Codes_SRS_CONNECTION_01_069: [The io parameters shall be filled in with the host and port information passed to connection_create.] */
			const TLSIO_CONFIG socket_io_config = { host, port };
			const IO_INTERFACE_DESCRIPTION* io_interface_description;

			/* Codes_SRS_CONNECTION_01_068: [connection_create shall pass to io_create the interface obtained by a call to socketio_get_interface_description.] */
			io_interface_description = tlsio_get_interface_description();
			if (io_interface_description == NULL)
			{
				/* Codes_SRS_CONNECTION_01_124: [If getting the io interface information (by calling socketio_get_interface_description) fails, connection_create shall return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				SASLIO_CONFIG sasl_io_config = { io_interface_description, &socket_io_config };

				io_interface_description = saslio_get_interface_description();
				if (io_interface_description == NULL)
				{
					/* Codes_SRS_CONNECTION_01_124: [If getting the io interface information (by calling socketio_get_interface_description) fails, connection_create shall return NULL.] */
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					/* Codes_SRS_CONNECTION_01_067: [connection_create shall call io_create to create its TCP IO interface.] */
					result->io = io_create(io_interface_description, &sasl_io_config, connection_receive_callback, result, consolelogger_log);
					if (result->io == NULL)
					{
						/* Codes_SRS_CONNECTION_01_070: [If io_create fails then connection_create shall return NULL.] */
						amqpalloc_free(result);
						result = NULL;
					}
					else
					{
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
								result->host_name = (char*)amqpalloc_malloc(strlen(host) + 1);
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
									strcpy(result->host_name, host);

									result->open_performative = NULL;

									result->max_frame_size = 0x1000;
									result->channel_max = 0xFFFF;
									result->idle_timeout = 0;

									/* Codes_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
									result->connection_state = CONNECTION_STATE_START;
									result->header_bytes_received = 0;
									result->endpoint_count = 0;
									result->endpoints = NULL;
								}
							}
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

		/* Codes_SRS_CONNECTION_01_151: [When max_frame_size is set, it shall be passed down to the frame_codec by a call to frame_codec_set_max_frame_size.] */
		if (frame_codec_set_max_frame_size(connection_instance->frame_codec, max_frame_size) != 0)
		{
			/* Codes_SRS_CONNECTION_01_152: [If frame_codec_set_max_frame_size fails then connection_set_max_frame_size shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_148: [connection_set_max_frame_size shall set the max_frame_size associated with a connection.] */
			connection_instance->max_frame_size = max_frame_size;

			/* Codes_SRS_CONNECTION_01_149: [On success connection_set_max_frame_size shall return 0.] */
			result = 0;
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

		/* Codes_SRS_CONNECTION_01_074: [connection_destroy shall close the socket connection.] */
		io_destroy(connection_instance->io);
		amqpalloc_free(connection_instance);
	}
}

void connection_dowork(CONNECTION_HANDLE connection)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
	if (connection_instance != NULL)
	{
		/* Codes_SRS_CONNECTION_01_084: [The connection_instance state machine implementing the protocol requirements shall be run as part of connection_dowork.] */
		switch (connection_instance->connection_state)
		{
		default:
			break;

		case CONNECTION_STATE_START:
			if (io_get_state(connection_instance->io) == IO_STATE_READY)
			{
				/* Codes_SRS_CONNECTION_01_086: [Prior to sending any frames on a connection_instance, each peer MUST start by sending a protocol header that indicates the protocol version used on the connection_instance.] */
				/* Codes_SRS_CONNECTION_01_091: [The AMQP peer which acted in the role of the TCP client (i.e. the peer that actively opened the connection_instance) MUST immediately send its outgoing protocol header on establishment of the TCP connection_instance.] */
				if (send_header(connection_instance) != 0)
				{
					io_destroy(connection_instance->io);
					connection_instance->io = NULL;
					connection_instance->connection_state = CONNECTION_STATE_END;
				}
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

	if (connection == NULL)
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

		connection_instance->endpoints = amqpalloc_realloc(connection_instance->endpoints, sizeof(ENDPOINT_INSTANCE) * (connection_instance->endpoint_count + 1));

		connection_instance->endpoints[connection_instance->endpoint_count].frame_received_callback = frame_received_callback;
		connection_instance->endpoints[connection_instance->endpoint_count].frame_payload_bytes_received_callback = frame_payload_bytes_received_callback;
		connection_instance->endpoints[connection_instance->endpoint_count].frame_received_callback_context = context;
		connection_instance->endpoints[connection_instance->endpoint_count].outgoing_channel = channel_no;
		connection_instance->endpoints[connection_instance->endpoint_count].connection = connection;

		result = &connection_instance->endpoints[connection_instance->endpoint_count];

		connection_instance->endpoint_count++;
	}

	return result;
}

void connection_destroy_endpoint(ENDPOINT_HANDLE endpoint)
{
	if (endpoint != NULL)
	{
		ENDPOINT_INSTANCE* endpoint_instance = (ENDPOINT_INSTANCE*)endpoint;
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
