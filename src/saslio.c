#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "saslio.h"
#include "amqpalloc.h"
#include "sasl_frame_codec.h"

typedef struct SASL_IO_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_RECEIVE_CALLBACK receive_callback;
	LOGGER_LOG logger_log;
	void* context;
} SASL_IO_DATA;

static const IO_INTERFACE_DESCRIPTION sasl_io_interface_description =
{
	saslio_create,
	saslio_destroy,
	saslio_send,
	saslio_dowork
};

static void saslio_receive_bytes(void* context, const void* buffer, size_t size)
{
}

IO_HANDLE saslio_create(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* context, LOGGER_LOG logger_log)
{
	SASLIO_CONFIG* sasl_io_config = io_create_parameters;
	SASL_IO_DATA* result;

	if (sasl_io_config == NULL)
	{
		result = NULL;
	}
	else
	{
		result = amqpalloc_malloc(sizeof(SASL_IO_DATA));
		if (result != NULL)
		{
			result->receive_callback = NULL;
			result->logger_log = logger_log;
			result->receive_callback = receive_callback;
			result->context = context;
			result->socket_io = sasl_io_config->socket_io;
		}
	}

	return result;
}

void saslio_destroy(IO_HANDLE handle)
{
	if (handle != NULL)
	{
		SASL_IO_DATA* sasl_io_data = (SASL_IO_DATA*)handle;
		amqpalloc_free(handle);
	}
}

int saslio_send(IO_HANDLE handle, const void* buffer, size_t size)
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
		SASL_IO_DATA* sasl_io_data = (SASL_IO_DATA*)handle;
	}

	return result;
}

void saslio_dowork(IO_HANDLE handle)
{
	if (handle != NULL)
	{
		SASL_IO_DATA* sasl_io_data = (SASL_IO_DATA*)handle;
	}
}

const IO_INTERFACE_DESCRIPTION* saslio_get_interface_description(void)
{
	return &sasl_io_interface_description;
}
