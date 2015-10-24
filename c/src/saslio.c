#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include "saslio.h"
#include "amqpalloc.h"
#include "frame_codec.h"
#include "sasl_frame_codec.h"
#include "amqp_definitions.h"
#include "logger.h"

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
	ON_BYTES_RECEIVED on_bytes_received;
	ON_IO_STATE_CHANGED on_io_state_changed;
	LOGGER_LOG logger_log;
	void* callback_context;
	SASL_IO_STATE sasl_io_state;
	SASL_CLIENT_NEGOTIATION_STATE sasl_client_negotiation_state;
	size_t header_bytes_received;
	SASL_FRAME_CODEC_HANDLE sasl_frame_codec;
	FRAME_CODEC_HANDLE frame_codec;
	IO_STATE io_state;
	SASL_MECHANISM_HANDLE sasl_mechanism;
} SASL_IO_INSTANCE;

static const IO_INTERFACE_DESCRIPTION sasl_io_interface_description =
{
	saslio_create,
	saslio_destroy,
	saslio_open,
	saslio_close,
	saslio_send,
	saslio_dowork
};

const unsigned char sasl_header[] = { 'A', 'M', 'Q', 'P', 3, 1, 0, 0 };

static void set_io_state(SASL_IO_INSTANCE* sasl_io_instance, IO_STATE io_state)
{
	IO_STATE previous_state = sasl_io_instance->io_state;
	sasl_io_instance->io_state = io_state;
	if (sasl_io_instance->on_io_state_changed != NULL)
	{
		sasl_io_instance->on_io_state_changed(sasl_io_instance->callback_context, io_state, previous_state);
	}
}

static int send_sasl_header(SASL_IO_INSTANCE* sasl_io_instance)
{
	int result;

	if (io_send(sasl_io_instance->socket_io, sasl_header, sizeof(sasl_header)) != 0)
	{
		result = __LINE__;
	}
	else
	{
		LOG(sasl_io_instance->logger_log, LOG_LINE, "-> Header (AMQP 3.1.0.0)");

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
			if (sasl_io->on_bytes_received != NULL)
			{
				sasl_io->on_bytes_received(sasl_io->callback_context, &b, 1);
			}
			result = 0;
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
				LOG(sasl_io->logger_log, LOG_LINE, "<- Header (AMQP 3.1.0.0)");

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
	}

	return result;
}

static void saslio_on_bytes_received(void* context, const void* buffer, size_t size)
{
	SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)context;

	if ((sasl_io_instance->sasl_io_state == SASL_IO_HEADER_EXCH) &&
		(sasl_io_instance->sasl_client_negotiation_state == SASL_CLIENT_NEGOTIATION_OUTCOME_RCVD))
	{
		/* simply pass bytes to the upper layer */
		if (sasl_io_instance->on_bytes_received != NULL)
		{
			sasl_io_instance->on_bytes_received(sasl_io_instance->callback_context, buffer, size);
		}
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

static int send_sasl_init(SASL_IO_INSTANCE* sasl_io)
{
	int result;

	const char* sasl_mechanism_name = saslmechanism_get_mechanism_name(sasl_io->sasl_mechanism);
	SASL_INIT_HANDLE sasl_init;
	INIT_BYTES init_bytes;

	if ((sasl_mechanism_name == NULL) ||
		((sasl_init = sasl_init_create(sasl_mechanism_name)) == NULL))
	{
		result = __LINE__;
	}
	else
	{
		if (saslmechanism_get_init_bytes(sasl_io->sasl_mechanism, &init_bytes) != 0)
		{
			result = __LINE__;
		}
		else
		{
			amqp_binary creds = { init_bytes.bytes, init_bytes.length };
			if (sasl_init_set_initial_response(sasl_init, creds) != 0)
			{
				result = __LINE__;
			}
			else
			{
				AMQP_VALUE sasl_init_value = amqpvalue_create_sasl_init(sasl_init);
				if (sasl_init_value == NULL)
				{
					result = __LINE__;
				}
				else
				{
					if (sasl_frame_codec_encode_frame(sasl_io->sasl_frame_codec, sasl_init_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						LOG(sasl_io->logger_log, LOG_LINE, "-> [SASL_INIT]");

						result = 0;
					}

					amqpvalue_destroy(sasl_init_value);
				}
			}
		}

		sasl_init_destroy(sasl_init);
	}

	return result;
}

static void saslio_on_io_state_changed(void* context, IO_STATE new_io_state, IO_STATE previous_io_state)
{
	SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)context;

	if (new_io_state == IO_STATE_OPEN)
	{
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

		case SASL_IO_HEADER_RCVD:
			if (send_sasl_header(sasl_io_instance) != 0)
			{
				sasl_io_instance->sasl_io_state = SASL_IO_ERROR;
			}
			else
			{
				sasl_io_instance->sasl_io_state = SASL_IO_HEADER_EXCH;
			}
			break;

		case SASL_IO_HEADER_EXCH:
			switch (sasl_io_instance->sasl_client_negotiation_state)
			{
			default:
				break;

			case SASL_CLIENT_NEGOTIATION_NOT_STARTED:
			case SASL_CLIENT_NEGOTIATION_INIT_SENT:
			case SASL_CLIENT_NEGOTIATION_RESPONSE_SENT:
				/* do nothing, just wait */
				break;

			case SASL_CLIENT_NEGOTIATION_MECH_RCVD:
				if (send_sasl_init(sasl_io_instance) != 0)
				{
					sasl_io_instance->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_ERROR;
				}
				else
				{
					sasl_io_instance->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_INIT_SENT;
				}
				break;

			case SASL_CLIENT_NEGOTIATION_CHALLENGE_RCVD:
				/* we should send the response here */
				break;

			case SASL_CLIENT_NEGOTIATION_OUTCOME_RCVD:
				/* SASL negotiated, simply do nothing*/
				break;
			}
			break;
		}
	}
}

