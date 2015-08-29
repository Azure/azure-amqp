#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "link.h"
#include "session.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"
#include "amqpalloc.h"
#include "amqp_frame_codec.h"

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
		if (link->link_state == LINK_STATE_HALF_ATTACHED)
		{
			link->link_state = LINK_STATE_ATTACHED;
		}
		break;
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

static int send_tranfer(LINK_DATA* link, const AMQP_VALUE* payload_chunks, size_t payload_chunk_count)
{
	int result;
	AMQP_VALUE transfer_frame_list = amqpvalue_create_list();
	if (transfer_frame_list == NULL)
	{
		result = __LINE__;
	}
	else
	{
		delivery_tag delivery_tag = { &link->delivery_id, sizeof(link->delivery_id) };
		AMQP_VALUE handle_value = amqpvalue_create_handle(link->handle);
		AMQP_VALUE delivery_id_value = amqpvalue_create_delivery_number(link->delivery_id++);
		AMQP_VALUE delivery_tag_value = amqpvalue_create_delivery_tag(delivery_tag);

		if ((handle_value == NULL) ||
			(delivery_id_value == NULL) ||
			(delivery_tag_value == NULL) ||
			(amqpvalue_set_list_item(transfer_frame_list, 0, handle_value) != 0) ||
			(amqpvalue_set_list_item(transfer_frame_list, 1, delivery_id_value) != 0) ||
			(amqpvalue_set_list_item(transfer_frame_list, 2, delivery_tag_value) != 0))
		{
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE* chunks = amqpalloc_malloc(sizeof(AMQP_VALUE) * (payload_chunk_count + 1));
			if (chunks == NULL)
			{
				result = __LINE__;
			}
			else
			{
				AMQP_VALUE ulong_descriptor_value = amqpvalue_create_ulong(0x14);
				AMQP_VALUE performative = amqpvalue_create_described(ulong_descriptor_value, transfer_frame_list);
				size_t i;

				chunks[0] = transfer_frame_list;
				for (i = 0; i < payload_chunk_count; i++)
				{
					chunks[i + 1] = payload_chunks[i];
				}

				/* here we should feed data to the transfer frame */
				if (session_begin_encode_frame(link->session, performative, 0) != 0)
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}

				amqpalloc_free(chunks);
			}
		}

		amqpvalue_destroy(transfer_frame_list);
	}

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
				(void)send_attach(link, "fake", 1, role_sender, sender_settle_mode_unsettled, receiver_settle_mode_first, link->source, link->target);
				link->link_state = LINK_STATE_HALF_ATTACHED;
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
