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
#include "session.h"
#include "link.h"
#include "messaging.h"

typedef struct AMQPLIB_DATA_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_HANDLE session;
	LINK_HANDLE link;
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

AMQPLIB_HANDLE amqplib_create(void)
{
	AMQPLIB_DATA* result = malloc(sizeof(AMQPLIB_DATA));
	if (result != NULL)
	{
	}

	return result;
}

void amqplib_destroy(AMQPLIB_HANDLE handle)
{
	if (handle != NULL)
	{
		AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;
		session_destroy(amqp_lib->session);
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
		if (result == 0)
		{
			result = session_dowork(amqp_lib->session);
			if (result == 0)
			{
				result = link_dowork(amqp_lib->link);
			}
		}
	}

	return result;
}

LINK_HANDLE amqplib_get_link(AMQPLIB_HANDLE handle)
{
	AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;
	return amqp_lib->link;
}