static void sasl_frame_received_callback(void* context, AMQP_VALUE sasl_frame)
{
	SASL_IO_INSTANCE* sasl_io = (SASL_IO_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(sasl_frame);
	uint64_t sasl_frame_code_ulong;

	amqpvalue_get_ulong(descriptor, &sasl_frame_code_ulong);
	switch (sasl_frame_code_ulong)
	{
	default:
		LOG(sasl_io->logger_log, LOG_LINE, "Bad SASL frame: %02x", sasl_frame_code_ulong);
		break;

	case SASL_MECHANISMS:
		LOG(sasl_io->logger_log, LOG_LINE, "<- [SASL_MECHANISMS]");
		switch (sasl_io->sasl_client_negotiation_state)
		{
		case SASL_CLIENT_NEGOTIATION_NOT_STARTED:
			sasl_io->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_MECH_RCVD;
			if (send_sasl_init(sasl_io) != 0)
			{
				sasl_io->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_ERROR;
			}
			else
			{
				sasl_io->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_INIT_SENT;
			}
			break;
		}
		break;
	case SASL_CHALLENGE:
		/* we should send the response here */
		LOG(sasl_io->logger_log, LOG_LINE, "<- [SASL_CHALLENGE]");
		break;
	case SASL_OUTCOME:
		LOG(sasl_io->logger_log, LOG_LINE, "<- [SASL_OUTCOME]");
		if (sasl_io->sasl_client_negotiation_state != SASL_CLIENT_NEGOTIATION_ERROR)
		{
			sasl_io->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_OUTCOME_RCVD;
			set_io_state(sasl_io, IO_STATE_OPEN);
		}

		break;
	}
}

static void frame_codec_error(void* context)
{
}

static void sasl_frame_codec_error(void* context)
{
}

IO_HANDLE saslio_create(void* io_create_parameters, LOGGER_LOG logger_log)
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
			result->socket_io = io_create(sasl_io_config->socket_io_interface, sasl_io_config->socket_io_parameters, logger_log);
			if (result->socket_io == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->frame_codec = frame_codec_create(result->socket_io, frame_codec_error, result, logger_log);
				if (result->frame_codec == NULL)
				{
					io_destroy(result->socket_io);
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					result->sasl_frame_codec = sasl_frame_codec_create(result->frame_codec, sasl_frame_received_callback, sasl_frame_codec_error, result);
					if (result->sasl_frame_codec == NULL)
					{
						frame_codec_destroy(result->frame_codec);
						io_destroy(result->socket_io);
						amqpalloc_free(result);
						result = NULL;
					}
					else
					{
						result->on_bytes_received = NULL;
						result->on_io_state_changed = NULL;
						result->logger_log = logger_log;
						result->callback_context = NULL;
						result->header_bytes_received = 0;
						result->sasl_mechanism = sasl_io_config->sasl_mechanism;

						result->sasl_io_state = SASL_IO_IDLE;
						result->sasl_client_negotiation_state = SASL_CLIENT_NEGOTIATION_NOT_STARTED;
						set_io_state(result, IO_STATE_NOT_OPEN);
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

int saslio_open(IO_HANDLE sasl_io, ON_BYTES_RECEIVED on_bytes_received, ON_IO_STATE_CHANGED on_io_state_changed, void* callback_context)
{
	int result = 0;

	if (sasl_io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;

		sasl_io_instance->on_bytes_received = on_bytes_received;
		sasl_io_instance->on_io_state_changed = on_io_state_changed;
		sasl_io_instance->callback_context = callback_context;

		if (io_open(sasl_io_instance->socket_io, saslio_on_bytes_received, saslio_on_io_state_changed, sasl_io_instance) != 0)
		{
			result = __LINE__;
		}
		else
		{
			set_io_state(sasl_io_instance, IO_STATE_OPENING);
			result = 0;
		}
	}
	
	return result;
}

int saslio_close(IO_HANDLE sasl_io)
{
	int result = 0;

	if (sasl_io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;
		if (io_close(sasl_io_instance->socket_io) != 0)
		{
			result = __LINE__;
		}
		else
		{
			set_io_state(sasl_io_instance, IO_STATE_NOT_OPEN);
			result = 0;
		}
	}

	return result;
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
		if (sasl_io_instance->io_state != IO_STATE_OPEN)
		{
			result = __LINE__;
		}
		else
		{
			if (io_send(sasl_io_instance->socket_io, buffer, size) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

void saslio_dowork(IO_HANDLE sasl_io)
{
	if (sasl_io != NULL)
	{
		SASL_IO_INSTANCE* sasl_io_instance = (SASL_IO_INSTANCE*)sasl_io;
		io_dowork(sasl_io_instance->socket_io);
	}
}

const IO_INTERFACE_DESCRIPTION* saslio_get_interface_description(void)
{
	return &sasl_io_interface_description;
}
