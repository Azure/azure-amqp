#include <string.h>
#include "session.h"
#include "connection.h"
#include "amqpalloc.h"
#include "consolelogger.h"
#include "logger.h"

typedef struct PENDING_TRANSFER_TAG
{
	AMQP_VALUE transfer_amqp_value;
	unsigned char* payload_bytes;
	uint32_t payload_length;
} PENDING_TRANSFER;

typedef struct LINK_ENDPOINT_INSTANCE_TAG
{
	char* name;
	handle incoming_handle;
	handle outgoing_handle;
	ON_ENDPOINT_FRAME_RECEIVED frame_received_callback;
	ON_SESSION_STATE_CHANGED on_session_state_changed;
	void* callback_context;
	SESSION_HANDLE session;
} LINK_ENDPOINT_INSTANCE;

typedef struct SESSION_INSTANCE_TAG
{
	ON_ENDPOINT_FRAME_RECEIVED frame_received_callback;
	void* frame_received_callback_context;
	SESSION_STATE session_state;
	CONNECTION_HANDLE connection;
	ENDPOINT_HANDLE endpoint;
	LINK_ENDPOINT_INSTANCE** link_endpoints;
	uint32_t link_endpoint_count;

	/* Codes_SRS_SESSION_01_016: [next-outgoing-id The next-outgoing-id is the transfer-id to assign to the next transfer frame.] */
	transfer_number next_outgoing_id;
	transfer_number next_incoming_id;
	uint32_t incoming_window;
	uint32_t outgoing_window;
	handle handle_max;
	uint32_t remote_incoming_window;
	uint32_t remote_outgoing_window;
} SESSION_INSTANCE;

static void session_set_state(SESSION_INSTANCE* session_instance, SESSION_STATE session_state)
{
	uint64_t i;

	SESSION_STATE previous_state = session_instance->session_state;
	session_instance->session_state = session_state;

	for (i = 0; i < session_instance->link_endpoint_count; i++)
	{
		session_instance->link_endpoints[i]->on_session_state_changed(session_instance->link_endpoints[i]->callback_context, session_state, previous_state);
	}
}

static int send_begin(SESSION_INSTANCE* session_instance)
{
	int result;
	BEGIN_HANDLE begin = begin_create(session_instance->next_outgoing_id, session_instance->incoming_window, session_instance->outgoing_window);

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if (begin_set_handle_max(begin, session_instance->handle_max) != 0)
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
				if (connection_encode_frame(session_instance->endpoint, begin_performative_value, NULL, 0) != 0)
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

		begin_destroy(begin);
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
		if (strcmp(session->link_endpoints[i]->name, name) == 0)
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
		result = session->link_endpoints[i];
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
		if (session->link_endpoints[i]->incoming_handle == incoming_handle)
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
		result = session->link_endpoints[i];
	}

	return result;
}

static void on_connection_state_changed(void* context, CONNECTION_STATE new_connection_state, CONNECTION_STATE previous_connection_state)
{
	SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)context;

	/* Codes_SRS_SESSION_01_060: [If the previous connection state is not OPENED and the new connection state is OPENED, the BEGIN frame shall be sent out and the state shall be switched to BEGIN_SENT.] */
	if ((new_connection_state == CONNECTION_STATE_OPENED) && (previous_connection_state != CONNECTION_STATE_OPENED) && (session_instance->session_state == SESSION_STATE_UNMAPPED))
	{
		if (send_begin(session_instance) == 0)
		{
			session_set_state(session_instance, SESSION_STATE_BEGIN_SENT);
		}
	}
	/* Codes_SRS_SESSION_01_061: [If the previous connection state is OPENED and the new connection state is not OPENED anymore, the state shall be switched to DISCARDING.] */
	else if ((new_connection_state != CONNECTION_STATE_OPENED) && (previous_connection_state == CONNECTION_STATE_OPENED))
	{
		session_set_state(session_instance, SESSION_STATE_DISCARDING);
	}
}

