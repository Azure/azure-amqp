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

typedef struct LINK_INSTANCE_TAG
{
	SESSION_HANDLE session;
	LINK_STATE link_state;
	AMQP_VALUE source;
	AMQP_VALUE target;
	handle handle;
	LINK_ENDPOINT_HANDLE link_endpoint;
	char* name;
	DELIVERY_SETTLED_CALLBACK delivery_settled_callback;
	void* callback_context;
} LINK_INSTANCE;

static void link_frame_received(void* context, AMQP_VALUE performative, uint32_t frame_payload_size)
{
	LINK_INSTANCE* link = (LINK_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_descriptor(performative);
	uint64_t performative_ulong;
	amqpvalue_get_ulong(descriptor, &performative_ulong);
	switch (performative_ulong)
	{
	case AMQP_ATTACH:
		LOG(consolelogger_log, LOG_LINE, "<- [ATTACH]");
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
		LOG(consolelogger_log, LOG_LINE, "<- [DISPOSITION]");
		break;

	case AMQP_DETACH:
	{
		const char* error = NULL;
		AMQP_VALUE described_value = amqpvalue_get_described_value(performative);
		AMQP_VALUE error_value = amqpvalue_get_list_item(described_value, 2);
		AMQP_VALUE error_described_value = amqpvalue_get_described_value(error_value);
		AMQP_VALUE error_description_value = amqpvalue_get_list_item(error_described_value, 1);
		amqpvalue_get_string(error_description_value, &error);

		LOG(consolelogger_log, LOG_LINE, "<- [DETACH:%s]", error);
		break;
	}
	}
}

static void link_frame_payload_bytes_received(void* context, const unsigned char* payload_bytes, uint32_t byte_count)
{
	LINK_INSTANCE* link = (LINK_INSTANCE*)context;
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
			if (session_begin_encode_frame(link->session, attach_performative_value, 0) != 0)
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

static int encode_bytes(void* context, const void* bytes, size_t length)
{
	SESSION_HANDLE session = context;
	(void)session_encode_payload_bytes(session, bytes, length);
	return 0;
}

static int send_tranfer(LINK_INSTANCE* link, AMQP_VALUE payload)
{
	int result;
	TRANSFER_HANDLE transfer =  transfer_create(0);
	unsigned char delivery_tag_bytes[] = "muie";
	delivery_tag delivery_tag = { &delivery_tag, sizeof(delivery_tag) };
	transfer_set_delivery_tag(transfer, delivery_tag);
	transfer_set_message_format(transfer, 0);
	transfer_set_settled(transfer, false);
	transfer_set_more(transfer, false);
	AMQP_VALUE transfer_value = amqpvalue_create_transfer(transfer);

	size_t encoded_size;
	AMQP_VALUE amqp_value_descriptor = amqpvalue_create_ulong(0x77);
	AMQP_VALUE amqp_value = amqpvalue_create_described(amqpvalue_clone(amqp_value_descriptor), amqpvalue_clone(payload));
	amqpvalue_get_encoded_size(amqp_value, &encoded_size);

	/* here we should feed data to the transfer frame */
	if (session_begin_transfer(link->session, transfer, encoded_size) != 0)
	{
		result = __LINE__;
	}
	else
	{
		amqpvalue_encode(amqp_value, encode_bytes, link->session);
		LOG(consolelogger_log, LOG_LINE, "-> [TRANSFER]");
		result = 0;
	}

	amqpvalue_destroy(amqp_value);
	amqpvalue_destroy(amqp_value_descriptor);

	transfer_destroy(transfer);

	return result;
}

LINK_HANDLE link_create(SESSION_HANDLE session, const char* name, AMQP_VALUE source, AMQP_VALUE target, DELIVERY_SETTLED_CALLBACK delivery_settled_callback, void* callback_context)
{
	LINK_INSTANCE* result = amqpalloc_malloc(sizeof(LINK_INSTANCE));
	if (result != NULL)
	{
		result->link_state = LINK_STATE_DETACHED;
		result->source = amqpvalue_clone(source);
		result->target = amqpvalue_clone(target);
		result->session = session;
		result->handle = 0;
		result->delivery_settled_callback = delivery_settled_callback;
		result->callback_context = callback_context;

		result->name = amqpalloc_malloc(_mbstrlen(name) + 1);
		if (result->name == NULL)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			(void)strcpy(result->name, name);
			result->link_endpoint = session_create_link_endpoint(session, name, link_frame_received, link_frame_payload_bytes_received, result);
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
		amqpalloc_free(handle);
	}
}

void link_dowork(LINK_HANDLE handle)
{
	LINK_INSTANCE* link = (LINK_INSTANCE*)handle;
	SESSION_STATE session_state;

	if (session_get_state(link->session, &session_state) == 0)
	{
		if (session_state == SESSION_STATE_MAPPED)
		{
			if (link->link_state == LINK_STATE_DETACHED)
			{
				if (send_attach(link, link->name, 0, role_sender, sender_settle_mode_settled, receiver_settle_mode_first) == 0)
				{
					LOG(consolelogger_log, LOG_LINE, "-> [ATTACH]");
					link->link_state = LINK_STATE_HALF_ATTACHED;
				}
			}
		}
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

int link_transfer(LINK_HANDLE handle, AMQP_VALUE payload_chunk)
{
	int result;
	LINK_INSTANCE* link = (LINK_INSTANCE*)handle;
	if (link == NULL)
	{
		result = __LINE__;
	}
	else
	{
		result = send_tranfer(link, payload_chunk);
	}

	return result;
}
