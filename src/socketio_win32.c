#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "socketio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"

typedef struct SOCKET_IO_DATA_TAG
{
	SOCKET socket;
	IO_RECEIVE_CALLBACK receive_callback;
} SOCKET_IO_DATA;

static const IO_INTERFACE_DESCRIPTION socket_io_interface_description = 
{
	socketio_create,
	socketio_send,
	socketio_dowork
};

IO_HANDLE socketio_create(void* config)
{
	SOCKETIO_CONFIG* socket_io_config = config;
	SOCKET_IO_DATA* result;

	if (socket_io_config == NULL)
	{
		result = NULL;
	}
	else
	{
		result = malloc(sizeof(SOCKET_IO_DATA));
		if (result != NULL)
		{
			result->receive_callback = NULL;
			result->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (result->socket == INVALID_SOCKET)
			{
				free(result);
				result = NULL;
			}
			else
			{
				ADDRINFO* addrInfo;
				char portString[16];

				sprintf(portString, "%u", socket_io_config->port);
				if (getaddrinfo(socket_io_config->hostname, portString, NULL, &addrInfo) != 0)
				{

				}
				else
				{
					if (connect(result->socket, addrInfo->ai_addr, sizeof(*addrInfo->ai_addr)) != 0)
					{

					}
				}
			}
		}
	}

	return result;
}

void socketio_destroy(IO_HANDLE handle)
{
	if (handle != NULL)
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)handle;
		/* we cannot do much if the close fails, so just ignore the result */
		(void)closesocket(socket_io_data->socket);
		free(handle);
	}
}

int socketio_send(IO_HANDLE handle, const void* buffer, size_t size)
{
	int result;

	if ((handle == NULL) ||
		(buffer == NULL) ||
		(size == 0))
	{
		/* Invalid arguments */
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

int socketio_startreceive(IO_HANDLE handle, IO_RECEIVE_CALLBACK callback)
{
	int result;

	if ((handle == NULL) ||
		(callback == NULL))
	{
		/* Invalid arguments */
		result = __LINE__;
	}
	else
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)handle;

		/* simply save the callback for later */
		socket_io_data->receive_callback = callback;
		result = 0;
	}

	return result;
}

int socketio_dowork(IO_HANDLE handle)
{
	int result;

	if (handle == NULL)
	{
		/* Invalid arguments */
		result = __LINE__;
	}
	else
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)handle;
		unsigned char c;
		int received = 1;

		while (received > 0)
		{
			received = recv(socket_io_data->socket, &c, 1, 0);
			if (socket_io_data->receive_callback != NULL)
			{
				socket_io_data->receive_callback(handle, &c, 1);
			}
		}

		result = 0;
	}

	return result;
}

const IO_INTERFACE_DESCRIPTION* socketio_get_interface_description(void)
{
	return &socket_io_interface_description;
}
