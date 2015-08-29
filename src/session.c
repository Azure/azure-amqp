#include "session.h"
#include "connection.h"
#include "amqpalloc.h"

typedef struct SESSION_INSTANCE_TAG
{
	uint16_t outgoing_channel;
	uint16_t incoming_channel;
	SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback;
	SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback;
	void* frame_received_callback_context;
	SESSION_STATE session_state;
	CONNECTION_HANDLE connection;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec;
} SESSION_INSTANCE;

static int send_begin(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
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
			if (amqp_frame_codec_begin_encode_frame(amqp_frame_codec, 0, begin_performative_value, 0) != 0)
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

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_INSTANCE* result = amqpalloc_malloc(sizeof(SESSION_INSTANCE));
	if (result != NULL)
	{
		result->connection = connection;
		result->session_state = SESSION_STATE_UNMAPPED;
	}

	return result;
}

void session_destroy(SESSION_HANDLE session)
{
	if (session != NULL)
	{
		free(session);
	}
}

AMQP_FRAME_CODEC_HANDLE session_get_amqp_frame_codec(SESSION_HANDLE session)
{
	AMQP_FRAME_CODEC_HANDLE result;

	if (session == NULL)
	{
		result = NULL;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		result = connection_get_amqp_frame_codec(session_instance->connection);
	}

	return result;
}

int session_set_frame_received_callback(SESSION_HANDLE session, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, void* context)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		session_instance->frame_received_callback = frame_received_callback;
		session_instance->frame_received_callback_context = context;
		result = 0;
	}

	return result;
}

int session_get_outgoing_channel(SESSION_HANDLE session, uint16_t* outgoing_channel)
{
	SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
	*outgoing_channel = session_instance->outgoing_channel;
	return 0;
}

void session_dowork(SESSION_HANDLE session)
{
	SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
	switch (session_instance->session_state)
	{
	default:
		break;

	case SESSION_STATE_BEGIN_SENT:
	case SESSION_STATE_MAPPED:
		break;

	case SESSION_STATE_UNMAPPED:
	{
		CONNECTION_STATE connection_state;

		if (connection_get_state(session_instance->connection, &connection_state) == 0)
		{
			if (connection_state == CONNECTION_STATE_OPENED)
			{
				AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = connection_get_amqp_frame_codec(session_instance->connection);
				if ((amqp_frame_codec != NULL) &&
					(send_begin(amqp_frame_codec, 0, 1, 1) == 0))
				{
					session_instance->session_state = SESSION_STATE_BEGIN_SENT;
				}
			}
		}

		break;
	}
	}
}

int session_get_state(SESSION_HANDLE session, SESSION_STATE* session_state)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		*session_state = session_instance->session_state;
		result = 0;
	}

	return result;
}