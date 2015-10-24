#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "link.h"
#include "session.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"
#include "amqpalloc.h"
#include "amqp_frame_codec.h"
#include "consolelogger.h"
#include "logger.h"

typedef struct DELIVERY_INSTANCE_TAG
{
	delivery_number delivery_id;
	ON_DELIVERY_SETTLED on_delivery_settled;
	void* callback_context;
} DELIVERY_INSTANCE;

typedef struct LINK_INSTANCE_TAG
{
	SESSION_HANDLE session;
	LINK_STATE link_state;
	AMQP_VALUE source;
	AMQP_VALUE target;
	handle handle;
	LINK_ENDPOINT_HANDLE link_endpoint;
	char* name;
	uint32_t pending_delivery_count;
	DELIVERY_INSTANCE* pending_deliveries;
	uint32_t delivery_tag_no;
	role role;
	ON_LINK_STATE_CHANGED on_link_state_changed;
    ON_TRANSFER_RECEIVED on_transfer_received;
	void* callback_context;
} LINK_INSTANCE;

static void set_link_state(LINK_INSTANCE* link_instance, LINK_STATE link_state)
{
	LINK_STATE previous_state = link_instance->link_state;
	link_instance->link_state = link_state;

	if (link_instance->on_link_state_changed != NULL)
	{
		link_instance->on_link_state_changed(link_instance->callback_context, link_state, previous_state);
	}
}

static int send_flow(LINK_INSTANCE* link)
{
	int result;
	FLOW_HANDLE flow = flow_create(100, 0, 100);

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		flow_set_link_credit(flow, 100000);
		flow_set_handle(flow, link->handle);
		AMQP_VALUE flow_performative_value = amqpvalue_create_flow(flow);
		if (flow_performative_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (session_encode_frame(link->link_endpoint, flow_performative_value, NULL, 0) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(flow_performative_value);
		}

		flow_destroy(flow);
	}

	return result;
}

static void remove_pending_delivery(LINK_INSTANCE* link_instance, uint32_t index)
{
	if (link_instance->pending_delivery_count - index > 1)
	{
		memmove(&link_instance->pending_deliveries[index], &link_instance->pending_deliveries[index + 1], sizeof(DELIVERY_INSTANCE) * (link_instance->pending_delivery_count - index - 1));
	}

	if (link_instance->pending_delivery_count > 1)
	{
		DELIVERY_INSTANCE* new_pending_deliveries = (DELIVERY_INSTANCE*)amqpalloc_realloc(link_instance->pending_deliveries, sizeof(DELIVERY_INSTANCE) * (link_instance->pending_delivery_count - 1));
		if (new_pending_deliveries != NULL)
		{
			link_instance->pending_deliveries = new_pending_deliveries;
		}
	}
	else
	{
		amqpalloc_free(link_instance->pending_deliveries);
		link_instance->pending_deliveries = NULL;
	}

	link_instance->pending_delivery_count--;
}

