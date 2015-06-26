#include <stdlib.h>
#include "connection.h"
#include "consolelogger.h"
#include "frame_codec.h"
#include "amqp_frame_codec.h"
#include "amqp_definitions.h"
#include "socketio.h"
#include "amqpalloc.h"

/* Requirements satisfied by the virtue of implementing the ISO:*/
/* Codes_SRS_CONNECTION_01_088: [Any data appearing beyond the protocol header MUST match the version indicated by the protocol header.] */

/* Codes_SRS_CONNECTION_01_087: [The protocol header consists of the upper case ASCII letters “AMQP” followed by a protocol id of zero, followed by three unsigned bytes representing the major, minor, and revision of the protocol version (currently 1 (MAJOR), 0 (MINOR), 0 (REVISION)). In total this is an 8-octet sequence] */
static unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_FRAME_DATA
} RECEIVE_FRAME_STATE;

typedef struct CONNECTION_DATA_TAG
{
	IO_HANDLE socket_io;
	OPEN_HANDLE open_performative;
	size_t header_bytes_received;
	CONNECTION_STATE connection_state;
	FRAME_CODEC_HANDLE frame_codec;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec;
	AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* frame_received_callback_context;
} CONNECTION_INSTANCE;

static int send_header(CONNECTION_INSTANCE* connection_instance)
{
	int result;

	/* Codes_SRS_CONNECTION_01_093: [_ When the client opens a new socket connection to a server, it MUST send a protocol header with the client’s preferred protocol version.] */
	/* Codes_SRS_CONNECTION_01_104: [Sending the protocol header shall be done by using io_send.] */
	if (io_send(connection_instance->socket_io, amqp_header, sizeof(amqp_header)) != 0)
	{
		/* Codes_SRS_CONNECTION_01_106: [When sending the protocol header fails, the connection shall be immediately closed.] */
		io_destroy(connection_instance->socket_io);
		connection_instance->socket_io = NULL;

		/* Codes_SRS_CONNECTION_01_057: [END In this state it is illegal for either endpoint to write anything more onto the connection. The connection can be safely closed and discarded.] */
		connection_instance->connection_state = CONNECTION_STATE_END;

		/* Codes_SRS_CONNECTION_01_105: [When io_send fails, connection_dowork shall return a non-zero value.] */
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_CONNECTION_01_041: [HDR SENT In this state the connection header has been sent to the peer but no connection header has been received.] */
		connection_instance->connection_state = CONNECTION_STATE_HDR_SENT;
		result = 0;
	}

	return result;
}

