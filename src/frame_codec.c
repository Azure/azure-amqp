#include <stdlib.h>
#include <stdint.h>
#include "frame_codec.h"
#include "amqpvalue.h"
#include "encoder.h"
#include "logger.h"
#include "io.h"

#define FRAME_HEADER_SIZE 8

typedef struct FRAME_CODEC_DATA_TAG
{
	IO_HANDLE io;
	LOGGER_LOG logger_log;
} FRAME_CODEC_DATA;

static int frame_codec_write_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, LOGGER_LOG logger_log)
{
	FRAME_CODEC_DATA* result;
	result = malloc(sizeof(FRAME_CODEC_DATA));
	if (result != NULL)
	{
		result->io = io;
		result->logger_log = logger_log;
	}

	return result;
}

void frame_codec_free(FRAME_CODEC_HANDLE handle)
{
	free(handle);
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
			frame_codec->logger_log("\r\n-> [Begin]\r\n");
		}

		if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
			(encoder_encode_ulong(encoder_handle, 0x11) != 0) ||
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
				(encoder_encode_ulong(encoder_handle, 0x11) != 0) ||
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