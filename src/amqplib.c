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
		else
		{
			result->session = session_create(result->connection);
			if (result->session == NULL)
			{
				connection_destroy(result->connection);
				free(result);
				result = NULL;
			}
			else
			{
				AMQP_VALUE source_address = amqpvalue_create_string("/");
				AMQP_VALUE target_address = amqpvalue_create_string("/");

				if ((source_address == NULL) ||
					(target_address == NULL))
				{
					connection_destroy(result->connection);
					link_destroy(result->link);
					free(result);
					result = NULL;
				}
				else
				{
					result->link = link_create(result->session, messaging_create_source(source_address), messaging_create_target(amqpvalue_create_string(target_address)));
				}

				amqpvalue_destroy(source_address);
				amqpvalue_destroy(target_address);
			}
		}
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
		}
	}

	return result;
}
