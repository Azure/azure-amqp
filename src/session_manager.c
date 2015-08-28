#include "session_manager.h"
#include "amqpalloc.h"
#include "connection.h"

typedef struct SESSION_INSTANCE_TAG
{
	uint16_t outgoing_channel;
	uint16_t incoming_channel;
	SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback;
	SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback;
	void* frame_received_callback_context;
	SESSION_STATE session_state;
	SESSION_MANAGER_HANDLE session_manager;
} SESSION_INSTANCE;

typedef struct SESSION_MANAGER_INSTANCE_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_INSTANCE* sessions;
	uint32_t session_count;
} SESSION_MANAGER_INSTANCE;

static SESSION_INSTANCE* find_session_by_outgoing_channel(SESSION_MANAGER_INSTANCE* session_manager, uint16_t outgoing_channel)
{
	uint32_t i;
	SESSION_INSTANCE* result;

	for (i = 0; i < session_manager->session_count; i++)
	{
		if (session_manager->sessions[i].outgoing_channel == outgoing_channel)
		{
			break;
		}
	}

	if (i == session_manager->session_count)
	{
		result = NULL;
	}
	else
	{
		result = &session_manager->sessions[i];
	}

	return result;
}

static void frame_received(void* context, uint16_t channel, AMQP_VALUE performative, uint32_t frame_payload_size)
{
	AMQP_VALUE descriptor = amqpvalue_get_descriptor(performative);
	uint64_t performative_ulong;
	SESSION_MANAGER_INSTANCE* session_manager_instance = context;

	amqpvalue_get_ulong(descriptor, &performative_ulong);
	switch (performative_ulong)
	{
	default:
		break;

	case AMQP_BEGIN:
	{
		SESSION_INSTANCE* session = find_session_by_outgoing_channel(session_manager_instance, 0);
		if (session == NULL)
		{
			/* error */
		}
		else
		{
			switch (session->session_state)
			{
			default:
				break;

			case SESSION_STATE_BEGIN_SENT:
				session->session_state = SESSION_STATE_MAPPED;
				session->incoming_channel = channel;
				break;
			}
		}

		break;
	}
	}
}

static void frame_payload_bytes_received(void* context, const unsigned char* payload_bytes, uint32_t byte_count)
{
}

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

SESSION_MANAGER_HANDLE session_manager_create(const char* host, int port, CONNECTION_OPTIONS* options)
{
	SESSION_MANAGER_INSTANCE* result = amqpalloc_malloc(sizeof(SESSION_MANAGER_INSTANCE));
	if (result != NULL)
	{
		result->sessions = NULL;
		result->session_count = 0;

		result->connection = connection_create(host, port, options, frame_received, frame_payload_bytes_received, result);
		if (result->connection == NULL)
		{
			amqpalloc_free(result);
			result = NULL;
		}
	}

	return result;
}

void session_manager_destroy(SESSION_MANAGER_HANDLE session_manager)
{
	if (session_manager != NULL)
	{
		SESSION_MANAGER_INSTANCE* session_manager_instance = (SESSION_MANAGER_INSTANCE*)session_manager;
		connection_destroy(session_manager_instance->connection);
		amqpalloc_free(session_manager_instance);
	}
}

SESSION_HANDLE session_manager_create_endpoint(SESSION_MANAGER_HANDLE session_manager, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback, void* context)
{
	SESSION_INSTANCE* result;

	if (session_manager == NULL)
	{
		result = NULL;
	}
	else
	{
		SESSION_MANAGER_INSTANCE* session_manager_instance = (SESSION_MANAGER_INSTANCE*)session_manager;
		uint16_t channel_no = 0;
		uint16_t channel_max;

		if (connection_get_channel_max(session_manager_instance->connection, &channel_max) != 0)
		{
			result = NULL;
		}
		else
		{
			while (channel_no < channel_max)
			{
				break;
				channel_no++;
			}

			session_manager_instance->sessions = amqpalloc_realloc(session_manager_instance->sessions, sizeof(SESSION_INSTANCE) * (session_manager_instance->session_count + 1));

			session_manager_instance->sessions[session_manager_instance->session_count].frame_received_callback = frame_received_callback;
			session_manager_instance->sessions[session_manager_instance->session_count].frame_payload_bytes_received_callback = frame_payload_bytes_received_callback;
			session_manager_instance->sessions[session_manager_instance->session_count].frame_received_callback_context = context;
			session_manager_instance->sessions[session_manager_instance->session_count].outgoing_channel = channel_no;
			session_manager_instance->sessions[session_manager_instance->session_count].session_state = SESSION_STATE_UNMAPPED;
			session_manager_instance->sessions[session_manager_instance->session_count].session_manager = session_manager;

			result = &session_manager_instance->sessions[session_manager_instance->session_count];

			session_manager_instance->session_count++;
		}
	}

	return result;
}

void session_manager_destroy_endpoint(SESSION_HANDLE session)
{
	if (session != NULL)
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		amqpalloc_free(session_instance);
	}
}

AMQP_FRAME_CODEC_HANDLE session_manager_get_amqp_frame_codec(SESSION_MANAGER_HANDLE session_manager)
{
	AMQP_FRAME_CODEC_HANDLE result;

	if (session_manager == NULL)
	{
		result = NULL;
	}
	else
	{
		SESSION_MANAGER_INSTANCE* session_manager_instance = (SESSION_MANAGER_INSTANCE*)session_manager;
		result = connection_get_amqp_frame_codec(session_manager_instance->connection);
	}

	return result;
}

CONNECTION_HANDLE session_manager_get_connection(SESSION_MANAGER_HANDLE session_manager)
{
	CONNECTION_HANDLE result;

	if (session_manager == NULL)
	{
		result = NULL;
	}
	else
	{
		SESSION_MANAGER_INSTANCE* session_manager_instance = (SESSION_MANAGER_INSTANCE*)session_manager;
		result = session_manager_instance->connection;
	}

	return result;
}

int session_get_endpoint_state(SESSION_HANDLE session, SESSION_STATE* session_state)
{
	int result;

	if ((session == NULL) ||
		(session_state == NULL))
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

void session_manager_dowork(SESSION_MANAGER_HANDLE session_manager)
{
	SESSION_MANAGER_INSTANCE* session_manager_instance = (SESSION_MANAGER_INSTANCE*)session_manager;
	uint32_t i;
	AMQP_FRAME_CODEC_HANDLE amqp_frame_codec = session_manager_get_amqp_frame_codec(session_manager);

	for (i = 0; i < session_manager_instance->session_count; i++)
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)&session_manager_instance->sessions[i];
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
			if (connection_get_state(session_manager_instance->connection, &connection_state) == 0)
			{
				if (connection_state == CONNECTION_STATE_OPENED)
				{
					if (send_begin(amqp_frame_codec, 0, 1, 1) == 0)
					{
						session_instance->session_state = SESSION_STATE_BEGIN_SENT;
					}
				}
			}
			break;
		}
		}
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
		result = session_manager_get_amqp_frame_codec(session_instance->session_manager);
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
