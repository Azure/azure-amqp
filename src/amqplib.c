#include <stdlib.h>
#include <stdint.h>
#include "io.h"
#include "consolelogger.h"
#include "amqplib.h"
#include "socketio.h"
#include "platform.h"
#include "encoder.h"
#include "decoder.h"
#include "connection.h"

typedef struct AMQPLIB_DATA_TAG
{
	CONNECTION_HANDLE connection;
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
		result->connection = connection_create(host, port);
		if (result->connection == NULL)
		{
			free(result);
			result = NULL;
		}
	}

	return result;
}

void amqplib_destroy(AMQPLIB_HANDLE handle)
{
	if (handle != NULL)
	{
		AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;
		connection_destroy(amqp_lib->connection);
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

		result = connection_dowork(amqp_lib->connection);
	}

	return result;
}
