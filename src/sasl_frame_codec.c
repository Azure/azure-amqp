#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "sasl_frame_codec.h"
#include "frame_codec.h"
#include "amqpalloc.h"
#include "amqpvalue.h"

typedef enum SASL_FRAME_DECODE_STATE_TAG
{
	SASL_FRAME_DECODE_FRAME,
	SASL_FRAME_DECODE_ERROR
} SASL_FRAME_DECODE_STATE;

typedef enum SASL_FRAME_ENCODE_STATE_TAG
{
	SASL_FRAME_ENCODE_FRAME,
	SASL_FRAME_ENCODE_ERROR
} SASL_FRAME_ENCODE_STATE;

typedef struct SASL_FRAME_CODEC_INSTANCE_TAG
{
	FRAME_CODEC_HANDLE frame_codec;

	/* decode */
	SASL_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* callback_context;
	AMQPVALUE_DECODER_HANDLE decoder;
	SASL_FRAME_DECODE_STATE decode_state;
	AMQP_VALUE decoded_performative;

	/* encode */
	SASL_FRAME_ENCODE_STATE encode_state;
} SASL_FRAME_CODEC_INSTANCE;

static void amqp_value_decoded(void* context, AMQP_VALUE decoded_value)
{
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;
	uint64_t performative_descriptor_ulong;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(decoded_value);

	if ((descriptor == NULL) ||
		(amqpvalue_get_ulong(descriptor, &performative_descriptor_ulong) != 0) ||
		(performative_descriptor_ulong < SASL_MECHANISMS) ||
		(performative_descriptor_ulong > SASL_OUTCOME))
	{
		sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
	}
	else
	{
		sasl_frame_codec_instance->decoded_performative = decoded_value;
	}
}

static int frame_received(void* context, const unsigned char* type_specific, uint32_t type_specific_size, const unsigned char* frame_body, uint32_t frame_body_size)
{
	int result;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;
	uint16_t channel;

	switch (sasl_frame_codec_instance->decode_state)
	{
	default:
	case SASL_FRAME_DECODE_ERROR:
		result = __LINE__;
		break;

	case SASL_FRAME_DECODE_FRAME:
		if (type_specific_size < 2)
		{
			sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
			result = __LINE__;
		}
		else
		{
			channel = ((uint16_t)type_specific[0]) << 8;
			channel += type_specific[1];

			sasl_frame_codec_instance->decoded_performative = NULL;

			while ((frame_body_size > 0) &&
				(sasl_frame_codec_instance->decoded_performative == NULL) &&
				(sasl_frame_codec_instance->decode_state != SASL_FRAME_DECODE_ERROR))
			{
				if (amqpvalue_decode_bytes(sasl_frame_codec_instance->decoder, frame_body, 1) != 0)
				{
					sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
				}
				else
				{
					frame_body_size--;
					frame_body++;
				}
			}

			if (sasl_frame_codec_instance->decode_state == SASL_FRAME_DECODE_ERROR)
			{
				result = __LINE__;
			}
			else
			{
				sasl_frame_codec_instance->frame_received_callback(sasl_frame_codec_instance->callback_context, sasl_frame_codec_instance->decoded_performative);
				result = 0;
			}
		}
		break;
	}

	return result;
}

SASL_FRAME_CODEC_HANDLE sasl_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, SASL_FRAME_RECEIVED_CALLBACK frame_received_callback, void* frame_received_callback_context)
{
	SASL_FRAME_CODEC_INSTANCE* result;

	/* Codes_SRS_SASL_FRAME_CODEC_01_019: [If any of the arguments frame_codec or frame_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
	if ((frame_codec == NULL) ||
		(frame_received_callback == NULL))
	{
		result = NULL;
	}
	else
	{
		/* Codes_SRS_SASL_FRAME_CODEC_01_018: [sasl_frame_codec_create shall create an instance of an sasl_frame_codec and return a non-NULL handle to it.] */
		result = (SASL_FRAME_CODEC_INSTANCE*)amqpalloc_malloc(sizeof(SASL_FRAME_CODEC_INSTANCE));
		if (result != NULL)
		{
			result->frame_codec = frame_codec;
			result->frame_received_callback = frame_received_callback;
			result->callback_context = frame_received_callback_context;
			result->decode_state = SASL_FRAME_DECODE_FRAME;
			result->encode_state = SASL_FRAME_ENCODE_FRAME;

			/* Codes_SRS_SASL_FRAME_CODEC_01_022: [amqp_frame_codec_create shall create a decoder to be used for decoding SASL values.] */
			result->decoder = amqpvalue_decoder_create(amqp_value_decoded, result);
			if (result->decoder == NULL)
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_023: [If creating the decoder fails, sasl_frame_codec_create shall fail and return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_020: [sasl_frame_codec_create shall subscribe for SASL frames with the given frame_codec.] */
				if (frame_codec_subscribe(frame_codec, FRAME_TYPE_SASL, frame_received, result) != 0)
				{
					/* Codes_SRS_SASL_FRAME_CODEC_01_021: [If subscribing for SASL frames fails, sasl_frame_codec_create shall fail and return NULL.] */
					amqpvalue_decoder_destroy(result->decoder);
					amqpalloc_free(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}

void sasl_frame_codec_destroy(SASL_FRAME_CODEC_HANDLE sasl_frame_codec)
{
	if (sasl_frame_codec != NULL)
	{
		SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)sasl_frame_codec;
		(void)frame_codec_unsubscribe(sasl_frame_codec_instance->frame_codec, FRAME_TYPE_SASL);
		amqpvalue_decoder_destroy(sasl_frame_codec_instance->decoder);
		amqpalloc_free(sasl_frame_codec_instance);
	}
}

int sasl_frame_codec_encode_frame(SASL_FRAME_CODEC_HANDLE sasl_frame_codec, const AMQP_VALUE performative)
{
	int result;
	uint32_t amqp_frame_payload_size;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)sasl_frame_codec;

	if ((sasl_frame_codec == NULL) ||
		(performative == NULL) ||
		(sasl_frame_codec_instance->encode_state != SASL_FRAME_ENCODE_FRAME))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE descriptor;
		uint64_t performative_ulong;

		if (((descriptor = amqpvalue_get_inplace_descriptor(performative)) == NULL) ||
			(amqpvalue_get_ulong(descriptor, &performative_ulong) != 0) ||
			(performative_ulong < SASL_MECHANISMS) ||
			(performative_ulong > SASL_OUTCOME))
		{
			result = __LINE__;
		}
		else if (amqpvalue_get_encoded_size(performative, &amqp_frame_payload_size) != 0)
		{
			result = __LINE__;
		}
		else
		{
			if (frame_codec_begin_encode_frame(sasl_frame_codec_instance->frame_codec, FRAME_TYPE_SASL, amqp_frame_payload_size, NULL, 0) != 0)
			{
				result = __LINE__;
			}
			else
			{
				if (amqpvalue_encode(performative, frame_codec_encode_frame_bytes, sasl_frame_codec_instance->frame_codec) != 0)
				{
					result = __LINE__;
				}
				else
				{
					sasl_frame_codec_instance->encode_state = SASL_FRAME_ENCODE_FRAME;

					result = 0;
				}
			}
		}
	}

	return result;
}
