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
	RECEIVE_FRAME_STATE_DOFF,
	RECEIVE_FRAME_STATE_FRAME_TYPE,
	RECEIVE_FRAME_STATE_TYPE_SPECIFIC,
	RECEIVE_FRAME_STATE_FRAME_BODY,
	RECEIVE_FRAME_STATE_ERROR
} RECEIVE_FRAME_STATE;

typedef struct FRAME_CODEC_DATA_TAG
{
	IO_HANDLE io;
	LOGGER_LOG logger_log;
	FRAME_BEGIN_CALLBACK frame_begin_callback;
	FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback;
	void* callback_context;
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_pos;
	uint32_t receive_frame_size;
	uint8_t receive_frame_doff;
	uint8_t receive_frame_type;
	uint32_t max_frame_size;
	unsigned char* receive_frame_type_specific;
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

/* Codes_SRS_FRAME_CODEC_01_001: [Frames are divided into three distinct areas: a fixed width frame header, a variable width extended header, and a variable width frame body.] */
/* Codes_SRS_FRAME_CODEC_01_002: [frame header The frame header is a fixed size (8 byte) structure that precedes each frame.] */
/* Codes_SRS_FRAME_CODEC_01_003: [The frame header includes mandatory information necessary to parse the rest of the frame including size and type information.] */
/* Codes_SRS_FRAME_CODEC_01_004: [extended header The extended header is a variable width area preceding the frame body.] */
/* Codes_SRS_FRAME_CODEC_01_007: [frame body The frame body is a variable width sequence of bytes the format of which depends on the frame type.] */
/* Codes_SRS_FRAME_CODEC_01_028: [The sequence of bytes shall be decoded according to the AMQP ISO.] */
/* Codes_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
static int receive_frame_byte(FRAME_CODEC_DATA* frame_codec, unsigned char b)
{
	int result;

	switch (frame_codec->receive_frame_state)
	{
	default:
	/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_receive_bytes shall fail.] */
	case RECEIVE_FRAME_STATE_ERROR:
		result = __LINE__;
		break;

	/* Codes_SRS_FRAME_CODEC_01_008: [SIZE Bytes 0-3 of the frame header contain the frame size.] */
	case RECEIVE_FRAME_STATE_FRAME_SIZE:
		/* Codes_SRS_FRAME_CODEC_01_009: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
		frame_codec->receive_frame_size += b << (24 - frame_codec->receive_frame_pos * 8);
		frame_codec->receive_frame_pos++;

		if (frame_codec->receive_frame_pos == 4)
		{
			/* Codes_SRS_FRAME_CODEC_01_010: [The frame is malformed if the size is less than the size of the frame header (8 bytes).] */
			if (frame_codec->receive_frame_size < FRAME_HEADER_SIZE)
			{
				/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_receive_bytes shall fail.] */
				frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
				result = __LINE__;
			}
			else
			{
				frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_DOFF;
				result = 0;
			}
		}
		else
		{
			result = 0;
		}

		break;

	case RECEIVE_FRAME_STATE_DOFF:
		/* Codes_SRS_FRAME_CODEC_01_011: [DOFF Byte 4 of the frame header is the data offset.] */
		/* Codes_SRS_FRAME_CODEC_01_013: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
		/* Codes_SRS_FRAME_CODEC_01_012: [This gives the position of the body within the frame.] */
		frame_codec->receive_frame_doff = b;
		/* Codes_SRS_FRAME_CODEC_01_014: [Due to the mandatory 8-byte frame header, the frame is malformed if the value is less than 2.] */
		if (frame_codec->receive_frame_doff < 2)
		{
			/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_receive_bytes shall fail.] */
			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
			result = __LINE__;
		}
		else
		{
			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_TYPE;
			result = 0;
		}

		break;

	case RECEIVE_FRAME_STATE_FRAME_TYPE:
	{
		uint32_t type_specific_size = (frame_codec->receive_frame_doff * 4) - 6;

		/* Codes_SRS_FRAME_CODEC_01_015: [TYPE Byte 5 of the frame header is a type code.] */
		frame_codec->receive_frame_type = b;
		frame_codec->receive_frame_pos = 0;
		frame_codec->receive_frame_type_specific = (unsigned char*)amqpalloc_malloc(type_specific_size);
		if (frame_codec->receive_frame_type_specific == NULL)
		{
			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
			result = __LINE__;
			break;
		}
		else
		{
			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_TYPE_SPECIFIC;
			result = 0;
			break;
		}
	}

	case RECEIVE_FRAME_STATE_TYPE_SPECIFIC:
	{
		uint32_t type_specific_size = (frame_codec->receive_frame_doff * 4) - 6;

		frame_codec->receive_frame_type_specific[frame_codec->receive_frame_pos++] = b;
		if (frame_codec->receive_frame_pos == type_specific_size)
		{
			/* Codes_SRS_FRAME_CODEC_01_031: [When a frame is successfully decoded it shall be indicated to the upper layer by invoking the receive callback passed to frame_codec_create.] */
			/* Codes_SRS_FRAME_CODEC_01_032: [Besides passing the frame information, the frame_received_callback_context value passed to frame_codec_create shall be passed to the frame_received_callback function.] */
			frame_codec->frame_begin_callback(frame_codec->callback_context, frame_codec->receive_frame_size - frame_codec->receive_frame_doff * 4, frame_codec->receive_frame_type_specific, type_specific_size);
			amqpalloc_free(frame_codec->receive_frame_type_specific);
			frame_codec->receive_frame_type_specific = NULL;

			frame_codec->receive_frame_pos = 0;
			if (frame_codec->receive_frame_size == FRAME_HEADER_SIZE)
			{
				frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
				frame_codec->receive_frame_size = 0;
			}
			else
			{
				frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_BODY;
			}
		}

		result = 0;
		break;
	}

	case RECEIVE_FRAME_STATE_FRAME_BODY:
		frame_codec->frame_body_bytes_received_callback(frame_codec->callback_context, &b, 1);
		frame_codec->receive_frame_pos++;

		if (frame_codec->receive_frame_pos == frame_codec->receive_frame_size)
		{
			frame_codec->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			frame_codec->receive_frame_pos = 0;
			frame_codec->receive_frame_size = 0;
		}
		result = 0;

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
			result->frame_begin_callback = NULL;
			result->frame_body_bytes_received_callback = NULL;
			result->callback_context = NULL;
			result->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			result->receive_frame_pos = 0;
			result->receive_frame_size = 0;
			result->receive_frame_type_specific = NULL;
		}
	}

	return result;
}