static void on_frame_received(void* context, AMQP_VALUE performative, uint32_t payload_size, const unsigned char* payload_bytes)
{
	SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);

	if (is_begin_type_by_descriptor(descriptor))
	{
		BEGIN_HANDLE begin_handle;

		if (amqpvalue_get_begin(performative, &begin_handle) != 0)
		{
			/* error */
		}
		else
		{
			if (begin_get_incoming_window(begin_handle, &session_instance->remote_incoming_window) != 0)
			{
				/* error */
				begin_destroy(begin_handle);
			}
			else
			{
				begin_destroy(begin_handle);
				session_set_state(session_instance, SESSION_STATE_MAPPED);
			}
		}
	}
	else if (is_attach_type_by_descriptor(descriptor))
	{
		const char* name = NULL;
		ATTACH_HANDLE attach_handle;

		if (amqpvalue_get_attach(performative, &attach_handle) != 0)
		{
			/* error */
		}
		else
		{
			if (attach_get_name(attach_handle, &name) != 0)
			{
				/* error */
				attach_destroy(attach_handle);
			}
			else
			{
				LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_name(session_instance, name);
				if (link_endpoint == NULL)
				{
					/* error */
				}
				else
				{
					link_endpoint->incoming_handle = 0;
					link_endpoint->frame_received_callback(link_endpoint->callback_context, performative, payload_size, payload_bytes);
				}

				attach_destroy(attach_handle);
			}
		}
	}
	else if (is_detach_type_by_descriptor(descriptor))
	{
		DETACH_HANDLE detach_handle;

		if (amqpvalue_get_detach(performative, &detach_handle) != 0)
		{
			/* error */
		}
		else
		{
			uint32_t remote_handle;
			if (detach_get_handle(detach_handle, &remote_handle) != 0)
			{
				/* error */
				detach_destroy(detach_handle);
			}
			else
			{
				detach_destroy(detach_handle);

				LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_incoming_handle(session_instance, remote_handle);
				if (link_endpoint == NULL)
				{
					/* error */
				}
				else
				{
					link_endpoint->frame_received_callback(link_endpoint->callback_context, performative, payload_size, payload_bytes);
				}
			}
		}
	}
	else if (is_flow_type_by_descriptor(descriptor))
	{
		ATTACH_HANDLE flow_handle;

		if (amqpvalue_get_flow(performative, &flow_handle) != 0)
		{
			/* error */
		}
		else
		{
			uint32_t remote_handle;
			transfer_number flow_next_incoming_id;
			uint32_t flow_incoming_window;
			if ((flow_get_handle(flow_handle, &remote_handle) != 0) ||
				(flow_get_next_outgoing_id(flow_handle, &session_instance->next_incoming_id) != 0) ||
				(flow_get_next_incoming_id(flow_handle, &flow_next_incoming_id) != 0) ||
				(flow_get_incoming_window(flow_handle, &flow_incoming_window) != 0))
			{
				flow_destroy(flow_handle);
				/* error */
			}
			else
			{
				session_instance->remote_incoming_window = flow_next_incoming_id + flow_incoming_window - session_instance->next_outgoing_id;

				flow_destroy(flow_handle);

				LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_incoming_handle(session_instance, remote_handle);
				if (link_endpoint == NULL)
				{
					/* error */
				}
				else
				{
					link_endpoint->frame_received_callback(link_endpoint->callback_context, performative, payload_size, payload_bytes);
				}
			}
		}
	}
	else if (is_transfer_type_by_descriptor(descriptor))
	{
		TRANSFER_HANDLE transfer_handle;

		if (amqpvalue_get_transfer(performative, &transfer_handle) != 0)
		{
			/* error */
		}
		else
		{
			uint32_t remote_handle;
			if (transfer_get_handle(transfer_handle, &remote_handle) != 0)
			{
				/* error */
				transfer_destroy(transfer_handle);
			}
			else
			{
				transfer_destroy(transfer_handle);

				LINK_ENDPOINT_INSTANCE* link_endpoint = find_link_endpoint_by_incoming_handle(session_instance, remote_handle);
				if (link_endpoint == NULL)
				{
					/* error */
				}
				else
				{
					link_endpoint->frame_received_callback(link_endpoint->callback_context, performative, payload_size, payload_bytes);
				}
			}
		}
	}
	else if (is_disposition_type_by_descriptor(descriptor))
	{
		uint32_t i;
		for (i = 0; i < session_instance->link_endpoint_count; i++)
		{
			LINK_ENDPOINT_INSTANCE* link_endpoint = session_instance->link_endpoints[i];
			link_endpoint->frame_received_callback(link_endpoint->callback_context, performative, payload_size, payload_bytes);
		}
	}
}

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_INSTANCE* result;

	if (connection == NULL)
	{
		/* Codes_SRS_SESSION_01_031: [If connection is NULL, session_create shall fail and return NULL.] */
		result = NULL;
	}
	else
	{
		/* Codes_SRS_SESSION_01_030: [session_create shall create a new session instance and return a non-NULL handle to it.] */
		result = amqpalloc_malloc(sizeof(SESSION_INSTANCE));
		/* Codes_SRS_SESSION_01_042: [If allocating memory for the session fails, session_create shall fail and return NULL.] */
		if (result != NULL)
		{
			result->connection = connection;
			result->link_endpoints = NULL;
			result->link_endpoint_count = 0;
			result->handle_max = 4294967295;

			/* Codes_SRS_SESSION_01_057: [The delivery ids shall be assigned starting at 0.] */
			/* Codes_SRS_SESSION_01_017: [The nextoutgoing-id MAY be initialized to an arbitrary value ] */
			result->next_outgoing_id = 0;

			result->incoming_window = 1;
			result->outgoing_window = 1;
			result->handle_max = 4294967295;
			result->remote_incoming_window = 0;
			result->remote_outgoing_window = 0;

			/* Codes_SRS_SESSION_01_032: [session_create shall create a new session endpoint by calling connection_create_endpoint.] */
			result->endpoint = connection_create_endpoint(connection, on_frame_received, on_connection_state_changed, result);
			if (result->endpoint == NULL)
			{
				/* Codes_SRS_SESSION_01_033: [If connection_create_endpoint fails, session_create shall fail and return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				session_set_state(result, SESSION_STATE_UNMAPPED);
			}
		}
	}

	return result;
}

int session_set_incoming_window(SESSION_HANDLE session, uint32_t incoming_window)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		session_instance->incoming_window = incoming_window;

		result = 0;
	}

	return result;
}

int session_get_incoming_window(SESSION_HANDLE session, uint32_t* incoming_window)
{
	return 0;
}

int session_set_outgoing_window(SESSION_HANDLE session, uint32_t outgoing_window)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		session_instance->outgoing_window = outgoing_window;

		result = 0;
	}

	return result;
}

int session_get_outgoing_window(SESSION_HANDLE session, uint32_t* outgoing_window)
{
	return 0;
}

int session_set_handle_max(SESSION_HANDLE session, handle handle_max)
{
	int result;

	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		session_instance->handle_max = handle_max;

		result = 0;
	}

	return result;
}

int session_get_handle_max(SESSION_HANDLE session, handle* handle_max)
{
	return 0;
}

void session_destroy(SESSION_HANDLE session)
{
	/* Codes_SRS_SESSION_01_036: [If session is NULL, session_destroy shall do nothing.] */
	if (session != NULL)
	{
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		/* Codes_SRS_SESSION_01_034: [session_destroy shall free all resources allocated by session_create.] */
		/* Codes_SRS_SESSION_01_035: [The endpoint created in session_create shall be freed by calling connection_destroy_endpoint.] */
		connection_destroy_endpoint(session_instance->endpoint);
		if (session_instance->link_endpoints != NULL)
		{
			amqpalloc_free(session_instance->link_endpoints);
		}

		amqpalloc_free(session);
	}
}

LINK_ENDPOINT_HANDLE session_create_link_endpoint(SESSION_HANDLE session, const char* name, ON_ENDPOINT_FRAME_RECEIVED frame_received_callback, ON_SESSION_STATE_CHANGED on_session_state_changed, void* context)
{
	LINK_ENDPOINT_INSTANCE* result;

	/* Codes_SRS_SESSION_01_044: [If session, name or frame_received_callback is NULL, session_create_link_endpoint shall fail and return NULL.] */
	if ((session == NULL) ||
		(name == NULL) ||
		(frame_received_callback == NULL))
	{
		result = NULL;
	}
	else
	{
		/* Codes_SRS_SESSION_01_043: [session_create_link_endpoint shall create a link endpoint associated with a given session and return a non-NULL handle to it.] */
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)session;

		result = (LINK_ENDPOINT_INSTANCE*)amqpalloc_malloc(sizeof(LINK_ENDPOINT_INSTANCE));
		/* Codes_SRS_SESSION_01_045: [If allocating memory for the link endpoint fails, session_create_link_endpoint shall fail and return NULL.] */
		if (result != NULL)
		{
			/* Codes_SRS_SESSION_01_046: [An unused handle shall be assigned to the link endpoint.] */
			handle selected_handle = 0;
			while (selected_handle < session_instance->handle_max)
			{
				break;
				selected_handle++;
			}

			result->frame_received_callback = frame_received_callback;
			result->on_session_state_changed = on_session_state_changed;
			result->callback_context = context;
			result->outgoing_handle = selected_handle;
			result->name = amqpalloc_malloc(strlen(name) + 1);
			if (result->name == NULL)
			{
				/* Codes_SRS_SESSION_01_045: [If allocating memory for the link endpoint fails, session_create_link_endpoint shall fail and return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				LINK_ENDPOINT_INSTANCE** new_link_endpoints;
				strcpy(result->name, name);
				result->session = session;

				new_link_endpoints = amqpalloc_realloc(session_instance->link_endpoints, sizeof(LINK_ENDPOINT_INSTANCE*) * (session_instance->link_endpoint_count + 1));
				if (new_link_endpoints == NULL)
				{
					/* Codes_SRS_SESSION_01_045: [If allocating memory for the link endpoint fails, session_create_link_endpoint shall fail and return NULL.] */
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					session_instance->link_endpoints = new_link_endpoints;
					session_instance->link_endpoints[session_instance->link_endpoint_count] = result;
					session_instance->link_endpoint_count++;
				}
			}
		}
	}

	return result;
}

