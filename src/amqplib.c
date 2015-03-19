#include <stdlib.h>
#include "amqplib.h"
#include "socketio.h"
#include "platform.h"

typedef struct AMQPLIB_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_HANDLE used_io;
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

AMQPLIB_HANDLE amqplib_create(const char* host, int port)
{
	AMQPLIB_DATA* result = malloc(sizeof(AMQPLIB_DATA));
	if (result != NULL)
	{
		SOCKETIO_CONFIG socket_io_config = { host, port };
		result->socket_io = socketio_create(&socket_io_config);

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
		socketio_dowork(amqp_lib->socket_io);
		result = 0;
	}

	return result;
}
