#include <stddef.h>
#include <stdlib.h>
#include "io.h"

typedef struct IO_DATA_TAG
{
	void* concrete_io_handle;
	IO_SEND io_send;
} IO_DATA;

IO_HANDLE io_create(void* concrete_io_handle, IO_SEND io_send)
{
	IO_DATA* io_data = (IO_DATA*)malloc(sizeof(IO_DATA));
	if (io_data != NULL)
	{
		io_data->concrete_io_handle = concrete_io_handle;
		io_data->io_send = io_send;
	}
	return (IO_HANDLE)io_data;
}

int io_send(IO_HANDLE handle, const void* buffer, size_t size)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		IO_DATA* io_data = (IO_DATA*)handle;
		result = io_data->io_send(io_data->concrete_io_handle, buffer, size);
	}

	return result;
}