void session_destroy_link_endpoint(LINK_ENDPOINT_HANDLE endpoint)
{
	/* Codes_SRS_SESSION_01_050: [If endpoint is NULL, session_destroy_link_endpoint shall do nothing.] */
	if (endpoint != NULL)
	{
		LINK_ENDPOINT_INSTANCE* endpoint_instance = (LINK_ENDPOINT_INSTANCE*)endpoint;
		SESSION_INSTANCE* session_instance = endpoint_instance->session;
		uint64_t i;

		/* Codes_SRS_SESSION_01_049: [session_destroy_link_endpoint shall free all resources associated with the endpoint.] */
		for (i = 0; i < session_instance->link_endpoint_count; i++)
		{
			if (session_instance->link_endpoints[i] == endpoint)
			{
				break;
			}
		}

		if (i < session_instance->link_endpoint_count)
		{
			LINK_ENDPOINT_INSTANCE** new_endpoints;

			(void)memmove(&session_instance->link_endpoints[i], &session_instance->link_endpoints[i + 1], (session_instance->link_endpoint_count - (uint32_t)i - 1) * sizeof(LINK_ENDPOINT_INSTANCE*));
			session_instance->link_endpoint_count--;

			if (session_instance->link_endpoint_count == 0)
			{
				amqpalloc_free(session_instance->link_endpoints);
				session_instance->link_endpoints = NULL;
			}
			else
			{
				new_endpoints = (LINK_ENDPOINT_INSTANCE**)amqpalloc_realloc(session_instance->link_endpoints, sizeof(LINK_ENDPOINT_INSTANCE*) * session_instance->link_endpoint_count);
				if (new_endpoints != NULL)
				{
					session_instance->link_endpoints = new_endpoints;
				}
			}
		}

		if (endpoint_instance->name != NULL)
		{
			amqpalloc_free(endpoint_instance->name);
		}

		amqpalloc_free(endpoint_instance);
	}
}

