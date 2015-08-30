#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tlsio.h"
#include "socketio.h"
#include "windows.h"
#include "amqpalloc.h"

typedef struct SOCKET_IO_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_RECEIVE_CALLBACK receive_callback;
	LOGGER_LOG logger_log;
	void* context;
} SOCKET_IO_DATA;

static const IO_INTERFACE_DESCRIPTION tls_io_interface_description =
{
	tlsio_create,
	tlsio_destroy,
	tlsio_send,
	tlsio_dowork,
	tlsio_get_state
};

static void tlsio_receive_bytes(void* context, const void* buffer, size_t size)
{
}

IO_HANDLE tlsio_create(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* context, LOGGER_LOG logger_log)
{
	TLSIO_CONFIG* tls_io_config = io_create_parameters;
	SOCKET_IO_DATA* result;

	if (tls_io_config == NULL)
	{
		result = NULL;
	}
	else
	{
		result = amqpalloc_malloc(sizeof(SOCKET_IO_DATA));
		if (result != NULL)
		{
			SOCKETIO_CONFIG socketio_config;

			socketio_config.hostname = tls_io_config->hostname;
			socketio_config.port = tls_io_config->port;

			result->receive_callback = NULL;
			result->logger_log = logger_log;
			result->receive_callback = receive_callback;
			result->context = context;

			result->socket_io = socketio_create(&socketio_config, tlsio_receive_bytes, result, logger_log);
			if (result->socket_io == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
		}
	}

	return result;
}

void tlsio_destroy(IO_HANDLE tls_io)
{
	if (tls_io != NULL)
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)tls_io;
		socketio_destroy(socket_io_data->socket_io);
		amqpalloc_free(tls_io);
	}
}

int tlsio_send(IO_HANDLE tls_io, const void* buffer, size_t size)
{
	int result;

	if ((tls_io == NULL) ||
		(buffer == NULL) ||
		(size == 0))
	{
		/* Invalid arguments */
		result = __LINE__;
	}
	else
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)tls_io;
	}

	return result;
}

void tlsio_dowork(IO_HANDLE tls_io)
{
	if (tls_io != NULL)
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)tls_io;
		socketio_dowork(socket_io_data->socket_io);
	}
}

IO_STATE tlsio_get_state(IO_HANDLE tls_io)
{
	IO_STATE result;

	if (tls_io == NULL)
	{
		result = IO_STATE_ERROR;
	}
	else
	{
		result = IO_STATE_READY;
	}

	return result;
}

const IO_INTERFACE_DESCRIPTION* tlsio_get_interface_description(void)
{
	return &tls_io_interface_description;
}
