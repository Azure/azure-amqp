#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "saslio.h"
#include "amqpalloc.h"
#include "frame_codec.h"
#include "sasl_frame_codec.h"

typedef enum SASL_IO_STATE_TAG
{
	SASL_IO_IDLE,
	SASL_IO_HEADER_SENT,
	SASL_IO_HEADER_RCVD,
	SASL_IO_HEADER_EXCH,
	SASL_IO_ERROR
} SASL_IO_STATE;

typedef enum SASL_CLIENT_NEGOTIATION_STATE_TAG
{
	SASL_CLIENT_NEGOTIATION_NOT_STARTED,
	SASL_CLIENT_NEGOTIATION_MECH_RCVD,
	SASL_CLIENT_NEGOTIATION_INIT_SENT,
	SASL_CLIENT_NEGOTIATION_CHALLENGE_RCVD,
	SASL_CLIENT_NEGOTIATION_RESPONSE_SENT,
	SASL_CLIENT_NEGOTIATION_OUTCOME_RCVD,
	SASL_CLIENT_NEGOTIATION_ERROR
} SASL_CLIENT_NEGOTIATION_STATE;

typedef struct SASL_IO_INSTANCE_TAG
{
	IO_HANDLE socket_io;
	IO_RECEIVE_CALLBACK receive_callback;
	LOGGER_LOG logger_log;
	void* context;
	SASL_IO_STATE sasl_io_state;
	SASL_CLIENT_NEGOTIATION_STATE sasl_client_negotiation_state;
	size_t header_bytes_received;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec;
	FRAME_CODEC_HANDLE frame_codec;
} SASL_IO_INSTANCE;

static const IO_INTERFACE_DESCRIPTION sasl_io_interface_description =
{
	saslio_create,
	saslio_destroy,
	saslio_send,
	saslio_dowork,
	saslio_get_state
};

const unsigned char sasl_header[] = { 'A', 'M', 'Q', 'P', 3, 1, 0, 0 };

static int send_sasl_header(SASL_IO_INSTANCE* sasl_io)
{
	int result;

	if (io_send(sasl_io->socket_io, sasl_header, sizeof(sasl_header)) != 0)
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

static int saslio_receive_byte(SASL_IO_INSTANCE* sasl_io, unsigned char b)
{
	int result;

	switch (sasl_io->sasl_io_state)
	{
	default:
		result = __LINE__;
		break;

	case SASL_IO_HEADER_EXCH:
		switch (sasl_io->sasl_client_negotiation_state)
		{
		case SASL_CLIENT_NEGOTIATION_ERROR:
			result = __LINE__;
			break;

		default:
			/* pass bytes to the sasl frame decoder */
			if (frame_codec_receive_bytes(sasl_io->frame_codec, &b, 1) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			break;

		case SASL_CLIENT_NEGOTIATION_OUTCOME_RCVD:
			/* simply pass bytes to the upper layer */
			sasl_io->receive_callback(sasl_io->context, &b, 1);
			break;
		}

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
				default:
					sasl_io->sasl_io_state = SASL_IO_ERROR;
					result = __LINE__;
					break;
				
				case SASL_IO_HEADER_SENT:
					/* from this point on we need to decode SASL frames */
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
	SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)context;

	if ((sasl_io_instance->sasl_io_state == SASL_IO_HEADER_EXCH) &&
		(sasl_io_instance->sasl_client_negotiation_state == SASL_CLIENT_NEGOTIATION_OUTCOME_RCVD))
	{
		/* simply pass bytes to the upper layer */
		sasl_io_instance->receive_callback(sasl_io_instance->context, buffer, size);
	}
	else
	{
		size_t i;

		for (i = 0; i < size; i++)
		{
			if (saslio_receive_byte(sasl_io_instance, ((unsigned char*)buffer)[i]) != 0)
			{
				break;
			}
		}
	}
}

static void sasl_frame_received_callback(void* context, AMQP_VALUE sasl_frame)
{

}

IO_HANDLE saslio_create(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* context, LOGGER_LOG logger_log)
{
	SASLIO_CONFIG* sasl_io_config = io_create_parameters;
	SASL_IO_INSTANCE* result;

	if (sasl_io_config == NULL)
	{
		result = NULL;
	}
	else
	{
		result = amqpalloc_malloc(sizeof(SASL_IO_INSTANCE));
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
				result->frame_codec = frame_codec_create(result->socket_io, logger_log);
				if (result->frame_codec == NULL)
				{
					io_destroy(result->socket_io);
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					result->sasl_frame_codec = sasl_frame_codec_create(result->frame_codec, sasl_frame_received_callback, result);
					if (result->sasl_frame_codec == NULL)
					{
						frame_codec_destroy(result->frame_codec);
						io_destroy(result->socket_io);
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

						result->sasl_io_state = SASL_IO_IDLE;
						result->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_NOT_STARTED;
					}
				}
			}
		}
	}

	return result;
}

void saslio_destroy(IO_HANDLE sasl_io)
{
	if (sasl_io != NULL)
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;
		sasl_frame_codec_destroy(sasl_io_instance->sasl_frame_codec);
		frame_codec_destroy(sasl_io_instance->frame_codec);
		io_destroy(sasl_io_instance->socket_io);
		amqpalloc_free(sasl_io);
	}
}

int saslio_send(IO_HANDLE sasl_io, const void* buffer, size_t size)
{
	int result;

	if ((sasl_io == NULL) ||
		(buffer == NULL) ||
		(size == 0))
	{
		/* Invalid arguments */
		result = __LINE__;
	}
	else
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;
		result = 0;
	}

	return result;
}

void saslio_dowork(IO_HANDLE sasl_io)
{
	if (sasl_io != NULL)
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;
		io_dowork(sasl_io_instance->socket_io);

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

IO_STATE saslio_get_state(IO_HANDLE sasl_io)
{
	IO_STATE result;

	if (sasl_io == NULL)
	{
		result = IO_STATE_ERROR;
	}
	else
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;
		switch (sasl_io_instance->sasl_io_state)
		{
		default:
			result = IO_STATE_ERROR;
			break;

		case SASL_IO_HEADER_EXCH:
			result = IO_STATE_READY;
			break;

		case SASL_IO_IDLE:
		case SASL_IO_HEADER_SENT:
		case SASL_IO_HEADER_RCVD:
			result = IO_STATE_NOT_READY;
			break;
		}
	}

	return result;
}

const IO_INTERFACE_DESCRIPTION* saslio_get_interface_description(void)
{
	return &sasl_io_interface_description;
}
