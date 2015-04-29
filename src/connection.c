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
	FRAME_CODEC_HANDLE frame_codec;
	FRAME_RECEIVED_CALLBACK session_frame_received_callback;
	void* session_frame_received_callback_context;
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
	case CONNECTION_STATE_OPENED:
		(void)frame_codec_receive_bytes(connection->frame_codec, &b, 1);
		break;
	}
}

static void connection_receive_callback(IO_HANDLE io, void* context, const void* buffer, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++)
	{
		connection_byte_received((CONNECTION_DATA*)context, ((unsigned char*)buffer)[i]);
	}
}

static void connection_frame_received(void* context, uint64_t performative, AMQP_VALUE frame_list_value)
{
	CONNECTION_DATA* connection = (CONNECTION_DATA*)context;
	switch (performative)
	{
	default:
		consolelogger_log("Bad performative: %llu", (unsigned long long)performative);
		break;

	case 0x10:
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

	case 0x11:
		if (connection->session_frame_received_callback != NULL)
		{
			connection->session_frame_received_callback(connection->session_frame_received_callback_context, performative, frame_list_value);
		}
		break;
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
			result->frame_codec = frame_codec_create(result->socket_io, connection_frame_received, result, consolelogger_log);
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

int connection_set_session_frame_receive_callback(CONNECTION_HANDLE handle, FRAME_RECEIVED_CALLBACK callback, void* context)
{
	int result;
	CONNECTION_DATA* connection = (CONNECTION_DATA*)handle;
	if (connection == NULL)
	{
		result = __LINE__;
	}
	else
	{
		connection->session_frame_received_callback = callback;
		connection->session_frame_received_callback_context = context;
		result = 0;
	}

	return result;
}