static int send_open_frame(CONNECTION_INSTANCE* connection_instance)
{
	int result;
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
			io_destroy(connection_instance->socket_io);
			connection_instance->socket_io = NULL;
			connection_instance->connection_state = CONNECTION_STATE_END;
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_CONNECTION_01_046: [OPEN SENT In this state the connection headers have been exchanged. An open frame has been sent to the peer but no open frame has yet been received.] */
			connection_instance->connection_state = CONNECTION_STATE_OPEN_SENT;
			result = 0;
		}

		amqpvalue_destroy(open_performative_value);
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
			io_destroy(connection_instance->socket_io);
			connection_instance->socket_io = NULL;
			connection_instance->connection_state = CONNECTION_STATE_END;
			result = __LINE__;
		}
		else
		{
			connection_instance->header_bytes_received++;
			if (connection_instance->header_bytes_received == sizeof(amqp_header))
			{
				if (connection_instance->connection_state == CONNECTION_STATE_START)
				{
					if (send_header(connection_instance) != 0)
					{
						io_destroy(connection_instance->socket_io);
						connection_instance->socket_io = NULL;
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
			io_destroy(connection_instance->socket_io);
			connection_instance->socket_io = NULL;
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

static int connection_receive_callback(void* context, const void* buffer, size_t size)
{
	int result;
	size_t i;

	for (i = 0; i < size; i++)
	{
		if (connection_byte_received((CONNECTION_INSTANCE*)context, ((unsigned char*)buffer)[i]) != 0)
		{
			break;
		}
	}

	if (i < size)
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

static int connection_empty_frame_received(void* context, uint16_t channel)
{
	return 0;
}

static int connection_payload_bytes_received(void* context, const unsigned char* payload_bytes, uint32_t byte_count)
{
	return 0;
}

static int connection_frame_received(void* context, uint16_t channel, AMQP_VALUE performative, uint32_t payload_size)
{
	CONNECTION_INSTANCE* connection = (CONNECTION_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_descriptor(performative);
	uint64_t performative_ulong;

	amqpvalue_get_ulong(descriptor, &performative_ulong);
	switch (performative_ulong)
	{
	default:
		consolelogger_log("Bad performative: %02x", performative);
		break;

	case AMQP_OPEN:
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
		break;

	case AMQP_BEGIN:
	case AMQP_ATTACH:
	case AMQP_FLOW:
	case AMQP_TRANSFER:
	case AMQP_DISPOSITION:
	case AMQP_DETACH:
	case AMQP_END:
		if (connection->frame_received_callback != NULL)
		{
			connection->frame_received_callback(connection->frame_received_callback_context, 0, performative, 0);
		}
		break;
	}

	return 0;
}

/* Codes_SRS_CONNECTION_01_001: [connection_create shall open a new connection to a specified host/port.] */
CONNECTION_HANDLE connection_create(const char* host, int port)
{
	CONNECTION_INSTANCE* result;

	/* Codes_SRS_CONNECTION_01_071: [If host is NULL, connection_create shall return NULL.] */
	if (host == NULL)
	{
		result = NULL;
	}
	else
	{
		result = (CONNECTION_INSTANCE*)amqpalloc_malloc(sizeof(CONNECTION_INSTANCE));
		/* Codes_SRS_CONNECTION_01_081: [If allocating the memory for the connection fails then connection_create shall return NULL.] */
		if (result != NULL)
		{
			/* Codes_SRS_CONNECTION_01_069: [The socket_io parameters shall be filled in with the host and port information passed to connection_create.] */
			SOCKETIO_CONFIG socket_io_config = { host, port };
			const IO_INTERFACE_DESCRIPTION* io_interface_description;

			/* Codes_SRS_CONNECTION_01_068: [connection_create shall pass to io_create the interface obtained by a call to socketio_get_interface_description.] */
			io_interface_description = socketio_get_interface_description();
			if (io_interface_description == NULL)
			{
				/* Codes_SRS_CONNECTION_01_080: [If socketio_get_interface_description fails, connection_create shall return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				/* Codes_SRS_CONNECTION_01_067: [connection_create shall call io_create to create its TCP IO interface.] */
				result->socket_io = io_create(io_interface_description, &socket_io_config, connection_receive_callback, result, consolelogger_log);
				if (result->socket_io == NULL)
				{
					/* Codes_SRS_CONNECTION_01_070: [If io_create fails then connection_create shall return NULL.] */
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					/* Codes_SRS_CONNECTION_01_082: [connection_create shall allocate a new frame_codec instance to be used for frame encoding/decoding.] */
					result->frame_codec = frame_codec_create(result->socket_io, consolelogger_log);
					if (result->frame_codec == NULL)
					{
						/* Codes_SRS_CONNECTION_01_083: [If frame_codec_create fails then connection_create shall return NULL.] */
						io_destroy(result->socket_io);
						amqpalloc_free(result);
						result = NULL;
					}
					else
					{
						result->amqp_frame_codec = amqp_frame_codec_create(result->frame_codec, connection_frame_received, connection_empty_frame_received, connection_payload_bytes_received, result);
						if (result->amqp_frame_codec == NULL)
						{
							frame_codec_destroy(result->frame_codec);
							io_destroy(result->socket_io);
							amqpalloc_free(result);
							result = NULL;
						}
						else
						{
							result->open_performative = open_create("1");
							if (result->open_performative == NULL)
							{
								amqp_frame_codec_destroy(result->amqp_frame_codec);
								frame_codec_destroy(result->frame_codec);
								io_destroy(result->socket_io);
								amqpalloc_free(result);
								result = NULL;
							}
							else
							{
								result->frame_received_callback = NULL;

								/* Codes_SRS_CONNECTION_01_072: [When connection_create succeeds, the state of the connection shall be CONNECTION_STATE_START.] */
								result->connection_state = CONNECTION_STATE_START;
								result->header_bytes_received = 0;
							}
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
		open_destroy(connection_instance->open_performative);

		/* Codes_SRS_CONNECTION_01_074: [connection_destroy shall close the socket connection.] */
		io_destroy(connection_instance->socket_io);
		amqpalloc_free(connection_instance);
	}
}

int connection_set_container_id(CONNECTION_HANDLE connection, const char* container_id)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
	open_set_container_id(connection_instance->open_performative, container_id);
	return 0;
}

int connection_set_max_frame_size(CONNECTION_HANDLE connection, uint32_t max_frame_size)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
	open_set_max_frame_size(connection_instance->open_performative, max_frame_size);
	return 0;
}

int connection_set_channel_max(CONNECTION_HANDLE connection, uint16_t channel_max)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
	open_set_channel_max(connection_instance->open_performative, channel_max);
	return 0;
}

int connection_set_idle_timeout(CONNECTION_HANDLE connection, milliseconds idle_timeout)
{
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
	open_set_idle_time_out(connection_instance->open_performative, idle_timeout);
	return 0;
}

int connection_connect(void)
{
	return 0;
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
			/* Codes_SRS_CONNECTION_01_086: [Prior to sending any frames on a connection_instance, each peer MUST start by sending a protocol header that indicates the protocol version used on the connection_instance.] */
			/* Codes_SRS_CONNECTION_01_091: [The AMQP peer which acted in the role of the TCP client (i.e. the peer that actively opened the connection_instance) MUST immediately send its outgoing protocol header on establishment of the TCP connection_instance.] */
			if (send_header(connection_instance))
			{
				io_destroy(connection_instance->socket_io);
				connection_instance->socket_io = NULL;
				connection_instance->connection_state = CONNECTION_STATE_END;
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
			if (send_open_frame(connection) != 0)
			{
				io_destroy(connection_instance->socket_io);
				connection_instance->socket_io = NULL;
				connection_instance->connection_state = CONNECTION_STATE_END;
			}
			break;

		case CONNECTION_STATE_OPEN_RCVD:
			break;
		}

		/* Codes_SRS_CONNECTION_01_076: [connection_dowork shall schedule the underlying IO interface to do its work by calling io_dowork.] */
		io_dowork(connection_instance->socket_io);
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

AMQP_FRAME_CODEC_HANDLE connection_get_amqp_frame_codec(CONNECTION_HANDLE connection)
{
	AMQP_FRAME_CODEC_HANDLE result;
	CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;

	if (connection_instance == NULL)
	{
		result = NULL;
	}
	else
	{
		result = connection_instance->amqp_frame_codec;
	}

	return result;
}

int connection_register_session(CONNECTION_HANDLE connection, AMQP_FRAME_RECEIVED_CALLBACK callback, void* context, uint16_t* channel_no)
{
	int result;

	if (connection == NULL)
	{
		CONNECTION_INSTANCE* connection_instance = (CONNECTION_INSTANCE*)connection;
		connection_instance->frame_received_callback = callback;
		connection_instance->frame_received_callback_context = context;
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return 0;
}

int connection_unregister_session(CONNECTION_HANDLE connection, uint16_t channel_no)
{
	return 0;
}
