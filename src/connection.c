#include <stdlib.h>
#include "connection.h"
#include "decoder.h"
#include "consolelogger.h"
#include "frame_codec.h"
#include "socketio.h"

static unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };
#define FRAME_HEADER_SIZE 8

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_FRAME_DATA
} RECEIVE_FRAME_STATE;

typedef struct CONNECTION_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_HANDLE used_io;
	size_t header_bytes_received;
	CONNECTION_STATE connection_state;
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_bytes;
	size_t receive_frame_consumed_bytes;
	uint32_t receive_frame_size;
	unsigned char receive_frame_buffer[2048];
	FRAME_CODEC_HANDLE frame_codec;
} CONNECTION_DATA;

static int connection_sendheader(CONNECTION_DATA* connection)
{
	int result;

	if (io_send(connection->used_io, amqp_header, sizeof(amqp_header)) != 0)
	{
		result = __LINE__;
	}
	else
	{
		connection->connection_state = CONNECTION_STATE_HDR_SENT;
		result = 0;
	}

	return result;
}

static int connection_frame_write_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

static int send_open(CONNECTION_DATA* connection, const char* container_id)
{
	int result;

	AMQP_VALUE open_frame_list;
	if ((open_frame_list = amqpvalue_create_list(1)) == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE container_id_value = amqpvalue_create_string(container_id);
		if (container_id_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if ((amqpvalue_set_list_item(open_frame_list, 0, container_id_value) != 0) ||
				(frame_codec_encode(connection->frame_codec, 0x10, open_frame_list) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(container_id_value);
		}

		amqpvalue_destroy(open_frame_list);
	}

	return result;
}

static int connection_decode_received_amqp_frame(CONNECTION_DATA* connection)
{
	uint16_t channel;
	uint8_t doff = connection->receive_frame_buffer[4];
	unsigned char* frame_body;
	uint32_t frame_body_size = connection->receive_frame_size - doff * 4;
	DECODER_HANDLE decoder_handle;
	AMQP_VALUE descriptor;
	AMQP_VALUE container_id;
	int result;
	bool more;

	channel = connection->receive_frame_buffer[6] << 8;
	channel += connection->receive_frame_buffer[7];

	frame_body = &connection->receive_frame_buffer[4 * doff];
	decoder_handle = decoder_create(frame_body, frame_body_size);

	if ((decoder_decode(decoder_handle, &descriptor, &more) != 0) ||
		(!more) ||
		(decoder_decode(decoder_handle, &container_id, &more) != 0))
	{
		result = __LINE__;
	}
	else
	{
		if (connection->connection_state == CONNECTION_STATE_OPEN_SENT)
		{
			connection->connection_state = CONNECTION_STATE_OPENED;
		}

		result = 0;
	}

	return result;
}

static int connection_decode_received_sasl_frame(CONNECTION_DATA* connection)
{
	/* not implemented */
	return __LINE__;
}

static int connection_decode_received_frame(CONNECTION_DATA* connection)
{
	int result;

	/* decode type */
	uint8_t type = connection->receive_frame_buffer[5];

	switch (type)
	{
	default:
		consolelogger_log("Unknown frame.\r\n");
		result = __LINE__;
		break;

	case 0:
		result = connection_decode_received_amqp_frame(connection);
		break;

	case 1:
		result = connection_decode_received_sasl_frame(connection);
		break;
	}

	return result;
}

static int connection_receive_frame(CONNECTION_DATA* connection, unsigned char b)
{
	int result;

	connection->receive_frame_buffer[connection->receive_frame_bytes] = b;
	connection->receive_frame_bytes++;

	switch (connection->receive_frame_state)
	{
	default:
		result = __LINE__;

	case RECEIVE_FRAME_STATE_FRAME_SIZE:
		if (connection->receive_frame_bytes - connection->receive_frame_consumed_bytes >= 4)
		{
			connection->receive_frame_size = connection->receive_frame_buffer[connection->receive_frame_consumed_bytes++] << 24;
			connection->receive_frame_size += connection->receive_frame_buffer[connection->receive_frame_consumed_bytes++] << 16;
			connection->receive_frame_size += connection->receive_frame_buffer[connection->receive_frame_consumed_bytes++] << 8;
			connection->receive_frame_size += connection->receive_frame_buffer[connection->receive_frame_consumed_bytes++];
			connection->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_DATA;
		}

		result = 0;
		break;

	case RECEIVE_FRAME_STATE_FRAME_DATA:
		if (connection->receive_frame_bytes - connection->receive_frame_consumed_bytes == connection->receive_frame_size - 4)
		{
			/* done receiving */
			if (connection_decode_received_frame(connection) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			connection->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			connection->receive_frame_bytes = 0;
			connection->receive_frame_consumed_bytes = 0;
		}
		else
		{
			result = 0;
		}
		break;
	}

	return result;
}

static void connection_byte_received(CONNECTION_DATA* connection, unsigned char b)
{
	switch (connection->connection_state)
	{
	default:
		break;

	case CONNECTION_STATE_HDR_SENT:
		if (b != amqp_header[connection->header_bytes_received])
		{
			/* close connection */
			io_destroy(connection->used_io);
			connection->connection_state = CONNECTION_STATE_END;
		}
		else
		{
			connection->header_bytes_received++;
			if (connection->header_bytes_received == sizeof(amqp_header))
			{
				connection->connection_state = CONNECTION_STATE_HDR_EXCH;
				connection->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
				connection->receive_frame_bytes = 0;
				connection->receive_frame_consumed_bytes = 0;

				/* handshake done, send open frame */
				if (send_open(connection, "1") != 0)
				{
					io_destroy(connection->used_io);
					connection->connection_state = CONNECTION_STATE_END;
				}
				else
				{
					connection->connection_state = CONNECTION_STATE_OPEN_SENT;
				}
			}
		}
		break;

	case CONNECTION_STATE_OPEN_SENT:
		(void)connection_receive_frame(connection, b);
		break;
	}
}

static void connection_receive_callback(IO_HANDLE handle, void* context, const void* buffer, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++)
	{
		connection_byte_received((CONNECTION_DATA*)context, ((unsigned char*)buffer)[i]);
	}
}

CONNECTION_HANDLE connection_create(const char* host, int port)
{
	CONNECTION_DATA* result = malloc(sizeof(CONNECTION_DATA));
	if (result != NULL)
	{
		SOCKETIO_CONFIG socket_io_config = { host, port };
		result->socket_io = io_create(socketio_get_interface_description(), &socket_io_config, connection_receive_callback, result, consolelogger_log);
		if (result->socket_io == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->frame_codec = frame_codec_create(result->socket_io, consolelogger_log);
			if (result->frame_codec == NULL)
			{
				io_destroy(result->socket_io);
				free(result);
				result = NULL;
			}
			else
			{
				result->connection_state = CONNECTION_STATE_START;
				result->header_bytes_received = 0;

				/* For now directly talk to the socket IO. By doing this there is no SASL, no SSL, pure AMQP only */
				result->used_io = result->socket_io;
			}
		}
	}

	return result;
}

void connection_destroy(CONNECTION_HANDLE handle)
{
	if (handle != NULL)
	{
		CONNECTION_DATA* connection = (CONNECTION_DATA*)handle;
		frame_codec_destroy(connection->frame_codec);
		io_destroy(connection->socket_io);
		free(handle);
	}
}

int connection_dowork(CONNECTION_HANDLE handle)
{
	int result;
	CONNECTION_DATA* connection = (CONNECTION_DATA*)handle;

	switch (connection->connection_state)
	{
	default:
		result = __LINE__;
		break;

	case CONNECTION_STATE_START:
		result = connection_sendheader(connection);
		break;

	case CONNECTION_STATE_HDR_SENT:
	case CONNECTION_STATE_OPEN_SENT:
	case CONNECTION_STATE_OPENED:
		result = 0;
		break;

	case CONNECTION_STATE_HDR_EXCH:
		send_open(connection, "1");
		break;

	case CONNECTION_STATE_OPEN_RCVD:
		/* peer wants to open, let's panic */
		result = __LINE__;
		break;
	}

	if (result == 0)
	{
		result = io_dowork(connection->socket_io);
	}

	return result;
}

int connection_get_state(CONNECTION_HANDLE handle, CONNECTION_STATE* connection_state)
{
	int result;
	CONNECTION_DATA* connection = (CONNECTION_DATA*)handle;

	if (connection == NULL)
	{
		result = __LINE__;
	}
	else
	{
		*connection_state = connection->connection_state;
		result = 0;
	}

	return result;
}

FRAME_CODEC_HANDLE connection_get_frame_codec(CONNECTION_HANDLE handle)
{
	FRAME_CODEC_HANDLE result;
	CONNECTION_DATA* connection = (CONNECTION_DATA*)handle;

	if (connection == NULL)
	{
		result = NULL;
	}
	else
	{
		result = connection->frame_codec;
	}

	return result;
}
