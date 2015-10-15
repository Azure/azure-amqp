#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "link.h"
#include "session.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"
#include "amqpalloc.h"
#include "amqp_frame_codec.h"
#include "consolelogger.h"
#include "logger.h"
#include "amqpvalue_to_string.h"

typedef struct DELIVERY_INSTANCE_TAG
{
	delivery_number delivery_id;
	DELIVERY_SETTLED_CALLBACK delivery_settled_callback;
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
} LINK_INSTANCE;

static void link_frame_received(void* context, AMQP_VALUE performative, uint32_t frame_payload_size, const unsigned char* payload_bytes)
{
	LINK_INSTANCE* link = (LINK_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);
	uint64_t performative_ulong;
	amqpvalue_get_ulong(descriptor, &performative_ulong);
	switch (performative_ulong)
	{
	case AMQP_ATTACH:
		LOG(consolelogger_log, LOG_LINE, "<- [ATTACH]");
		LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(performative));
		if (link->link_state == LINK_STATE_HALF_ATTACHED)
		{
			link->link_state = LINK_STATE_ATTACHED;
		}
		break;

	case AMQP_FLOW:
		break;

	case AMQP_TRANSFER:
		break;

	case AMQP_DISPOSITION:
	{
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE first_value = amqpvalue_get_list_item(described_value, 1);
		AMQP_VALUE last_value = amqpvalue_get_list_item(described_value, 2);
		delivery_number first;
		delivery_number last;

		amqpvalue_get_uint(first_value, &first);
		if (amqpvalue_get_uint(last_value, &last) != 0)
		{
			last = first;
		}

		uint32_t i;
		for (i = 0; i < link->pending_delivery_count; i++)
		{
			if ((link->pending_deliveries[i].delivery_id >= first) &&
				(link->pending_deliveries[i].delivery_id <= last))
			{
				link->pending_deliveries[i].delivery_settled_callback(link->pending_deliveries[i].callback_context, link->pending_deliveries[i].delivery_id);
				if (link->pending_delivery_count - i > 1)
				{
					memmove(&link->pending_deliveries[i], &link->pending_deliveries[i + 1], sizeof(DELIVERY_INSTANCE) * (link->pending_delivery_count - i - 1));
				}

				link->pending_delivery_count--;
				i--;
			}
		}

		LOG(consolelogger_log, 0, "<- [DISPOSITION]");
		LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(performative));
		break;
	}

	case AMQP_DETACH:
	{
		const char* error = NULL;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE error_value = amqpvalue_get_list_item(described_value, 2);
		AMQP_VALUE error_described_value = amqpvalue_get_described_value(error_value);
		AMQP_VALUE error_description_value = amqpvalue_get_list_item(error_described_value, 1);
		amqpvalue_get_string(error_description_value, &error);

		LOG(consolelogger_log, 0, "<- [DETACH]");
		LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(performative));
		break;
	}
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
		attach_set_role(attach, false);
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
				LOG(consolelogger_log, 0, "-> [ATTACH]");
				LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(attach_performative_value));

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
			if (send_attach(link_instance, link_instance->name, 0, role_sender, sender_settle_mode_settled, receiver_settle_mode_first) == 0)
			{
				link_instance->link_state = LINK_STATE_HALF_ATTACHED;
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

LINK_HANDLE link_create(SESSION_HANDLE session, const char* name, AMQP_VALUE source, AMQP_VALUE target, AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback)
{
	LINK_INSTANCE* result = amqpalloc_malloc(sizeof(LINK_INSTANCE));
	if (result != NULL)
	{
		result->link_state = LINK_STATE_DETACHED;
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
		}
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

		amqpalloc_free(handle);
	}
}

int link_get_state(LINK_HANDLE handle, LINK_STATE* link_state)
{
	int result;
	LINK_INSTANCE* link = (LINK_INSTANCE*)handle;

	if ((link == NULL) ||
		(link_state == NULL))
	{
		result = __LINE__;
	}
	else
	{
		*link_state = link->link_state;
		result = 0;
	}

	return result;
}

int link_transfer(LINK_HANDLE handle, PAYLOAD* payloads, size_t payload_count, DELIVERY_SETTLED_CALLBACK delivery_settled_callback, void* callback_context)
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
				link->pending_deliveries[link->pending_delivery_count].delivery_settled_callback = delivery_settled_callback;
				link->pending_deliveries[link->pending_delivery_count].callback_context = callback_context;
				link->pending_delivery_count++;

				LOG(consolelogger_log, 0, "-> [TRANSFER]");
				LOG(consolelogger_log, LOG_LINE, amqpvalue_to_string(transfer_value));

				result = 0;
			}

			amqpvalue_destroy(amqp_value);
			amqpvalue_destroy(amqp_value_descriptor);

			transfer_destroy(transfer);
		}
	}

	return result;
}
