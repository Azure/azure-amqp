#include "session_manager.h"
#include "amqpalloc.h"
#include "connection.h"

typedef struct SESSION_ENDPOINT_INSTANCE_TAG
{
	uint16_t outgoing_channel;
	uint16_t incoming_channel;
	SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback;
	SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback;
	void* frame_received_callback_context;
} SESSION_ENDPOINT_INSTANCE;

typedef struct SESSION_MANAGER_INSTANCE_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_ENDPOINT_INSTANCE* session_endpoints;
	uint32_t session_endpoint_count;
} SESSION_MANAGER_INSTANCE;

static SESSION_ENDPOINT_INSTANCE* find_session_endpoint_by_outgoing_channel(SESSION_MANAGER_INSTANCE* session_manager, uint16_t outgoing_channel)
{
	uint32_t i;
	SESSION_ENDPOINT_INSTANCE* result;

	for (i = 0; i < session_manager->session_endpoint_count; i++)
	{
		if (session_manager->session_endpoints[i].outgoing_channel == outgoing_channel)
		{
			break;
		}
	}

	if (i == session_manager->session_endpoint_count)
	{
		result = NULL;
	}
	else
	{
		result = &session_manager->session_endpoints[i];
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
		SESSION_ENDPOINT_INSTANCE* session_endpoint = find_session_endpoint_by_outgoing_channel(session_manager_instance, 0);
		if (session_endpoint == NULL)
		{
			/* error */
		}
		else
		{
			session_endpoint->incoming_channel = channel;
		}

		break;
	}
	}
}

static void frame_payload_bytes_received(void* context, const unsigned char* payload_bytes, uint32_t byte_count)
{
}

SESSION_MANAGER_HANDLE session_manager_create(const char* host, int port, CONNECTION_OPTIONS* options)
{
	SESSION_MANAGER_INSTANCE* result = amqpalloc_malloc(sizeof(SESSION_MANAGER_INSTANCE));
	if (result != NULL)
	{
		result->session_endpoints = NULL;
		result->session_endpoint_count = 0;

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

SESSION_ENDPOINT_HANDLE session_manager_create_endpoint(SESSION_MANAGER_HANDLE session_manager, SESSION_ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, SESSION_ENDPOINT_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK frame_payload_bytes_received_callback, void* context)
{
	SESSION_ENDPOINT_INSTANCE* result;

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

			result = amqpalloc_malloc(sizeof(SESSION_ENDPOINT_INSTANCE));
			if (result != NULL)
			{
				result->frame_received_callback = frame_received_callback;
				result->frame_payload_bytes_received_callback = frame_payload_bytes_received_callback;
				result->frame_received_callback_context = context;
				result->outgoing_channel = channel_no;
			}
		}
	}

	return result;
}

void session_manager_destroy_endpoint(SESSION_ENDPOINT_HANDLE session_endpoint)
{
	if (session_endpoint != NULL)
	{
		SESSION_ENDPOINT_INSTANCE* session_endpoint_instance = (SESSION_ENDPOINT_INSTANCE*)session_endpoint;
		amqpalloc_free(session_endpoint_instance);
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
