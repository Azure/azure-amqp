#include <stdlib.h>
#include <stdint.h>
#include "session.h"
#include "connection.h"
#include "amqpvalue.h"
#include "amqp_protocol_types.h"

typedef struct SESSION_DATA_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_STATE session_state;
	SESSION_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* frame_received_callback_context;
} SESSION_DATA;

static int send_begin(SESSION_DATA* session_data, transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
{
	int result;
	AMQP_VALUE begin_frame_list = amqpvalue_create_list(4);
	if (begin_frame_list == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE remote_channel_value = amqpvalue_create_null();
		AMQP_VALUE next_outgoing_id_value = amqpvalue_create_transfer_number(next_outgoing_id);
		AMQP_VALUE incoming_window_value = amqpvalue_create_transfer_number(incoming_window);
		AMQP_VALUE outgoing_window_value = amqpvalue_create_transfer_number(outgoing_window);
		/* do not set remote_channel for now */

		if ((remote_channel_value == NULL) ||
			(next_outgoing_id_value == NULL) ||
			(incoming_window_value == NULL) ||
			(outgoing_window_value == NULL) ||
			(amqpvalue_set_list_item(begin_frame_list, 0, remote_channel_value) != 0) ||
			(amqpvalue_set_list_item(begin_frame_list, 1, next_outgoing_id_value) != 0) ||
			(amqpvalue_set_list_item(begin_frame_list, 2, incoming_window_value) != 0) ||
			(amqpvalue_set_list_item(begin_frame_list, 3, outgoing_window_value) != 0))
		{
			result = __LINE__;
		}
		else
		{
			FRAME_CODEC_HANDLE frame_codec;
			if (((frame_codec = connection_get_frame_codec(session_data->connection)) == NULL) ||
				(frame_codec_encode(frame_codec, 0x11, begin_frame_list) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}

		amqpvalue_destroy(remote_channel_value);
		amqpvalue_destroy(next_outgoing_id_value);
		amqpvalue_destroy(incoming_window_value);
		amqpvalue_destroy(outgoing_window_value);
		amqpvalue_destroy(begin_frame_list);
	}

	return result;
}

static void frame_received(void* context, uint64_t performative, AMQP_VALUE frame_list_value)
{
	SESSION_DATA* session = (SESSION_DATA*)context;
	if (performative == 0x11)
	{
		switch (session->session_state)
		{
		default:
			break;

		case SESSION_STATE_BEGIN_SENT:
			session->session_state = SESSION_STATE_MAPPED;
			break;
		}
	}
}

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_DATA* result = malloc(sizeof(SESSION_DATA));
	if (result != NULL)
	{
		if (connection_set_session_frame_receive_callback(connection, frame_received, result) != 0)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->frame_received_callback = NULL;
			result->session_state = SESSION_STATE_UNMAPPED;
			result->connection = connection;
		}
	}

	return result;
}

void session_destroy(SESSION_HANDLE handle)
{
	free(handle);
}

int session_dowork(SESSION_HANDLE handle)
{
	int result;
	SESSION_DATA* session_data = (SESSION_DATA*)handle;

	switch (session_data->session_state)
	{
		default:
			result = __LINE__;
			break;

		case SESSION_STATE_UNMAPPED:
		{
			CONNECTION_STATE connection_state;
			if (connection_get_state(session_data->connection, &connection_state) != 0)
			{
				result = __LINE__;
			}
			else
			{
				if (connection_state == CONNECTION_STATE_OPENED)
				{
					result = send_begin(session_data, 0, 1, 1);
					if (result == 0)
					{
						session_data->session_state = SESSION_STATE_BEGIN_SENT;
					}
				}
				else
				{
					result = 0;
				}
			}
			break;
		}
	}

	return result;
}

int session_set_frame_received_callback(SESSION_HANDLE handle, SESSION_FRAME_RECEIVED_CALLBACK callback, void* context)
{
	int result;

	SESSION_DATA* session = (SESSION_DATA*)handle;
	if (session == NULL)
	{
		session->frame_received_callback = callback;
		session->frame_received_callback_context = context;
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

int session_get_state(SESSION_HANDLE handle, SESSION_STATE* session_state)
{
	int result;

	SESSION_DATA* session = (SESSION_DATA*)handle;
	if (session == NULL)
	{
		result = __LINE__;
	}
	else
	{
		*session_state = session->session_state;
		result = 0;
	}

	return result;
}

FRAME_CODEC_HANDLE session_get_frame_codec(SESSION_HANDLE handle)
{
	FRAME_CODEC_HANDLE result;

	SESSION_DATA* session = (SESSION_DATA*)handle;
	if (session == NULL)
	{
		result = NULL;
	}
	else
	{
		result = connection_get_frame_codec(session->connection);
	}

	return result;
}
