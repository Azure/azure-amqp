#include <stdlib.h>
#include "session.h"
#include "connection.h"
#include "amqpvalue.h"
#include "amqp_protocol_types.h"
#include "encoder.h"
#include "consolelogger.h"

#define FRAME_HEADER_SIZE 8

typedef enum SESION_STATE_TAG
{
	SESSION_STATE_UNMAPPED,
	SESSION_STATE_BEGIN_SENT,
	SESSION_STATE_BEGIN_RCVD,
	SESSION_STATE_MAPPED,
	SESSION_STATE_END_SENT,
	SESSION_STATE_END_RCVD,
	SESSION_STATE_DISCARDING
} SESSION_STATE;

typedef struct SESSION_DATA_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_STATE session_state;
} SESSION_DATA;

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_DATA* result = malloc(sizeof(SESSION_DATA));
	if (result != NULL)
	{
		result->session_state = SESSION_STATE_UNMAPPED;
		result->connection = connection;
	}

	return result;
}

void session_destroy(SESSION_HANDLE handle)
{
	free(handle);
}

static int session_frame_write_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

static int send_begin(SESSION_DATA* session_data, transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
{
	int result;
	AMQP_VALUE begin_list_value = amqpvalue_create_list(1);
	if (begin_list_value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		uint32_t frame_size;
		uint8_t doff = 2;
		uint8_t type = 0;
		uint16_t channel = 0;
		ENCODER_HANDLE encoder_handle;

		AMQP_VALUE next_outgoing_id_value = amqpvalue_create_transfer_number(next_outgoing_id);
		AMQP_VALUE incoming_window_value = amqpvalue_create_transfer_number(incoming_window);
		AMQP_VALUE outgoing_window_value = amqpvalue_create_transfer_number(outgoing_window);
		/* do not set remote_channel for now */

		if ((next_outgoing_id_value == NULL) ||
			(incoming_window_value == NULL) ||
			(outgoing_window_value == NULL) ||
			(amqpvalue_set_list_item(begin_list_value, 0, next_outgoing_id_value) != 0) ||
			(amqpvalue_set_list_item(begin_list_value, 1, incoming_window_value) != 0) ||
			(amqpvalue_set_list_item(begin_list_value, 2, outgoing_window_value) != 0))
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}

		encoder_handle = encoder_create(NULL, NULL);

		if (encoder_handle == NULL)
		{
			result = __LINE__;
		}
		else
		{
			consolelogger_log("\r\n-> [Begin]\r\n");

			if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
				(encoder_encode_ulong(encoder_handle, 0x11) != 0) ||
				(encoder_encode_amqp_value(encoder_handle, begin_list_value) != 0) ||
				(encoder_get_encoded_size(encoder_handle, &frame_size) != 0))
			{
				result = __LINE__;
			}
			else
			{
				frame_size += FRAME_HEADER_SIZE;
				result = 0;
			}

			encoder_destroy(encoder_handle);
		}

		if (result == 0)
		{
			IO_HANDLE io = connection_get_io(session_data->connection);
			encoder_handle = encoder_create(session_frame_write_bytes, io);
			if (encoder_handle == NULL)
			{
				result = __LINE__;
			}
			else
			{
				unsigned char b;

				b = (frame_size >> 24) & 0xFF;
				(void)io_send(io, &b, 1);
				b = (frame_size >> 16) & 0xFF;
				(void)io_send(io, &b, 1);
				b = (frame_size >> 8) & 0xFF;
				(void)io_send(io, &b, 1);
				b = (frame_size)& 0xFF;
				(void)io_send(io, &b, 1);
				(void)io_send(io, &doff, sizeof(doff));
				(void)io_send(io, &type, sizeof(type));
				b = (channel >> 8) & 0xFF;
				(void)io_send(io, &b, 1);
				b = (channel)& 0xFF;
				(void)io_send(io, &b, 1);

				if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
					(encoder_encode_ulong(encoder_handle, 0x11) != 0) ||
					(encoder_encode_amqp_value(encoder_handle, begin_list_value) != 0))
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}

				encoder_destroy(encoder_handle);
			}
		}

		amqpvalue_destroy(next_outgoing_id_value);
		amqpvalue_destroy(incoming_window_value);
		amqpvalue_destroy(outgoing_window_value);
		amqpvalue_destroy(begin_list_value);
	}

	return result;
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
