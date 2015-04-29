#include <stdlib.h>
#include <stdint.h>
#include "frame_codec.h"
#include "amqpvalue.h"
#include "encoder.h"
#include "decoder.h"
#include "logger.h"
#include "io.h"

#define FRAME_HEADER_SIZE 8

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_FRAME_DATA
} RECEIVE_FRAME_STATE;

typedef struct FRAME_CODEC_DATA_TAG
{
	IO_HANDLE io;
	LOGGER_LOG logger_log;
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_bytes;
	size_t receive_frame_consumed_bytes;
	uint32_t receive_frame_size;
	unsigned char receive_frame_buffer[2048];
} FRAME_CODEC_DATA;

static int frame_codec_write_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

static const char* performative_name(uint64_t performative)
{
	const char* result;
	switch (performative)
	{
	default:
		result = "Unknown";
		break;

	case 0x10:
		result = "open";
		break;

	case 0x11:
		result = "begin";
		break;
	}

	return result;
}

static int decode_received_amqp_frame(FRAME_CODEC_DATA* frame_codec)
{
	uint16_t channel;
	uint8_t doff = frame_codec->receive_frame_buffer[4];
	unsigned char* frame_body;
	uint32_t frame_body_size = frame_codec->receive_frame_size - doff * 4;
	DECODER_HANDLE decoder_handle;
	AMQP_VALUE descriptor;
	AMQP_VALUE container_id;
	int result;
	bool more;

	channel = frame_codec->receive_frame_buffer[6] << 8;
	channel += frame_codec->receive_frame_buffer[7];

	frame_body = &frame_codec->receive_frame_buffer[4 * doff];
	decoder_handle = decoder_create(frame_body, frame_body_size);

	if ((decoder_decode(decoder_handle, &descriptor, &more) != 0) ||
		(!more) ||
		(decoder_decode(decoder_handle, &container_id, &more) != 0))
	{
		result = __LINE__;
	}
	else
	{
		/* notify of received frame */
/*		if (frame_codec->frame_codec_state == frame_codec_STATE_OPEN_SENT)
		{
			frame_codec->frame_codec_state = frame_codec_STATE_OPENED;
		}*/

		result = 0;
	}

	return result;
}

static int decode_received_sasl_frame(FRAME_CODEC_DATA* frame_codec)
{
	/* not implemented */
	return __LINE__;
}

static int decode_received_frame(FRAME_CODEC_DATA* frame_codec)
{
	int result;

	/* decode type */
	uint8_t type = frame_codec->receive_frame_buffer[5];

	switch (type)
	{
	default:
		frame_codec->logger_log("Unknown frame.\r\n");
		result = __LINE__;
		break;

	case 0:
		result = decode_received_amqp_frame(frame_codec);
		break;

	case 1:
		result = decode_received_sasl_frame(frame_codec);
		break;
	}

	return result;
}

static int receive_frame_byte(FRAME_CODEC_DATA* frame_codec, unsigned char b)
{
	int result;

	frame_codec->receive_frame_buffer[frame_codec->receive_frame_bytes] = b;
	frame_codec->receive_frame_bytes++;

	switch (frame_codec->receive_frame_state)
	{
	default:
		result = __LINE__;

	case RECEIVE_FRAME_STATE_FRAME_SIZE:
		if (frame_codec->receive_frame_bytes - frame_codec->receive_frame_consumed_bytes >= 4)
		{
			frame_codec->receive_frame_size = frame_codec->receive_frame_buffer[frame_codec->receive_frame_consumed_bytes++] << 24;
			frame_codec->receive_frame_size += frame_codec->receive_frame_buffer[frame_codec->receive_frame_consumed_bytes++] << 16;
			frame_codec->receive_frame_size += frame_codec->receive_frame_buffer[frame_codec->receive_frame_consumed_bytes++] << 8;
			frame_codec->receive_frame_size += frame_codec->receive_frame_buffer[frame_codec->receive_frame_consumed_bytes++];
			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_DATA;
		}

		result = 0;
		break;

	case RECEIVE_FRAME_STATE_FRAME_DATA:
		if (frame_codec->receive_frame_bytes - frame_codec->receive_frame_consumed_bytes == frame_codec->receive_frame_size - 4)
		{
			/* done receiving */
			if (decode_received_frame(frame_codec) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			frame_codec->receive_frame_bytes = 0;
			frame_codec->receive_frame_consumed_bytes = 0;
		}
		else
		{
			result = 0;
		}
		break;
	}

	return result;
}

FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, LOGGER_LOG logger_log)
{
	FRAME_CODEC_DATA* result;
	result = malloc(sizeof(FRAME_CODEC_DATA));
	if (result != NULL)
	{
		result->io = io;
		result->logger_log = logger_log;
		result->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
		result->receive_frame_bytes = 0;
		result->receive_frame_consumed_bytes = 0;
	}

	return result;
}

void frame_codec_destroy(FRAME_CODEC_HANDLE handle)
{
	free(handle);
}

int frame_codec_receive_bytes(FRAME_CODEC_HANDLE handle, const void* buffer, size_t size)
{
	int result;
	size_t i;
	FRAME_CODEC_DATA* frame_codec = (FRAME_CODEC_DATA*)handle;

	if (frame_codec == NULL)
	{
		result = __LINE__;
	}
	else
	{
		for (i = 0; i < size; i++)
		{
			if (receive_frame_byte(frame_codec, ((unsigned char*)buffer)[i]) != 0)
			{
				break;
			}
		}

		if (i < size)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

int frame_codec_encode(FRAME_CODEC_HANDLE handle, uint64_t performative, AMQP_VALUE frame_content)
{
	int result;
	ENCODER_HANDLE encoder_handle = encoder_create(NULL, NULL);
	uint32_t frame_size;
	uint8_t doff = 2;
	uint8_t type = 0;
	uint16_t channel = 0;
	FRAME_CODEC_DATA* frame_codec = (FRAME_CODEC_DATA*)handle;

	if (encoder_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if (frame_codec->logger_log != NULL)
		{
			frame_codec->logger_log("\r\n-> [%s]\r\n", performative_name(performative));
		}

		if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
			(encoder_encode_ulong(encoder_handle, performative) != 0) ||
			(encoder_encode_amqp_value(encoder_handle, frame_content) != 0) ||
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
		encoder_handle = encoder_create(frame_codec_write_bytes, frame_codec->io);
		if (encoder_handle == NULL)
		{
			result = __LINE__;
		}
		else
		{
			unsigned char b;

			b = (frame_size >> 24) & 0xFF;
			(void)io_send(frame_codec->io, &b, 1);
			b = (frame_size >> 16) & 0xFF;
			(void)io_send(frame_codec->io, &b, 1);
			b = (frame_size >> 8) & 0xFF;
			(void)io_send(frame_codec->io, &b, 1);
			b = (frame_size)& 0xFF;
			(void)io_send(frame_codec->io, &b, 1);
			(void)io_send(frame_codec->io, &doff, sizeof(doff));
			(void)io_send(frame_codec->io, &type, sizeof(type));
			b = (channel >> 8) & 0xFF;
			(void)io_send(frame_codec->io, &b, 1);
			b = (channel)& 0xFF;
			(void)io_send(frame_codec->io, &b, 1);

			if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
				(encoder_encode_ulong(encoder_handle, performative) != 0) ||
				(encoder_encode_amqp_value(encoder_handle, frame_content) != 0))
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

	return result;
}