static void link_frame_received(void* context, AMQP_VALUE performative, uint32_t payload_size, const unsigned char* payload_bytes)
{
	LINK_INSTANCE* link_instance = (LINK_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
	if (is_attach_type_by_descriptor(descriptor))
	{
		if (link_instance->link_state == LINK_STATE_HALF_ATTACHED)
		{
			if (link_instance->role == role_receiver)
			{
				send_flow(link_instance);
			}

			set_link_state(link_instance, LINK_STATE_ATTACHED);
		}
	}
	else if (is_flow_type_by_descriptor(descriptor))
	{
	}
	else if (is_transfer_type_by_descriptor(descriptor))
	{
		if (link_instance->on_transfer_received != NULL)
		{
			TRANSFER_HANDLE transfer_handle;
			if (amqpvalue_get_transfer(performative, &transfer_handle) == 0)
			{
				link_instance->on_transfer_received(link_instance->callback_context, transfer_handle, payload_size, payload_bytes);
				transfer_destroy(transfer_handle);
			}
		}
	}
	else if (is_disposition_type_by_descriptor(descriptor))
	{
		DISPOSITION_HANDLE disposition;
		if (amqpvalue_get_disposition(performative, &disposition) != 0)
		{
			/* error */
		}
		else
		{
			delivery_number first;
			delivery_number last;

			if (disposition_get_first(disposition, &first) != 0)
			{
				/* error */
			}
			else
			{
				if (disposition_get_last(disposition, &last) != 0)
				{
					last = first;
				}

				uint32_t i;
				for (i = 0; i < link_instance->pending_delivery_count; i++)
				{
					if ((link_instance->pending_deliveries[i].delivery_id >= first) &&
						(link_instance->pending_deliveries[i].delivery_id <= last))
					{
						link_instance->pending_deliveries[i].on_delivery_settled(link_instance->pending_deliveries[i].callback_context, link_instance->pending_deliveries[i].delivery_id);
						remove_pending_delivery(link_instance, i);
						i--;
					}
				}
			}

			disposition_destroy(disposition);
		}
	}
	else if (is_detach_type_by_descriptor(descriptor))
	{
	}
}

static int send_attach(LINK_INSTANCE* link, const char* name, handle handle, role role, sender_settle_mode snd_settle_mode, receiver_settle_mode rcv_settle_mode)
{
	int result;
	ATTACH_HANDLE attach = attach_create(name, handle, role);

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		attach_set_snd_settle_mode(attach, snd_settle_mode);
		attach_set_rcv_settle_mode(attach, rcv_settle_mode);
		attach_set_role(attach, role);
		attach_set_source(attach, link->source);
		attach_set_target(attach, link->target);

		AMQP_VALUE attach_performative_value = amqpvalue_create_attach(attach);
		if (attach_performative_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (session_encode_frame(link->link_endpoint, attach_performative_value, NULL, 0) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(attach_performative_value);
		}
	}

	return result;
}

static void on_session_state_changed(void* context, SESSION_STATE new_session_state, SESSION_STATE previous_session_state)
{
	LINK_INSTANCE* link_instance = (LINK_INSTANCE*)context;
	if (new_session_state == SESSION_STATE_MAPPED)
	{
		if (link_instance->link_state == LINK_STATE_DETACHED)
		{
			if (send_attach(link_instance, link_instance->name, 0, link_instance->role, sender_settle_mode_settled, receiver_settle_mode_first) == 0)
			{
				set_link_state(link_instance, LINK_STATE_HALF_ATTACHED);
			}
		}
	}
}

static int encode_bytes(void* context, const void* bytes, size_t length)
{
	PAYLOAD* payload = (PAYLOAD*)context;
	memcpy((unsigned char*)payload->bytes + payload->length, bytes, length);
	payload->length += length;
	return 0;
}

LINK_HANDLE link_create(SESSION_HANDLE session, const char* name, role role, AMQP_VALUE source, AMQP_VALUE target)
{
	LINK_INSTANCE* result = amqpalloc_malloc(sizeof(LINK_INSTANCE));
	if (result != NULL)
	{
		result->link_state = LINK_STATE_DETACHED;
		result->role = role;
		result->source = amqpvalue_clone(source);
		result->target = amqpvalue_clone(target);
		result->session = session;
		result->handle = 0;
		result->pending_deliveries = NULL;
		result->pending_delivery_count = 0;
		result->delivery_tag_no = 0;

		result->name = amqpalloc_malloc(_mbstrlen(name) + 1);
		if (result->name == NULL)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			(void)strcpy(result->name, name);
			result->link_endpoint = session_create_link_endpoint(session, name, link_frame_received, on_session_state_changed, result);
			if (result->link_endpoint == NULL)
			{
				amqpalloc_free(result->name);
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->on_link_state_changed = NULL;
				result->callback_context = NULL;

				set_link_state(result, LINK_STATE_DETACHED);
			}
		}
	}

	return result;
}

