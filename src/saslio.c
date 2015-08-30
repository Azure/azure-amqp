#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "saslio.h"
#include "amqpalloc.h"
#include "sasl_frame_codec.h"

typedef enum SASL_IO_STATE_TAG
{
	SASL_IO_IDLE,
	SASL_IO_HEADER_SENT,
	SASL_IO_HEADER_RCVD,
	SASL_IO_HEADER_EXCH,
	SASL_IO_ERROR
} SASL_IO_STATE;

typedef struct SASL_IO_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_RECEIVE_CALLBACK receive_callback;
	LOGGER_LOG logger_log;
	void* context;
	SASL_IO_STATE sasl_io_state;
} SASL_IO_DATA;

static const IO_INTERFACE_DESCRIPTION sasl_io_interface_description =
{
	saslio_create,
	saslio_destroy,
	saslio_send,
	saslio_dowork
};

const unsigned char sasl_header[] = { 'A', 'M', 'Q', 'P', 3, 1, 0, 0 };

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

			/* send the SASL header */
			result->sasl_io_state = SASL_IO_IDLE;
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
		switch (sasl_io_data->sasl_io_state)
		{
		default:
			break;
		case SASL_IO_IDLE:
			/* send SASL header */
			if (io_send(sasl_io_data->socket_io, sasl_header, sizeof(sasl_header)) != sizeof(sasl_header))
			{
				sasl_io_data->sasl_io_state = SASL_IO_ERROR;
			}
			break;
		}
	}
}

const IO_INTERFACE_DESCRIPTION* saslio_get_interface_description(void)
{
	return &sasl_io_interface_description;
}
