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
	size_t header_bytes_received;
} SASL_IO_DATA;

static const IO_INTERFACE_DESCRIPTION sasl_io_interface_description =
{
	saslio_create,
	saslio_destroy,
	saslio_send,
	saslio_dowork
};

const unsigned char sasl_header[] = { 'A', 'M', 'Q', 'P', 3, 1, 0, 0 };

static int send_sasl_header(SASL_IO_DATA* sasl_io)
{
	int result;

	if (io_send(sasl_io->socket_io, sasl_header, sizeof(sasl_header)) != sizeof(sasl_header))
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

static int saslio_receive_byte(SASL_IO_DATA* sasl_io, unsigned char b)
{
	int result;

	switch (sasl_io->sasl_io_state)
	{
	default:
		result = __LINE__;
		break;

	case SASL_IO_IDLE:
	case SASL_IO_HEADER_SENT:
		if (b != sasl_header[sasl_io->header_bytes_received])
		{
			sasl_io->sasl_io_state = SASL_IO_ERROR;
			result = __LINE__;
		}
		else
		{
			sasl_io->header_bytes_received++;
			if (sasl_io->header_bytes_received == sizeof(sasl_header))
			{
				switch (sasl_io->sasl_io_state)
				{
				case SASL_IO_HEADER_EXCH:
				case SASL_IO_HEADER_RCVD:
				default:
					sasl_io->sasl_io_state = SASL_IO_ERROR;
					result = __LINE__;
					break;
				
				case SASL_IO_HEADER_SENT:
					/* from this point on we simply pass down all calls */
					sasl_io->sasl_io_state = SASL_IO_HEADER_EXCH;
					result = 0;
					break;

				case SASL_IO_IDLE:
					sasl_io->sasl_io_state = SASL_IO_HEADER_RCVD;
					result = send_sasl_header(sasl_io);
					break;
				}
			}
			else
			{
				result = 0;
			}
		}
		break;

		break;
	}

	return result;
}

static void saslio_receive_bytes(void* context, const void* buffer, size_t size)
{
	SASL_IO_DATA* sasl_io_data = (SASL_IO_DATA*)context;
	size_t i;

	for (i = 0; i < size; i++)
	{
		if (saslio_receive_byte(sasl_io_data, ((unsigned char*)buffer)[i]) != 0)
		{
			break;
		}
	}
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
			result->socket_io = io_create(sasl_io_config->socket_io_interface, sasl_io_config->socket_io_parameters, saslio_receive_bytes, result, logger_log);
			if (result->socket_io == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->receive_callback = NULL;
				result->logger_log = logger_log;
				result->receive_callback = receive_callback;
				result->context = context;
				result->header_bytes_received = 0;

				/* send the SASL header */
				result->sasl_io_state = SASL_IO_IDLE;
			}
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
		result = 0;
	}

	return result;
}

void saslio_dowork(IO_HANDLE sasl_io)
{
	if (sasl_io != NULL)
	{
		SASL_IO_DATA* sasl_io_instance = (SASL_IO_DATA*)sasl_io;
		switch (sasl_io_instance->sasl_io_state)
		{
		default:
			break;
		case SASL_IO_IDLE:
			/* send SASL header */
			if (send_sasl_header(sasl_io_instance) != 0)
			{
				sasl_io_instance->sasl_io_state = SASL_IO_ERROR;
			}
			else
			{
				sasl_io_instance->sasl_io_state = SASL_IO_HEADER_SENT;
			}
			break;
		}
	}
}

const IO_INTERFACE_DESCRIPTION* saslio_get_interface_description(void)
{
	return &sasl_io_interface_description;
}
