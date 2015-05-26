#include <stdlib.h>
#include <stdint.h>
#include "frame_codec.h"
#include "amqpvalue.h"
#include "encoder.h"
#include "decoder.h"
#include "logger.h"
#include "io.h"
#include "amqpalloc.h"

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
	FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* frame_received_callback_context;
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_bytes;
	size_t receive_frame_consumed_bytes;
	uint32_t receive_frame_size;
	unsigned char receive_frame_buffer[512];
} FRAME_CODEC_DATA;

int frame_codec_encode_frame_bytes(FRAME_CODEC_HANDLE frame_codec, const void* bytes, size_t length)
{
	int result;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	if (frame_codec_data->io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if (io_send(frame_codec_data->io, bytes, length) != 0)
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

int frame_codec_encode_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

static int decode_received_sasl_frame(FRAME_CODEC_DATA* frame_codec)
{
	/* not implemented */
	return __LINE__;
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
			uint8_t doff = frame_codec->receive_frame_buffer[4];
			uint32_t frame_body_offset = (doff * 4);
			
			frame_codec->frame_received_callback(frame_codec->frame_received_callback_context, frame_codec->receive_frame_buffer[5],
				&frame_codec->receive_frame_buffer[frame_body_offset], frame_codec->receive_frame_bytes - frame_body_offset,
				&frame_codec->receive_frame_buffer[6], frame_body_offset - 6);

			result = 0;

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

	/* Codes_SRS_FRAME_CODEC_01_020: [If the io argument is NULL, frame_codec_create shall return NULL.] */
	if (io == NULL)
	{
		result = NULL;
	}
	else
	{
		result = amqpalloc_malloc(sizeof(FRAME_CODEC_DATA));
		/* Codes_SRS_FRAME_CODEC_01_022: [If allocating memory for the frame_codec instance fails, frame_codec_create shall return NULL.] */
		if (result != NULL)
		{
			/* Codes_SRS_FRAME_CODEC_01_021: [frame_codec_create shall create a new instance of frame_codec and return a non-NULL handle to it on success.] */
			result->io = io;
			result->logger_log = logger_log;
			result->frame_received_callback = NULL;
			result->frame_received_callback_context = NULL;
			result->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			result->receive_frame_bytes = 0;
			result->receive_frame_consumed_bytes = 0;
		}
	}

	return result;
}

void frame_codec_destroy(FRAME_CODEC_HANDLE frame_codec)
{
	/* Codes_SRS_FRAME_CODEC_01_024: [If frame_codec is NULL, frame_codec_destroy shall do nothing.] */
	if (frame_codec != NULL)
	{
		/* Codes_SRS_FRAME_CODEC_01_023: [frame_codec_destroy shall free all resources associated with a frame_codec instance.] */
		amqpalloc_free(frame_codec);
	}
}

int frame_codec_receive_bytes(FRAME_CODEC_HANDLE frame_codec, const void* buffer, size_t size)
{
	int result;
	size_t i;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	if (frame_codec == NULL)
	{
		result = __LINE__;
	}
	else
	{
		for (i = 0; i < size; i++)
		{
			if (receive_frame_byte(frame_codec_data, ((unsigned char*)buffer)[i]) != 0)
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
			/* Codes_SRS_FRAME_CODEC_01_025: [frame_codec_receive_bytes decodes a sequence of bytes into frames and on success it returns zero.] */
			result = 0;
		}
	}

	return result;
}

int frame_codec_start_encode_frame(FRAME_CODEC_HANDLE frame_codec, size_t frame_payload_size)
{
	int result;
	ENCODER_HANDLE encoder_handle;
	size_t frame_size = frame_payload_size + FRAME_HEADER_SIZE;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	uint8_t doff = 2;
	uint8_t type = 0;
	uint16_t channel = 0;

	encoder_handle = encoder_create(frame_codec_encode_bytes, frame_codec_data->io);
	if (encoder_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		unsigned char b;

		b = (frame_size >> 24) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (frame_size >> 16) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (frame_size >> 8) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (frame_size)& 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		(void)io_send(frame_codec_data->io, &doff, sizeof(doff));
		(void)io_send(frame_codec_data->io, &type, sizeof(type));
		b = (channel >> 8) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (channel)& 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);

		result = 0;
	}

	return result;
}

int frame_codec_subscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type, FRAME_RECEIVED_CALLBACK frame_received_callback, void* frame_received_callback_context)
{
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	frame_codec_data->frame_received_callback = frame_received_callback;
	frame_codec_data->frame_received_callback_context = frame_received_callback_context;
	return 0;
}

int frame_codec_unsubscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type)
{
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	frame_codec_data->frame_received_callback = NULL;
	frame_codec_data->frame_received_callback_context = NULL;
	return 0;
}
