#include <stdlib.h>
#include "io.h"
#include "consolelogger.h"
#include "amqplib.h"
#include "socketio.h"
#include "platform.h"

typedef enum CONNECTION_STATE_TAG
{
	CONNECTION_STATE_START,
	CONNECTION_STATE_HDR_RCVD,
	CONNECTION_STATE_HDR_SENT,
	CONNECTION_STATE_HDR_EXCH,
	CONNECTION_STATE_OPEN_PIPE,
	CONNECTION_STATE_OC_PIPE,
	CONNECTION_STATE_OPEN_RCVD,
	CONNECTION_STATE_OPEN_SENT,
	CONNECTION_STATE_CLOSE_PIPE,
	CONNECTION_STATE_OPENED,
	CONNECTION_STATE_CLOSE_RCVD,
	CONNECTION_STATE_CLOSE_SENT,
	CONNECTION_STATE_DISCARDING,
	CONNECTION_STATE_END
} CONNECTION_STATE;

typedef struct AMQPLIB_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_HANDLE used_io;
	CONNECTION_STATE connection_state;
} AMQPLIB_DATA;

int amqplib_init(void)
{
	int result;

	if (platform_init())
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

void amqplib_deinit(void)
{
	platform_deinit();
}

static void connection_receive_callback(IO_HANDLE handle, const void* buffer, size_t size)
{

}

AMQPLIB_HANDLE amqplib_create(const char* host, int port)
{
	AMQPLIB_DATA* result = malloc(sizeof(AMQPLIB_DATA));
	if (result != NULL)
	{
		SOCKETIO_CONFIG socket_io_config = { host, port };
		result->socket_io = io_create(socketio_get_interface_description(), &socket_io_config, connection_receive_callback, consolelogger_log);
		result->connection_state = CONNECTION_STATE_START;

		/* For now directly talk to the socket IO. By doing this there is no SASL, no SSL, pure AMQP only */
		result->used_io = result->socket_io;
	}

	return result;
}

void amqplib_destroy(AMQPLIB_HANDLE handle)
{
	if (handle != NULL)
	{
		AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;
		socketio_destroy(amqp_lib->socket_io);
		free(handle);
	}
}

static int connection_sendheader(AMQPLIB_DATA* amqp_lib)
{
	int result;
	unsigned char header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

	if (io_send(amqp_lib->used_io, header, sizeof(header)) != 0)
	{
		result = __LINE__;
	}
	else
	{
		amqp_lib->connection_state = CONNECTION_STATE_HDR_SENT;
		result = 0;
	}

	return result;
}

static int connection_dowork(AMQPLIB_DATA* amqp_lib)
{
	int result;

	switch (amqp_lib->connection_state)
	{
		default:
			result = __LINE__;
			break;

		case CONNECTION_STATE_START:
			result = connection_sendheader(amqp_lib);
			break;
	}

	return result;
}

int amqplib_dowork(AMQPLIB_HANDLE handle)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;

		result = connection_dowork(amqp_lib);
		if (result == 0)
		{
			result = io_dowork(amqp_lib->socket_io);
		}
	}

	return result;
}
