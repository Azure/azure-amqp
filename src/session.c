#include <string.h>
#include "session.h"
#include "connection.h"
#include "amqpalloc.h"
#include "consolelogger.h"
#include "logger.h"

typedef struct LINK_ENDPOINT_INSTANCE_TAG
{
	char* name;
	handle incoming_handle;
	handle outgoing_handle;
	ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* frame_received_callback_context;
	SESSION_HANDLE session;
} LINK_ENDPOINT_INSTANCE;

typedef struct SESSION_INSTANCE_TAG
{
	ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* frame_received_callback_context;
	SESSION_STATE session_state;
	CONNECTION_HANDLE connection;
	ENDPOINT_HANDLE endpoint;
	LINK_ENDPOINT_INSTANCE* link_endpoints;
	uint32_t link_endpoint_count;
	delivery_number delivery_id;

	uint32_t handle_max;
} SESSION_INSTANCE;

static int send_begin(ENDPOINT_HANDLE endpoint, transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
{
	int result;
	BEGIN_HANDLE begin = begin_create(1, next_outgoing_id, incoming_window, outgoing_window);

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
			if (connection_encode_frame(endpoint, begin_performative_value, NULL, 0) != 0)
			{
				result = __LINE__;
			}
			else
			{
				LOG(consolelogger_log, LOG_LINE, "-> [BEGIN]");

				result = 0;
			}

			amqpvalue_destroy(begin_performative_value);
		}
	}

	return result;
}

static LINK_ENDPOINT_INSTANCE* find_link_endpoint_by_name(SESSION_INSTANCE* session, const char* name)
{
	uint32_t i;
	LINK_ENDPOINT_INSTANCE* result;

	for (i = 0; i < session->link_endpoint_count; i++)
	{
		/* what do we compare with ? */
		if (strcmp(session->link_endpoints[i].name, name) == 0)
		{
			break;
		}
	}

	if (i == session->link_endpoint_count)
	{
		result = NULL;
	}
	else
	{
		result = &session->link_endpoints[i];
	}

	return result;
}

static LINK_ENDPOINT_INSTANCE* find_link_endpoint_by_incoming_handle(SESSION_INSTANCE* session, handle incoming_handle)
{
	uint32_t i;
	LINK_ENDPOINT_INSTANCE* result;

	for (i = 0; i < session->link_endpoint_count; i++)
	{
		/* what do we compare with ? */
		if (session->link_endpoints[i].incoming_handle == incoming_handle)
		{
			break;
		}
	}

	if (i == session->link_endpoint_count)
	{
		result = NULL;
	}
	else
	{
		result = &session->link_endpoints[i];
	}

	return result;
}

static void session_frame_received(void* context, AMQP_VALUE performative, uint32_t payload_size, const unsigned char* payload_bytes)
{
	SESSION_INSTANCE* session = (SESSION_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
	uint64_t performative_ulong;

	amqpvalue_get_ulong(descriptor, &performative_ulong);
	switch (performative_ulong)
	{
	default:
		break;

	case AMQP_BEGIN:
		LOG(consolelogger_log, LOG_LINE, "<- [BEGIN]");
		session->session_state = SESSION_STATE_MAPPED;
		break;

	case AMQP_ATTACH:
	{
		const char* name = NULL;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE name_value = amqpvalue_get_list_item(described_value, 0);
		amqpvalue_get_string(name_value, &name);

		LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_name(session, name);
		if (link_endpoint == NULL)
		{
			/* error */
		}
		else
		{
			link_endpoint->incoming_handle = 0;
			link_endpoint->frame_received_callback(link_endpoint->frame_received_callback_context, performative, payload_size, payload_bytes);
		}

		break;
	}

	case AMQP_DETACH:
	{
		uint32_t remote_handle;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE handle_value = amqpvalue_get_list_item(described_value, 0);
		amqpvalue_get_uint(handle_value, &remote_handle);
		LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_incoming_handle(session, remote_handle);
		if (link_endpoint == NULL)
		{
			/* error */
		}
		else
		{
			link_endpoint->incoming_handle = 0;
			link_endpoint->frame_received_callback(link_endpoint->frame_received_callback_context, performative, payload_size, payload_bytes);
		}

		break;
	}

	case AMQP_FLOW:
	{
		uint32_t remote_handle;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE handle_value = amqpvalue_get_list_item(described_value, 5);
		if ((handle_value != NULL) &&
			(amqpvalue_get_uint(handle_value, &remote_handle) == 0))
		{
			LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_incoming_handle(session, remote_handle);
			if (link_endpoint == NULL)
			{
				/* error */
			}
			else
			{
				link_endpoint->incoming_handle = 0;
				link_endpoint->frame_received_callback(link_endpoint->frame_received_callback_context, performative, payload_size, payload_bytes);
			}
		}

		LOG(consolelogger_log, LOG_LINE, "<- [FLOW]");
		break;
	}

	case AMQP_TRANSFER:
	{
		uint32_t remote_handle;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE handle_value = amqpvalue_get_list_item(described_value, 0);
		amqpvalue_get_uint(handle_value, &remote_handle);
		LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_incoming_handle(session, remote_handle);
		if (link_endpoint == NULL)
		{
			/* error */
		}
		else
		{
			link_endpoint->incoming_handle = 0;
			link_endpoint->frame_received_callback(link_endpoint->frame_received_callback_context, performative, payload_size, payload_bytes);
		}

		LOG(consolelogger_log, LOG_LINE, "<- [TRANSFER]");
		break;
	}

	case AMQP_DISPOSITION:
	{
		uint32_t i;
		for (i = 0; i < session->link_endpoint_count; i++)
		{
			LINK_ENDPOINT_INSTANCE* link_endpoint = &session->link_endpoints[i];
			link_endpoint->frame_received_callback(link_endpoint->frame_received_callback_context, performative, payload_size, payload_bytes);
		}

		break;
	}

	case AMQP_END:
	{
		const char* error = NULL;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE error_value = amqpvalue_get_list_item(described_value, 0);
		AMQP_VALUE error_described_value = amqpvalue_get_described_value(error_value);
		AMQP_VALUE error_description_value = amqpvalue_get_list_item(error_described_value, 1);
		if (error_description_value != NULL)
		{
			amqpvalue_get_string(error_description_value, &error);
		}
		else
		{
			error = NULL;
		}

		LOG(consolelogger_log, LOG_LINE, "<- [END:%s]", error);
		break;
	}
	}
}

static void session_frame_payload_bytes_received(void* context, const unsigned char* payload_bytes, uint32_t byte_count)
{
	SESSION_INSTANCE* session = (SESSION_INSTANCE*)context;
}

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_INSTANCE* result = amqpalloc_malloc(sizeof(SESSION_INSTANCE));
	if (result != NULL)
	{
		result->connection = connection;
		result->session_state = SESSION_STATE_UNMAPPED;
		result->link_endpoints = NULL;
		result->link_endpoint_count = 0;
		result->delivery_id = 0;
		result->endpoint = connection_create_endpoint(connection, session_frame_received, result);

		result->handle_max = 4294967295;
	}

	return result;
}

