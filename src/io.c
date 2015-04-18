#include <stddef.h>
#include <stdlib.h>
#include "io.h"

typedef struct IO_DATA_TAG
{
	const IO_INTERFACE_DESCRIPTION* io_interface_description;
	IO_HANDLE concrete_io_handle;
} IO_DATA;

IO_HANDLE io_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, LOGGER_LOG logger_log)
{
	IO_DATA* io_data = (IO_DATA*)malloc(sizeof(IO_DATA));
	if (io_data != NULL)
	{
		io_data->io_interface_description = io_interface_description;
		io_data->concrete_io_handle = io_data->io_interface_description->io_create(io_create_parameters, receive_callback, logger_log);
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
		result = io_data->io_interface_description->io_send(io_data->concrete_io_handle, buffer, size);
	}

	return result;
}

int io_dowork(IO_HANDLE handle)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		IO_DATA* io_data = (IO_DATA*)handle;
		result = io_data->io_interface_description->io_dowork(io_data->concrete_io_handle);
	}

	return result;
}
