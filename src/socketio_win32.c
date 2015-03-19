#include <stddef.h>
#include <stdlib.h>
#include "socketio.h"
#include "windows.h"
#include "winsock.h"

typedef struct SOCKET_IO_DATA_TAG
{
	SOCKET socket;
} SOCKET_IO_DATA;

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
			result->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (result->socket == NULL)
			{
				free(result);
				result = NULL;
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
		free(handle);
	}
}
