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

typedef struct LINK_DATA_TAG
{
	SESSION_HANDLE session;
	LINK_STATE link_state;
	AMQP_VALUE source;
	AMQP_VALUE target;
	handle handle;
	delivery_number delivery_id;
} LINK_DATA;

static void link_frame_received(void* context, AMQP_VALUE performative, uint32_t frame_payload_size)
{
	LINK_DATA* link = (LINK_DATA*)context;
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
		LOG(consolelogger_log, LOG_LINE, "<- [FLOW]");
		break;

	case AMQP_DETACH:
	{
		const char* error;
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

static int send_attach(LINK_DATA* link, const char* name, handle handle, role role, sender_settle_mode snd_settle_mode, receiver_settle_mode rcv_settle_mode, AMQP_VALUE source, AMQP_VALUE target)
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
		attach_set_source(attach, source);
		attach_set_target(attach, target);

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

static int send_tranfer(LINK_DATA* link, const AMQP_VALUE* payload_chunks, size_t payload_chunk_count)
{
	int result;
	TRANSFER_HANDLE transfer =  transfer_create(0);
	delivery_tag delivery_tag = { &link->delivery_id, sizeof(link->delivery_id) };
	transfer_set_delivery_id(transfer, link->delivery_id++);
	transfer_set_delivery_tag(transfer, delivery_tag);
	AMQP_VALUE transfer_value = amqpvalue_create_transfer(transfer);

	AMQP_VALUE ulong_descriptor_value = amqpvalue_create_ulong(0x14);
	AMQP_VALUE performative = amqpvalue_create_described(ulong_descriptor_value, transfer_value);

	size_t encoded_size;
	amqpvalue_get_encoded_size(payload_chunks[0], &encoded_size);

	/* here we should feed data to the transfer frame */
	if (session_begin_encode_frame(link->session, performative, encoded_size) != 0)
	{
		result = __LINE__;
	}
	else
	{
		amqpvalue_encode(payload_chunks[0], encode_bytes, link->session);
		LOG(consolelogger_log, LOG_LINE, "-> [TRANSFER]");
		result = 0;
	}

	transfer_destroy(transfer);

	return result;
}

LINK_HANDLE link_create(SESSION_HANDLE session, AMQP_VALUE source, AMQP_VALUE target)
{
	LINK_DATA* result = amqpalloc_malloc(sizeof(LINK_DATA));
	if (result != NULL)
	{
		if (session_set_frame_received_callback(session, link_frame_received, result) != 0)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			result->link_state = LINK_STATE_DETACHED;
			result->source = amqpvalue_clone(source);
			result->target = amqpvalue_clone(target);
			result->session = session;
			result->handle = 1;
			result->delivery_id = 0;
		}
	}

	return result;
}

void link_destroy(LINK_HANDLE handle)
{
	if (handle != NULL)
	{
		LINK_DATA* link = (LINK_DATA*)handle;
		amqpvalue_destroy(link->source);
		amqpvalue_destroy(link->target);
		amqpalloc_free(handle);
	}
}

void link_dowork(LINK_HANDLE handle)
{
	LINK_DATA* link = (LINK_DATA*)handle;
	SESSION_STATE session_state;

	if (session_get_state(link->session, &session_state) == 0)
	{
		if (session_state == SESSION_STATE_MAPPED)
		{
			if (link->link_state == LINK_STATE_DETACHED)
			{
				if (send_attach(link, "ingress", 1, role_sender, sender_settle_mode_unsettled, receiver_settle_mode_first, link->source, link->target) == 0)
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
	LINK_DATA* link = (LINK_DATA*)handle;

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

int link_transfer(LINK_HANDLE handle, const AMQP_VALUE* payload_chunks, size_t payload_chunk_count)
{
	int result;
	LINK_DATA* link = (LINK_DATA*)handle;
	if (link == NULL)
	{
		result = __LINE__;
	}
	else
	{
		result = send_tranfer(link, payload_chunks, payload_chunk_count);
	}

	return result;
}
