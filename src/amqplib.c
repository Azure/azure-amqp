#include <stdlib.h>
#include <stdint.h>
#include "io.h"
#include "consolelogger.h"
#include "amqplib.h"
#include "socketio.h"
#include "platform.h"
#include "encoder.h"
#include "decoder.h"

typedef enum CONNECTION_STATE_TAG
{
	CONNECTION_STATE_START,
	CONNECTION_STATE_HDR_RCVD,
	CONNECTION_STATE_HDR_SENT,
	CONNECTION_STATE_HDR_EXCH,
	CONNECTION_STATE_OPEN_PIPE,
	CONNECTION_STATE_OC_PIPE,
	CONNECTION_STATE_OPEN_RCVD,
	CONNECTION_STATE_OPEN_SENT,
	CONNECTION_STATE_CLOSE_PIPE,
	CONNECTION_STATE_OPENED,
	CONNECTION_STATE_CLOSE_RCVD,
	CONNECTION_STATE_CLOSE_SENT,
	CONNECTION_STATE_DISCARDING,
	CONNECTION_STATE_END
} CONNECTION_STATE;

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_FRAME_DATA
} RECEIVE_FRAME_STATE;

typedef struct AMQPLIB_DATA_TAG
{
	IO_HANDLE socket_io;
	IO_HANDLE used_io;
	size_t header_bytes_received;
	CONNECTION_STATE connection_state;
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_bytes;
	size_t receive_frame_consumed_bytes;
	uint32_t receive_frame_size;
	unsigned char receive_frame_buffer[2048];
} AMQPLIB_DATA;