void session_destroy(SESSION_HANDLE session)
{
	if (session != NULL)
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		if (session_instance->link_endpoints != NULL)
		{
			free(session_instance->link_endpoints);
		}
		free(session);
	}
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
				if (send_begin(session_instance->endpoint, 0, 2000, 200) == 0)
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

int session_encode_frame(SESSION_HANDLE session, const AMQP_VALUE performative, PAYLOAD* payloads, size_t payload_count)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		if (connection_encode_frame(session_instance->endpoint, performative, payloads, payload_count) != 0)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

int session_transfer(SESSION_HANDLE session, TRANSFER_HANDLE transfer, PAYLOAD* payloads, size_t payload_count, delivery_number* delivery_id)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		AMQP_VALUE transfer_value;

		*delivery_id = session_instance->delivery_id++;
		transfer_set_delivery_id(transfer, *delivery_id);
		transfer_value = amqpvalue_create_transfer(transfer);
		if (transfer_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (connection_encode_frame(session_instance->endpoint, transfer_value, payloads, payload_count) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(transfer_value);
		}
	}

	return result;
}

int session_encode_payload_bytes(SESSION_HANDLE session, const unsigned char* bytes, uint32_t count)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		if (connection_encode_payload_bytes(session_instance->endpoint, bytes, count) != 0)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

LINK_ENDPOINT_HANDLE session_create_link_endpoint(SESSION_HANDLE session, const char* name, ENDPOINT_FRAME_RECEIVED_CALLBACK frame_received_callback, void* context)
{
	LINK_ENDPOINT_INSTANCE* result;

	if (session == NULL)
	{
		result = NULL;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;
		handle selected_handle = 0;

		while (selected_handle < session_instance->handle_max)
		{
			break;
			selected_handle++;
		}

		session_instance->link_endpoints = amqpalloc_realloc(session_instance->link_endpoints, sizeof(LINK_ENDPOINT_INSTANCE) * (session_instance->link_endpoint_count + 1));

		session_instance->link_endpoints[session_instance->link_endpoint_count].frame_received_callback = frame_received_callback;
		session_instance->link_endpoints[session_instance->link_endpoint_count].frame_received_callback_context = context;
		session_instance->link_endpoints[session_instance->link_endpoint_count].outgoing_handle = selected_handle;
		session_instance->link_endpoints[session_instance->link_endpoint_count].name = amqpalloc_malloc(strlen(name) + 1);
		strcpy(session_instance->link_endpoints[session_instance->link_endpoint_count].name, name);
		session_instance->link_endpoints[session_instance->link_endpoint_count].session = session;

		result = &session_instance->link_endpoints[session_instance->link_endpoint_count];

		session_instance->link_endpoint_count++;
	}

	return result;
}

void session_destroy_link_endpoint(LINK_ENDPOINT_HANDLE endpoint)
{
	if (endpoint != NULL)
	{
		LINK_ENDPOINT_INSTANCE* endpoint_instance = (LINK_ENDPOINT_INSTANCE*)endpoint;
		if (endpoint_instance->name != NULL)
		{
			amqpalloc_free(endpoint_instance->name);
		}

		amqpalloc_free(endpoint_instance);
	}
}
