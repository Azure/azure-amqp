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
	FRAME_CODEC_HANDLE frame_codec;
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

/* Codes_SRS_AMQP_FRAME_CODEC_01_011: [amqp_frame_codec_create shall create an instance of an amqp_frame_codec and return a non-NULL handle to it.] */
AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback,
	AMQP_EMPTY_FRAME_RECEIVED_CALLBACK empty_frame_received_callback, AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK payload_bytes_received_callback,
	void* frame_received_callback_context)
{
	AMQP_FRAME_CODEC_DATA* result;

	/* Codes_SRS_AMQP_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback, empty_frame_received_callback or payload_bytes_received_callback is NULL, amqp_frame_codec_create shall return NULL.] */
	if ((frame_codec == NULL) ||
		(frame_received_callback == NULL) ||
		(empty_frame_received_callback == NULL) ||
		(payload_bytes_received_callback == NULL))
	{
		result = NULL;
	}
	else
	{
		result = (AMQP_FRAME_CODEC_DATA*)amqpalloc_malloc(sizeof(AMQP_FRAME_CODEC_DATA));
		/* Codes_SRS_AMQP_FRAME_CODEC_01_020: [If allocating memory for the new amqp_frame_codec fails, then amqp_frame_codec_create shall fail and return NULL.] */
		if (result != NULL)
		{
			result->frame_codec = frame_codec;
			result->frame_received_callback = frame_received_callback;
			result->empty_frame_received_callback = empty_frame_received_callback;
			result->frame_received_callback_context = frame_received_callback_context;
			result->decode_state = AMQP_FRAME_DECODE_DESCRIPTOR;

			/* Codes_SRS_AMQP_FRAME_CODEC_01_018: [amqp_frame_codec_create shall create a decoder to be used for decoding AMQP values.] */
			result->decoder = decoder_create(amqp_value_decoded, result);
			if (result->decoder == NULL)
			{
				/* Codes_SRS_AMQP_FRAME_CODEC_01_019: [If creating the decoder fails, amqp_frame_codec_create shall fail and return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				/* Codes_SRS_AMQP_FRAME_CODEC_01_013: [amqp_frame_codec_create shall subscribe for AMQP frames with the given frame_codec.] */
				if (frame_codec_subscribe(frame_codec, 0, frame_begin, frame_body_bytes_received, result) != 0)
				{
					/* Codes_SRS_AMQP_FRAME_CODEC_01_014: [If subscribing for AMQP frames fails, amqp_frame_codec_create shall fail and return NULL.] */
					decoder_destroy(result->decoder);
					amqpalloc_free(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}

void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec)
{
	if (amqp_frame_codec != NULL)
	{
		AMQP_FRAME_CODEC_DATA* amqp_frame_codec_instance = (AMQP_FRAME_CODEC_DATA*)amqp_frame_codec;

		/* Codes_SRS_AMQP_FRAME_CODEC_01_017: [amqp_frame_codec_destroy shall unsubscribe from receiving AMQP frames from the frame_codec that was passed to amqp_frame_codec_create.] */
		(void)frame_codec_unsubscribe(amqp_frame_codec_instance->frame_codec, FRAME_TYPE_AMQP);

		/* Codes_SRS_AMQP_FRAME_CODEC_01_021: [The decoder created in amqp_frame_codec_create shall be destroyed by amqp_frame_codec_destroy.] */
		decoder_destroy(amqp_frame_codec_instance->decoder);

		/* Codes_SRS_AMQP_FRAME_CODEC_01_015: [amqp_frame_codec_destroy shall free all resources associated with the amqp_frame_codec instance.] */
		amqpalloc_free(amqp_frame_codec_instance);
	}
}

int amqp_frame_codec_begin_encode_frame(FRAME_CODEC_HANDLE frame_codec, uint16_t channel, const AMQP_VALUE performative, uint32_t payload_size)
{
	int result;
	ENCODER_HANDLE encoder_handle = encoder_create(NULL, NULL);
	uint32_t amqp_frame_payload_size;

	if (encoder_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if ((encoder_encode_amqp_value(encoder_handle, performative) != 0) ||
			(encoder_get_encoded_size(encoder_handle, &amqp_frame_payload_size) != 0))
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
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
				if (encoder_encode_amqp_value(encoder_handle, performative) != 0)
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
	}

	return result;
}

extern int amqp_frame_codec_encode_payload_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, uint32_t count)
{
	return 0;
}
