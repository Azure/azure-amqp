#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tlsio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "amqpalloc.h"

typedef struct SOCKET_IO_DATA_TAG
{
	SOCKET socket;
	IO_RECEIVE_CALLBACK receive_callback;
	LOGGER_LOG logger_log;
	void* context;
} SOCKET_IO_DATA;

static const IO_INTERFACE_DESCRIPTION socket_io_interface_description =
{
	tlsio_create,
	tlsio_destroy,
	tlsio_send,
	tlsio_dowork
};

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
			result->receive_callback = NULL;
			result->logger_log = logger_log;
			result->receive_callback = receive_callback;
			result->context = context;

			result->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (result->socket == INVALID_SOCKET)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				ADDRINFO* addrInfo;
				char portString[16];

				sprintf(portString, "%u", tls_io_config->port);
				if (getaddrinfo(tls_io_config->hostname, portString, NULL, &addrInfo) != 0)
				{
					closesocket(result->socket);
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					u_long iMode = 1;

					if (connect(result->socket, addrInfo->ai_addr, sizeof(*addrInfo->ai_addr)) != 0)
					{
						closesocket(result->socket);
						amqpalloc_free(result);
						result = NULL;
					}
					else if (ioctlsocket(result->socket, FIONBIO, &iMode))
					{
						closesocket(result->socket);
						amqpalloc_free(result);
						result = NULL;
					}
				}
			}
		}
	}

	return result;
}

void tlsio_destroy(IO_HANDLE handle)
{
	if (handle != NULL)
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)handle;
		/* we cannot do much if the close fails, so just ignore the result */
		(void)closesocket(socket_io_data->socket);
		amqpalloc_free(handle);
	}
}

int tlsio_send(IO_HANDLE handle, const void* buffer, size_t size)
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
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)handle;
		int send_result = send(socket_io_data->socket, buffer, size, 0);
		if (send_result != size)
		{
			result = __LINE__;
		}
		else
		{
			size_t i;
			for (i = 0; i < size; i++)
			{
				socket_io_data->logger_log("%02x-> ", ((unsigned char*)buffer)[i]);
			}

			result = 0;
		}
	}

	return result;
}

void tlsio_dowork(IO_HANDLE handle)
{
	if (handle != NULL)
	{
		SOCKET_IO_DATA* socket_io_data = (SOCKET_IO_DATA*)handle;
		unsigned char c;
		int received = 1;

		while (received > 0)
		{
			received = recv(socket_io_data->socket, &c, 1, 0);
			if (received > 0)
			{
				socket_io_data->logger_log("<-%02x ", (unsigned char)c);

				if (socket_io_data->receive_callback != NULL)
				{
					/* explictly ignoring here the result of the callback */
					(void)socket_io_data->receive_callback(socket_io_data->context, &c, 1);
				}
			}
		}
	}
}

const IO_INTERFACE_DESCRIPTION* tlsio_get_interface_description(void)
{
	return &socket_io_interface_description;
}
