#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "amqp_frame_codec.h"
#include "frame_codec.h"
#include "encoder.h"
#include "amqpalloc.h"
#include "amqpvalue.h"

typedef enum AMQP_FRAME_DECODE_STATE_TAG
{
	AMQP_FRAME_DECODE_DESCRIPTOR,
	AMQP_FRAME_DECODE_FRAME_LIST,
	AMQP_FRAME_DECODE_ERROR
} AMQP_FRAME_DECODE_STATE;

typedef struct AMQP_FRAME_CODEC_DATA_TAG
{
	FRAME_CODEC_HANDLE frame_codec_handle;
	AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback;
	AMQP_EMPTY_FRAME_RECEIVED_CALLBACK empty_frame_received_callback;
	void* frame_received_callback_context;
	uint32_t frame_body_size;
	uint32_t frame_body_pos;
	uint8_t channel;
	DECODER_HANDLE decoder;
	AMQP_FRAME_DECODE_STATE decode_state;
	uint64_t performative;
} AMQP_FRAME_CODEC_DATA;

static void amqp_value_decoded(void* context, AMQP_VALUE decoded_value)
{
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec = (AMQP_FRAME_CODEC_DATA*)context;

	switch (amqp_frame_codec->decode_state)
	{
	default:
		break;
	case AMQP_FRAME_DECODE_DESCRIPTOR:
		if (amqpvalue_get_ulong(amqpvalue_get_descriptor(decoded_value), &amqp_frame_codec->performative) != 0)
		{
			amqp_frame_codec->decode_state = AMQP_FRAME_DECODE_ERROR;
		}
		else
		{
			amqp_frame_codec->decode_state = AMQP_FRAME_DECODE_FRAME_LIST;
		}
		break;
	case AMQP_FRAME_DECODE_FRAME_LIST:
		if (amqp_frame_codec->frame_received_callback != NULL)
		{
			amqp_frame_codec->frame_received_callback(amqp_frame_codec->frame_received_callback_context, 0, amqp_frame_codec->performative, decoded_value, 0);
			amqp_frame_codec->decode_state = AMQP_FRAME_DECODE_DESCRIPTOR;
		}
		break;
	}
}

static void frame_begin(void* context, uint32_t frame_body_size, const unsigned char* type_specific, uint32_t type_specific_size)
{
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec = (AMQP_FRAME_CODEC_DATA*)context;
	amqp_frame_codec->frame_body_size = frame_body_size;
	amqp_frame_codec->frame_body_pos = 0;
	amqp_frame_codec->channel = type_specific[0] << 8;
	amqp_frame_codec->channel += type_specific[1];
}

static void frame_body_bytes_received(void* context, const unsigned char* frame_body_bytes, uint32_t frame_body_bytes_size)
{
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec = (AMQP_FRAME_CODEC_DATA*)context;
	decoder_decode_bytes(amqp_frame_codec->decoder, frame_body_bytes, frame_body_bytes_size);
}

AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback,
	AMQP_EMPTY_FRAME_RECEIVED_CALLBACK empty_frame_received_callback, AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK payload_bytes_received_callback,
	void* frame_received_callback_context)
{
	AMQP_FRAME_CODEC_DATA* result = (AMQP_FRAME_CODEC_DATA*)amqpalloc_malloc(sizeof(AMQP_FRAME_CODEC_DATA));
	if (result != NULL)
	{
		result->frame_codec_handle = frame_codec;
		result->frame_received_callback = frame_received_callback;
		result->empty_frame_received_callback = empty_frame_received_callback;
		result->frame_received_callback_context = frame_received_callback_context;
		result->decoder = decoder_create(amqp_value_decoded, result);
		result->decode_state = AMQP_FRAME_DECODE_DESCRIPTOR;

		frame_codec_subscribe(frame_codec, 0, frame_begin, frame_body_bytes_received, result);
	}

	return result;
}

void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec)
{
	if (amqp_frame_codec != NULL)
	{
		AMQP_FRAME_CODEC_DATA* amqp_frame_codec_instance = (AMQP_FRAME_CODEC_DATA*)amqp_frame_codec;
		decoder_destroy(amqp_frame_codec_instance->decoder);
		amqpalloc_free(amqp_frame_codec_instance);
	}
}

int amqp_frame_codec_begin_encode_frame(FRAME_CODEC_HANDLE frame_codec, uint16_t channel, uint64_t performative, const AMQP_VALUE performative_fields, uint32_t payload_size)
{
	int result;
	ENCODER_HANDLE encoder_handle = encoder_create(NULL, NULL);
	uint32_t amqp_frame_payload_size;
	AMQP_VALUE ulong_descriptor_value = amqpvalue_create_ulong(performative);
	AMQP_VALUE descriptor = amqpvalue_create_descriptor(ulong_descriptor_value);

	if (encoder_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if (encoder_encode_amqp_value(encoder_handle, descriptor) != 0)
		{
			result = __LINE__;
		}
		else
		{
			if ((encoder_encode_amqp_value(encoder_handle, performative_fields) != 0) ||
				(encoder_get_encoded_size(encoder_handle, &amqp_frame_payload_size) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}

		encoder_destroy(encoder_handle);
	}

	if (result == 0)
	{
		amqp_frame_payload_size += payload_size;
		if (frame_codec_begin_encode_frame(frame_codec, FRAME_TYPE_AMQP, amqp_frame_payload_size, NULL, amqp_frame_payload_size) != 0)
		{
			result = __LINE__;
		}
		else
		{
			encoder_handle = encoder_create(frame_codec_encode_frame_bytes, frame_codec);
			if (encoder_handle == NULL)
			{
				result = __LINE__;
			}
			else
			{
				if (encoder_encode_amqp_value(encoder_handle, descriptor) != 0)
				{
					result = __LINE__;
				}
				else
				{
					if (encoder_encode_amqp_value(encoder_handle, performative_fields) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}

				encoder_destroy(encoder_handle);
			}
		}
	}

	return result;
}

extern int amqp_frame_codec_encode_payload_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, uint32_t count)
{
	return 0;
}