int link_subscribe_events(LINK_HANDLE link, ON_TRANSFER_RECEIVED on_transfer_received, ON_LINK_STATE_CHANGED on_link_state_changed, void* callback_context)
{
	int result;

	if (link == NULL)
	{
		result = __LINE__;
	}
	else
	{
		LINK_INSTANCE* link_instance = (LINK_INSTANCE*)link;

		link_instance->on_link_state_changed = on_link_state_changed;
        link_instance->on_transfer_received = on_transfer_received;
		link_instance->callback_context = callback_context;

		result = 0;
	}

	return result;
}

void link_destroy(LINK_HANDLE handle)
{
	if (handle != NULL)
	{
		LINK_INSTANCE* link = (LINK_INSTANCE*)handle;
		session_destroy_link_endpoint(link->link_endpoint);
		amqpvalue_destroy(link->source);
		amqpvalue_destroy(link->target);
		if (link->pending_deliveries != NULL)
		{
			amqpalloc_free(link->pending_deliveries);
		}
		if (link->name != NULL)
		{
			amqpalloc_free(link->name);
		}

		amqpalloc_free(handle);
	}
}

int link_transfer(LINK_HANDLE handle, PAYLOAD* payloads, size_t payload_count, ON_DELIVERY_SETTLED on_delivery_settled, void* callback_context)
{
	int result;
	LINK_INSTANCE* link = (LINK_INSTANCE*)handle;

	if (link == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_HANDLE transfer = transfer_create(0);

		unsigned char delivery_tag_bytes[sizeof(int)];
		memcpy(delivery_tag_bytes, &link->delivery_tag_no, sizeof(int));
		link->delivery_tag_no++;
		delivery_tag delivery_tag = { &delivery_tag_bytes, sizeof(delivery_tag_bytes) };
		transfer_set_delivery_tag(transfer, delivery_tag);
		transfer_set_message_format(transfer, 0);
		transfer_set_settled(transfer, false);
		transfer_set_more(transfer, false);
		AMQP_VALUE transfer_value = amqpvalue_create_transfer(transfer);

		DELIVERY_INSTANCE* new_pending_deliveries = amqpalloc_realloc(link->pending_deliveries, (link->pending_delivery_count + 1) * sizeof(DELIVERY_INSTANCE));
		if (new_pending_deliveries == NULL)
		{
			result = __LINE__;
		}
		else
		{
			size_t encoded_size;
			AMQP_VALUE amqp_value_descriptor = amqpvalue_create_ulong(0x77);
			amqp_binary binary_value = { payloads[0].bytes, payloads[0].length };
			AMQP_VALUE amqp_value = amqpvalue_create_described(amqpvalue_clone(amqp_value_descriptor), amqpvalue_create_binary(binary_value));
			amqpvalue_get_encoded_size(amqp_value, &encoded_size);
			void* data_bytes = amqpalloc_malloc(encoded_size);
			PAYLOAD payload = { data_bytes, 0 };
			(void)amqpvalue_encode(amqp_value, encode_bytes, &payload);

			link->pending_deliveries = new_pending_deliveries;

			/* here we should feed data to the transfer frame */
			if (session_transfer(link->link_endpoint, transfer, &payload, 1, &link->pending_deliveries[link->pending_delivery_count].delivery_id) != 0)
			{
				result = __LINE__;
			}
			else
			{
				link->pending_deliveries[link->pending_delivery_count].on_delivery_settled = on_delivery_settled;
				link->pending_deliveries[link->pending_delivery_count].callback_context = callback_context;
				link->pending_delivery_count++;

				result = 0;
			}

			amqpvalue_destroy(amqp_value);
			amqpvalue_destroy(amqp_value_descriptor);
		}

		transfer_destroy(transfer);
	}

	return result;
}
