#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "link.h"
#include "session.h"
#include "amqpvalue.h"
#include "amqp_protocol_types.h"

typedef struct LINK_DATA_TAG
{
	SESSION_HANDLE session;
	LINK_STATE link_state;
	AMQP_VALUE source;
	AMQP_VALUE target;
	handle handle;
	delivery_number delivery_id;
} LINK_DATA;

static void link_frame_received(void* context, uint64_t performative, AMQP_VALUE frame_list_value)
{
	LINK_DATA* link = (LINK_DATA*)context;
	switch (performative)
	{
	case 0x12:
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
	AMQP_VALUE attach_frame_list = amqpvalue_create_list(7);
	if (attach_frame_list == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE name_value = amqpvalue_create_string(name);
		AMQP_VALUE handle_value = amqpvalue_create_handle(handle);
		AMQP_VALUE role_value = amqpvalue_create_role(role);
		AMQP_VALUE snd_settle_mode_value = amqpvalue_create_sender_settle_mode(snd_settle_mode);
		AMQP_VALUE rcv_settle_mode_value = amqpvalue_create_receiver_settle_mode(rcv_settle_mode);
		/* do not set remote_channel for now */

		if ((name_value == NULL) ||
			(handle_value == NULL) ||
			(role_value == NULL) ||
			(snd_settle_mode_value == NULL) ||
			(rcv_settle_mode_value == NULL) ||
			(amqpvalue_set_list_item(attach_frame_list, 0, name_value) != 0) ||
			(amqpvalue_set_list_item(attach_frame_list, 1, handle_value) != 0) ||
			(amqpvalue_set_list_item(attach_frame_list, 2, role_value) != 0) ||
			(amqpvalue_set_list_item(attach_frame_list, 3, snd_settle_mode_value) != 0) ||
			(amqpvalue_set_list_item(attach_frame_list, 4, rcv_settle_mode_value) != 0) ||
			(amqpvalue_set_list_item(attach_frame_list, 5, source) != 0) ||
			(amqpvalue_set_list_item(attach_frame_list, 6, target) != 0))
		{
			result = __LINE__;
		}
		else
		{
			FRAME_CODEC_HANDLE frame_codec;
			if (((frame_codec = session_get_frame_codec(link->session)) == NULL) ||
				(frame_codec_encode(frame_codec, 0x12, &attach_frame_list, 1) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}

		amqpvalue_destroy(name_value);
		amqpvalue_destroy(handle_value);
		amqpvalue_destroy(role_value);
		amqpvalue_destroy(snd_settle_mode_value);
		amqpvalue_destroy(rcv_settle_mode_value);
		amqpvalue_destroy(attach_frame_list);
	}

	return result;
}

static int send_tranfer(LINK_DATA* link, const AMQP_VALUE* payload_chunks, size_t payload_chunk_count)
{
	int result;
	AMQP_VALUE transfer_frame_list = amqpvalue_create_list(3);
	if (transfer_frame_list == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE handle_value = amqpvalue_create_handle(link->handle);
		AMQP_VALUE delivery_id_value = amqpvalue_create_delivery_number(link->delivery_id++);
		AMQP_VALUE delivery_tag_value = amqpvalue_create_delivery_tag(&link->delivery_id, sizeof(link->delivery_id));

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
			AMQP_VALUE* chunks = malloc(sizeof(AMQP_VALUE) * (payload_chunk_count + 1));
			if (chunks == NULL)
			{
				result = __LINE__;
			}
			else
			{
				FRAME_CODEC_HANDLE frame_codec;
				size_t i;

				chunks[0] = transfer_frame_list;
				for (i = 0; i < payload_chunk_count; i++)
				{
					chunks[i + 1] = payload_chunks[i];
				}

				if (((frame_codec = session_get_frame_codec(link->session)) == NULL) ||
					(frame_codec_encode(frame_codec, 0x14, chunks, payload_chunk_count + 1) != 0))
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}

				free(chunks);
			}
		}

		amqpvalue_destroy(handle_value);
		amqpvalue_destroy(delivery_id_value);
		amqpvalue_destroy(delivery_tag_value);
		amqpvalue_destroy(transfer_frame_list);
	}

	return result;
}

LINK_HANDLE link_create(SESSION_HANDLE session, AMQP_VALUE source, AMQP_VALUE target)
{
	LINK_DATA* result = malloc(sizeof(LINK_DATA));
	if (result != NULL)
	{
		if (session_set_frame_received_callback(session, link_frame_received, result) != 0)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->link_state = LINK_STATE_DETACHED;
			result->source = source;
			result->target = target;
			result->session = session;
			result->handle = 1;
			result->delivery_id = 0;
		}
	}

	return result;
}

void link_destroy(LINK_HANDLE handle)
{
	free(handle);
}

int link_dowork(LINK_HANDLE handle)
{
	int result;
	LINK_DATA* link = (LINK_DATA*)handle;
	SESSION_STATE session_state;

	if (session_get_state(link->session, &session_state) != 0)
	{
		result = __LINE__;
	}
	else
	{
		result = 0;

		if (session_state == SESSION_STATE_MAPPED)
		{
			if (link->link_state == LINK_STATE_DETACHED)
			{
				if (send_attach(link, "fake", 1, sender, unsettled, first, link->source, link->target) != 0)
				{
					result = __LINE__;
				}

				link->link_state = LINK_STATE_HALF_ATTACHED;
			}
		}
	}

	return result;
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
