#include <stdlib.h>
#include <stdint.h>
#include "session.h"
#include "connection.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"
#include "consolelogger.h"
#include "amqpalloc.h"
#include "amqp_frame_codec.h"

typedef struct SESSION_DATA_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_STATE session_state;
	uint16_t channel_no;
	SESSION_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* frame_received_callback_context;
} SESSION_DATA;

static int send_begin(SESSION_DATA* session_data, transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
{
	int result;
	BEGIN_HANDLE begin = begin_create(next_outgoing_id, incoming_window, outgoing_window);
	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE begin_performative_value = amqpvalue_create_begin(begin);
		if (begin_performative_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = connection_get_amqp_frame_codec(session_data->connection);
			if (amqp_frame_codec == NULL)
			{
				result = __LINE__;
			}
			else if (amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, begin_performative_value, 0) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(begin_performative_value);
		}
	}

	return result;
}

static int frame_received(void* context, uint16_t channel, AMQP_VALUE performative, uint32_t frame_payload_size)
{
	SESSION_DATA* session = (SESSION_DATA*)context;
	uint64_t performative_descriptor;

	if (amqpvalue_get_ulong(amqpvalue_get_descriptor(performative), &performative_descriptor) != 0)
	{
	}
	else
	{
		switch (performative_descriptor)
		{
		default:
			consolelogger_log("Bad performative: %llu", (unsigned long long)performative);
			break;

		case 0x11:
		{
			switch (session->session_state)
			{
			default:
				break;

			case SESSION_STATE_BEGIN_SENT:
				session->session_state = SESSION_STATE_MAPPED;
				break;
			}
			break;
		}

		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
			if (session->frame_received_callback != NULL)
			{
				session->frame_received_callback(session->frame_received_callback_context, performative_descriptor, amqpvalue_get_described_value(performative));
			}
			break;
		}
	}

	return 0;
}

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_DATA* result = amqpalloc_malloc(sizeof(SESSION_DATA));
	if (result != NULL)
	{
		uint16_t channel_no;

		if (connection_register_session(connection, frame_received, result, &channel_no) != 0)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			result->channel_no = channel_no;
			result->frame_received_callback = NULL;
			result->session_state = SESSION_STATE_UNMAPPED;
			result->connection = connection;
		}
	}

	return result;
}

void session_destroy(SESSION_HANDLE handle)
{
	amqpalloc_free(handle);
}

void session_dowork(SESSION_HANDLE handle)
{
	SESSION_DATA* session_data = (SESSION_DATA*)handle;

	switch (session_data->session_state)
	{
		default:
			break;

		case SESSION_STATE_BEGIN_SENT:
		case SESSION_STATE_MAPPED:
			break;

		case SESSION_STATE_UNMAPPED:
		{
			CONNECTION_STATE connection_state;
			if (connection_get_state(session_data->connection, &connection_state) == 0)
			{
				if (connection_state == CONNECTION_STATE_OPENED)
				{
					if (send_begin(session_data, 0, 1, 1) == 0)
					{
						session_data->session_state = SESSION_STATE_BEGIN_SENT;
					}
				}
			}
			break;
		}
	}
}

int session_set_frame_received_callback(SESSION_HANDLE handle, SESSION_FRAME_RECEIVED_CALLBACK callback, void* context)
{
	int result;

	SESSION_DATA* session = (SESSION_DATA*)handle;
	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		session->frame_received_callback = callback;
		session->frame_received_callback_context = context;
		result = 0;
	}

	return result;
}

int session_get_state(SESSION_HANDLE handle, SESSION_STATE* session_state)
{
	int result;

	SESSION_DATA* session = (SESSION_DATA*)handle;
	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		*session_state = session->session_state;
		result = 0;
	}

	return result;
}

AMQP_FRAME_CODEC_HANDLE session_get_amqp_frame_codec(SESSION_HANDLE handle)
{
	AMQP_FRAME_CODEC_HANDLE result;

	SESSION_DATA* session = (SESSION_DATA*)handle;
	if (session == NULL)
	{
		result = NULL;
	}
	else
	{
		result = connection_get_amqp_frame_codec(session->connection);
	}

	return result;
}