/* Codes_SRS_SESSION_01_037: [session_encode_frame shall encode an AMQP frame.] */
int session_encode_frame(LINK_ENDPOINT_HANDLE link_endpoint, const AMQP_VALUE performative, PAYLOAD* payloads, size_t payload_count)
{
	int result;

	/* Codes_SRS_SESSION_01_040: [If link_endpoint or performative is NULL, session_encode_frame shall fail and return a non-zero value.] */
	if ((link_endpoint == NULL) ||
		(performative == NULL))
	{
		result = __LINE__;
	}
	else
	{
		LINK_ENDPOINT_INSTANCE* link_endpoint_instance = (LINK_ENDPOINT_INSTANCE*)link_endpoint;
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)link_endpoint_instance->session;

		/* Codes_SRS_SESSION_01_039: [The encoding shall be done by passing the performative and the payloads to the connection_encode_frame function.] */
		if (connection_encode_frame(session_instance->endpoint, performative, payloads, payload_count) != 0)
		{
			/* Codes_SRS_SESSION_01_041: [If connection_encode_frame fails, session_encode_frame shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_SESSION_01_038: [On success it shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_SESSION_01_051: [session_transfer shall send a transfer frame with the performative indicated in the transfer argument.] */
int session_transfer(LINK_ENDPOINT_HANDLE link_endpoint, TRANSFER_HANDLE transfer, PAYLOAD* payloads, size_t payload_count, delivery_number* delivery_id)
{
	int result;

	/* Codes_SRS_SESSION_01_054: [If link_endpoint or transfer is NULL, session_transfer shall fail and return a non-zero value.] */
	if ((link_endpoint == NULL) ||
		(transfer == NULL))
	{
		result = __LINE__;
	}
	else
	{
		LINK_ENDPOINT_INSTANCE* link_endpoint_instance = (LINK_ENDPOINT_INSTANCE*)link_endpoint;
		SESSION_INSTANCE* session_instance = (SESSION_INSTANCE*)link_endpoint_instance->session;

		/* Codes_SRS_SESSION_01_059: [When session_transfer is called while the session is not in the MAPPED state, session_transfer shall fail and return a non-zero value.] */
		if (session_instance->session_state != SESSION_STATE_MAPPED)
		{
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE transfer_value;

			/* Codes_SRS_SESSION_01_012: [The session endpoint assigns each outgoing transfer frame an implicit transfer-id from a session scoped sequence.] */
			/* Codes_SRS_SESSION_01_027: [sending a transfer Upon sending a transfer, the sending endpoint will increment its next-outgoing-id] */
			*delivery_id = session_instance->next_outgoing_id;
			if (transfer_set_delivery_id(transfer, *delivery_id) != 0)
			{
				/* Codes_SRS_SESSION_01_058: [When any other error occurs, session_transfer shall fail and return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				transfer_value = amqpvalue_create_transfer(transfer);
				if (transfer_value == NULL)
				{
					/* Codes_SRS_SESSION_01_058: [When any other error occurs, session_transfer shall fail and return a non-zero value.] */
					result = __LINE__;
				}
				else
				{
					uint32_t available_frame_size;
					size_t encoded_size;

					if ((connection_get_remote_max_frame_size(session_instance->connection, &available_frame_size) != 0) ||
						(amqpvalue_get_encoded_size(transfer_value, &encoded_size) != 0))
					{
						result = __LINE__;
					}
					else
					{
						uint32_t payload_size = 0;
						size_t i;

						for (i = 0; i < payload_count; i++)
						{
							payload_size += payloads[i].length;
						}

						available_frame_size -= encoded_size;
						available_frame_size -= 8;

						if (available_frame_size >= payload_size)
						{
							/* Codes_SRS_SESSION_01_055: [The encoding of the frame shall be done by calling connection_encode_frame and passing as arguments: the connection handle associated with the session, the transfer performative and the payload chunks passed to session_transfer.] */
							if (connection_encode_frame(session_instance->endpoint, transfer_value, payloads, payload_count) != 0)
							{
								/* Codes_SRS_SESSION_01_056: [If connection_encode_frame fails then session_transfer shall fail and return a non-zero value.] */
								result = __LINE__;
							}
							else
							{
								/* Codes_SRS_SESSION_01_018: [is incremented after each successive transfer according to RFC-1982 [RFC1982] serial number arithmetic.] */
								session_instance->next_outgoing_id++;
								session_instance->outgoing_window--;

								/* Codes_SRS_SESSION_01_053: [On success, session_transfer shall return 0.] */
								result = 0;
							}
						}
						else
						{
							size_t current_payload_index = 0;
							uint32_t current_payload_pos = 0;

							/* break it down into different deliveries */
							while (payload_size > 0)
							{
								uint32_t transfer_frame_payload_count = 0;
								uint32_t current_transfer_frame_payload_size = payload_size;
								uint32_t byte_counter;
								size_t temp_current_payload_index = current_payload_index;
								uint32_t temp_current_payload_pos = current_payload_pos;
								AMQP_VALUE multi_transfer_amqp_value;

								if (current_transfer_frame_payload_size > available_frame_size)
								{
									current_transfer_frame_payload_size = available_frame_size;
								}

								if (available_frame_size >= payload_size)
								{
									transfer_set_more(transfer, false);
								}
								else
								{
									transfer_set_more(transfer, true);
								}

								multi_transfer_amqp_value = amqpvalue_create_transfer(transfer);
								if (multi_transfer_amqp_value == NULL)
								{
									break;
								}

								byte_counter = current_transfer_frame_payload_size;
								while (byte_counter > 0)
								{
									if (payloads[temp_current_payload_index].length - temp_current_payload_pos >= byte_counter)
									{
										/* more data than we need */
										temp_current_payload_pos += byte_counter;
										byte_counter = 0;
									}
									else
									{
										byte_counter -= payloads[temp_current_payload_index].length - temp_current_payload_pos;
										temp_current_payload_index++;
										temp_current_payload_pos = 0;
									}
								}

								transfer_frame_payload_count = temp_current_payload_index - current_payload_index + 1;
								PAYLOAD* transfer_frame_payloads = (PAYLOAD*)malloc(transfer_frame_payload_count * sizeof(PAYLOAD));
								if (transfer_frame_payloads == NULL)
								{
									amqpvalue_destroy(multi_transfer_amqp_value);
									break;
								}

								/* copy data */
								byte_counter = current_transfer_frame_payload_size;
								transfer_frame_payload_count = 0;

								while (byte_counter > 0)
								{
									if (payloads[current_payload_index].length - current_payload_pos > byte_counter)
									{
										/* more data than we need */
										transfer_frame_payloads[transfer_frame_payload_count].bytes = payloads[current_payload_index].bytes + current_payload_pos;
										transfer_frame_payloads[transfer_frame_payload_count].length = byte_counter;
										current_payload_pos += byte_counter;
										byte_counter = 0;
									}
									else
									{
										/* copy entire payload and move to the next */
										transfer_frame_payloads[transfer_frame_payload_count].bytes = payloads[current_payload_index].bytes + current_payload_pos;
										transfer_frame_payloads[transfer_frame_payload_count].length = payloads[current_payload_index].length - current_payload_pos;
										byte_counter -= payloads[current_payload_index].length - current_payload_pos;
										current_payload_index++;
										current_payload_pos = 0;
									}

									transfer_frame_payload_count++;
								}

								if (connection_encode_frame(session_instance->endpoint, multi_transfer_amqp_value, transfer_frame_payloads, transfer_frame_payload_count) != 0)
								{
									amqpalloc_free(transfer_frame_payloads);
									amqpvalue_destroy(multi_transfer_amqp_value);
									break;
								}

								amqpalloc_free(transfer_frame_payloads);
								amqpvalue_destroy(multi_transfer_amqp_value);
								payload_size -= current_transfer_frame_payload_size;
							}

							if (payload_size > 0)
							{
								result = __LINE__;
							}
							else
							{
								result = 0;
							}
						}
					}

					amqpvalue_destroy(transfer_value);
				}
			}
		}
	}

	return result;
}