int amqplib_init(void)
{
	int result;

	if (platform_init())
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

void amqplib_deinit(void)
{
	platform_deinit();
}

static unsigned char amqp_header[] = { 'A', 'M', 'Q', 'P', 0, 1, 0, 0 };

#define FRAME_HEADER_SIZE 8

static int connection_frame_write_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

static int connection_encode_open(ENCODER_HANDLE encoderHandle, const char* container_id)
{
	int result;

	if ((encoder_encode_descriptor_header(encoderHandle) != 0) ||
		(encoder_encode_ulong(encoderHandle, 0x10) != 0) ||
		(encoder_encode_string(encoderHandle, container_id) != 0))
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

static int connection_send_open(AMQPLIB_DATA* amqp_lib, const char* container_id)
{
	uint32_t frame_size;
	uint8_t doff = 2;
	uint8_t type = 0;
	uint16_t channel = 0;
	ENCODER_HANDLE encoderHandle = encoder_create(NULL, NULL);
	int result;

	if (encoderHandle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		consolelogger_log("\r\n-> [Open] container-id=%s\r\n", container_id);

		if ((connection_encode_open(encoderHandle, container_id) != 0) ||
			(encoder_get_encoded_size(encoderHandle, &frame_size) != 0))
		{
			result = __LINE__;
		}
		else
		{
			frame_size += FRAME_HEADER_SIZE;
			result = 0;
		}

		encoder_destroy(encoderHandle);
	}

	if (result == 0)
	{
		encoderHandle = encoder_create(connection_frame_write_bytes, amqp_lib->used_io);
		if (encoderHandle == NULL)
		{
			result = __LINE__;
		}
		else
		{
			unsigned char b;

			b = (frame_size >> 24) & 0xFF;
			(void)io_send(amqp_lib->used_io, &b, 1);
			b = (frame_size >> 16) & 0xFF;
			(void)io_send(amqp_lib->used_io, &b, 1);
			b = (frame_size >> 8) & 0xFF;
			(void)io_send(amqp_lib->used_io, &b, 1);
			b = (frame_size) & 0xFF;
			(void)io_send(amqp_lib->used_io, &b, 1);
			(void)io_send(amqp_lib->used_io, &doff, sizeof(doff));
			(void)io_send(amqp_lib->used_io, &type, sizeof(type));
			b = (channel >> 8) & 0xFF;
			(void)io_send(amqp_lib->used_io, &b, 1);
			b = (channel) & 0xFF;
			(void)io_send(amqp_lib->used_io, &b, 1);

			if (connection_encode_open(encoderHandle, container_id) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			encoder_destroy(encoderHandle);
		}
	}

	return 0;
}

static int connection_decode_received_amqp_frame(AMQPLIB_DATA* amqp_lib)
{
	uint16_t channel;
	uint8_t doff = amqp_lib->receive_frame_buffer[4];
	unsigned char* frame_body;
	uint32_t frame_body_size = amqp_lib->receive_frame_size - doff * 4;
	DECODER_HANDLE decoder_handle;
	AMQP_VALUE descriptor;
	AMQP_VALUE container_id;
	int result;
	bool more;

	channel = amqp_lib->receive_frame_buffer[6] << 8;
	channel += amqp_lib->receive_frame_buffer[7];

	frame_body = &amqp_lib->receive_frame_buffer[4 * doff];
	decoder_handle = decoder_create(frame_body, frame_body_size);

	if ((decoder_decode(decoder_handle, &descriptor, &more) != 0) ||
		(!more) ||
		(decoder_decode(decoder_handle, &container_id, &more) != 0))
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

static int connection_decode_received_sasl_frame(AMQPLIB_DATA* amqp_lib)
{
	/* not implemented */
	return __LINE__;
}

static int connection_decode_received_frame(AMQPLIB_DATA* amqp_lib)
{
	int result;

	/* decode type */
	uint8_t type = amqp_lib->receive_frame_buffer[5];

	switch (type)
	{
		default:
			consolelogger_log("Unknown frame.\r\n");
			result = __LINE__;
			break;

		case 0:
			result = connection_decode_received_amqp_frame(amqp_lib);
			break;

		case 1:
			result = connection_decode_received_sasl_frame(amqp_lib);
			break;
	}

	return result;
}

static int connection_receive_frame(AMQPLIB_DATA* amqp_lib, unsigned char b)
{
	int result;

	amqp_lib->receive_frame_buffer[amqp_lib->receive_frame_bytes] = b;
	amqp_lib->receive_frame_bytes++;

	switch (amqp_lib->receive_frame_state)
	{
		default:
			result = __LINE__;

		case RECEIVE_FRAME_STATE_FRAME_SIZE:
			if (amqp_lib->receive_frame_bytes - amqp_lib->receive_frame_consumed_bytes >= 4)
			{
				amqp_lib->receive_frame_size = amqp_lib->receive_frame_buffer[amqp_lib->receive_frame_consumed_bytes++] << 24;
				amqp_lib->receive_frame_size += amqp_lib->receive_frame_buffer[amqp_lib->receive_frame_consumed_bytes++] << 16;
				amqp_lib->receive_frame_size += amqp_lib->receive_frame_buffer[amqp_lib->receive_frame_consumed_bytes++] << 8;
				amqp_lib->receive_frame_size += amqp_lib->receive_frame_buffer[amqp_lib->receive_frame_consumed_bytes++];
				amqp_lib->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_DATA;
			}

			result = 0;
			break;

		case RECEIVE_FRAME_STATE_FRAME_DATA:
			if (amqp_lib->receive_frame_bytes - amqp_lib->receive_frame_consumed_bytes == amqp_lib->receive_frame_size - 4)
			{
				/* done receiving */
				if (connection_decode_received_frame(amqp_lib) != 0)
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}

				amqp_lib->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
				amqp_lib->receive_frame_bytes = 0;
				amqp_lib->receive_frame_consumed_bytes = 0;
			}
			else
			{
				result = 0;
			}
			break;
	}

	return result;
}

static void connection_byte_received(AMQPLIB_DATA* amqp_lib, unsigned char b)
{
	switch (amqp_lib->connection_state)
	{
		default:
			break;

		case CONNECTION_STATE_HDR_SENT:
			if (b != amqp_header[amqp_lib->header_bytes_received])
			{
				/* close connection */
				io_destroy(amqp_lib->used_io);
				amqp_lib->connection_state = CONNECTION_STATE_END;
			}
			else
			{
				amqp_lib->header_bytes_received++;
				if (amqp_lib->header_bytes_received == sizeof(amqp_header))
				{
					amqp_lib->connection_state = CONNECTION_STATE_HDR_EXCH;
					amqp_lib->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
					amqp_lib->receive_frame_bytes = 0;
					amqp_lib->receive_frame_consumed_bytes = 0;

					/* handshake done, send open frame */
					if (connection_send_open(amqp_lib, "1") != 0)
					{
						io_destroy(amqp_lib->used_io);
						amqp_lib->connection_state = CONNECTION_STATE_END;
					}
					else
					{
						amqp_lib->connection_state = CONNECTION_STATE_OPEN_SENT;
					}
				}
			}
			break;

		case CONNECTION_STATE_OPEN_SENT:
			(void)connection_receive_frame(amqp_lib, b);
			break;
	}
}

static void connection_receive_callback(IO_HANDLE handle, void* context, const void* buffer, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++)
	{
		connection_byte_received((AMQPLIB_DATA*)context, ((unsigned char*)buffer)[i]);
	}
}

AMQPLIB_HANDLE amqplib_create(const char* host, int port)
{
	AMQPLIB_DATA* result = malloc(sizeof(AMQPLIB_DATA));
	if (result != NULL)
	{
		SOCKETIO_CONFIG socket_io_config = { host, port };
		result->socket_io = io_create(socketio_get_interface_description(), &socket_io_config, connection_receive_callback, result, consolelogger_log);
		if (result->socket_io == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->connection_state = CONNECTION_STATE_START;
			result->header_bytes_received = 0;

			/* For now directly talk to the socket IO. By doing this there is no SASL, no SSL, pure AMQP only */
			result->used_io = result->socket_io;
		}
	}

	return result;
}

void amqplib_destroy(AMQPLIB_HANDLE handle)
{
	if (handle != NULL)
	{
		AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;
		io_destroy(amqp_lib->socket_io);
		free(handle);
	}
}

static int connection_sendheader(AMQPLIB_DATA* amqp_lib)
{
	int result;

	if (io_send(amqp_lib->used_io, amqp_header, sizeof(amqp_header)) != 0)
	{
		result = __LINE__;
	}
	else
	{
		amqp_lib->connection_state = CONNECTION_STATE_HDR_SENT;
		result = 0;
	}

	return result;
}

static int connection_dowork(AMQPLIB_DATA* amqp_lib)
{
	int result;

	switch (amqp_lib->connection_state)
	{
		default:
			result = __LINE__;
			break;

		case CONNECTION_STATE_START:
			result = connection_sendheader(amqp_lib);
			break;

        case CONNECTION_STATE_HDR_SENT:
        case CONNECTION_STATE_OPEN_SENT:
            result = 0;
            break;

        case CONNECTION_STATE_HDR_EXCH:
            connection_send_open(amqp_lib, "1");
            break;

        case CONNECTION_STATE_OPEN_RCVD:
            /* peer wants to open, let's panic */
            result = __LINE__;
            break;
	}

	return result;
}

int amqplib_dowork(AMQPLIB_HANDLE handle)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQPLIB_DATA* amqp_lib = (AMQPLIB_DATA*)handle;

		result = connection_dowork(amqp_lib);
		if (result == 0)
		{
			result = io_dowork(amqp_lib->socket_io);
		}
	}

	return result;
}