void frame_codec_destroy(FRAME_CODEC_HANDLE frame_codec)
{
	/* Codes_SRS_FRAME_CODEC_01_024: [If frame_codec is NULL, frame_codec_destroy shall do nothing.] */
	if (frame_codec != NULL)
	{
		FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
		if (frame_codec_data->receive_frame_type_specific != NULL)
		{
			amqpalloc_free(frame_codec_data->receive_frame_type_specific);
		}

		/* Codes_SRS_FRAME_CODEC_01_023: [frame_codec_destroy shall free all resources associated with a frame_codec instance.] */
		amqpalloc_free(frame_codec);
	}
}

int frame_codec_set_max_frame_size(FRAME_CODEC_HANDLE frame_codec, uint32_t max_frame_size)
{
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	frame_codec_data->max_frame_size = max_frame_size;
	return 0;
}

int frame_codec_receive_bytes(FRAME_CODEC_HANDLE frame_codec, const void* buffer, size_t size)
{
	int result;
	size_t i;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	/* Codes_SRS_FRAME_CODEC_01_026: [If frame_codec or buffer are NULL, frame_codec_receive_bytes shall return a non-zero value.] */
	if ((frame_codec == NULL) ||
		(buffer == NULL) ||
		/* Codes_SRS_FRAME_CODEC_01_027: [If size is zero, frame_codec_receive_bytes shall return a non-zero value.] */
		(size == 0))
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

int frame_codec_subscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type, FRAME_BEGIN_CALLBACK frame_begin_callback, FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback, void* callback_context)
{
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	frame_codec_data->frame_begin_callback = frame_begin_callback;
	frame_codec_data->frame_body_bytes_received_callback = frame_body_bytes_received_callback;
	frame_codec_data->callback_context = callback_context;
	return 0;
}

int frame_codec_unsubscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type)
{
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	frame_codec_data->frame_begin_callback = NULL;
	frame_codec_data->frame_body_bytes_received_callback = NULL;
	frame_codec_data->callback_context = NULL;
	return 0;
}
