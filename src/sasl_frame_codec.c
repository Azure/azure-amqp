#include "sasl_frame_codec.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

typedef enum SASL_FRAME_DECODE_STATE_TAG
{
	SASL_FRAME_DECODE_FRAME_HEADER,
	SASL_FRAME_DECODE_FRAME,
	SASL_FRAME_DECODE_ERROR
} SASL_FRAME_DECODE_STATE;

typedef struct SASL_FRAME_CODEC_INSTANCE_TAG
{
	FRAME_CODEC_HANDLE frame_codec;

	/* decode */
	SASL_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* callback_context;
	uint32_t decode_frame_body_size;
	uint32_t decode_frame_body_pos;
	uint16_t channel;
	AMQPVALUE_DECODER_HANDLE decoder;
	SASL_FRAME_DECODE_STATE decode_state;
} SASL_FRAME_CODEC_INSTANCE;

static void amqp_value_decoded(void* context, AMQP_VALUE decoded_value)
{
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;
	uint64_t performative_descriptor_ulong;

	switch (sasl_frame_codec_instance->decode_state)
	{
	default:
		sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
		break;

	case SASL_FRAME_DECODE_FRAME:
	{
		AMQP_VALUE descriptor = amqpvalue_get_descriptor(decoded_value);

		if ((descriptor == NULL) ||
			(amqpvalue_get_ulong(descriptor, &performative_descriptor_ulong) != 0) ||
			(performative_descriptor_ulong < SASL_MECHANISMS) ||
			(performative_descriptor_ulong > SASL_OUTCOME))
		{
			sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
		}
		else
		{
			sasl_frame_codec_instance->frame_received_callback(sasl_frame_codec_instance->callback_context, decoded_value);

			if (sasl_frame_codec_instance->decode_frame_body_size > sasl_frame_codec_instance->decode_frame_body_pos)
			{
				sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
			}
			else
			{
				sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_FRAME_HEADER;
			}
		}
		break;
	}
	}
}

static int frame_begin(void* context, uint32_t decode_frame_body_size, const unsigned char* type_specific, uint32_t type_specific_size)
{
	int result;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;

	switch (sasl_frame_codec_instance->decode_state)
	{
	default:
	case SASL_FRAME_DECODE_ERROR:
	case SASL_FRAME_DECODE_FRAME:
		result = __LINE__;
		break;

	case SASL_FRAME_DECODE_FRAME_HEADER:
		sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_FRAME;
		sasl_frame_codec_instance->decode_frame_body_size = decode_frame_body_size;
		result = 0;
		break;
	}

	return result;
}

static int frame_body_bytes_received(void* context, const unsigned char* frame_body_bytes, uint32_t frame_body_bytes_size)
{
	int result;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;

	if (sasl_frame_codec_instance->decode_frame_body_size - sasl_frame_codec_instance->decode_frame_body_pos < frame_body_bytes_size)
	{
		result = __LINE__;
	}
	else
	{
		while (frame_body_bytes_size > 0)
		{
			switch (sasl_frame_codec_instance->decode_state)
			{
			default:
			case SASL_FRAME_DECODE_ERROR:
				result = __LINE__;
				break;

			case SASL_FRAME_DECODE_FRAME_HEADER:
				sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
				result = __LINE__;
				break;

			case SASL_FRAME_DECODE_FRAME:
				sasl_frame_codec_instance->decode_frame_body_pos++;
				if ((amqpvalue_decode_bytes(sasl_frame_codec_instance->decoder, frame_body_bytes, 1) != 0) ||
					(sasl_frame_codec_instance->decode_state == SASL_FRAME_DECODE_ERROR))
				{
					result = __LINE__;
				}
				else
				{
					frame_body_bytes_size--;
					frame_body_bytes++;
					result = 0;
				}
				break;
			}

			if (result != 0)
			{
				break;
			}
		}
	}

	return result;
}

SASL_FRAME_CODEC_HANDLE sasl_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, SASL_FRAME_RECEIVED_CALLBACK frame_received_callback,
	void* frame_received_callback_context)
{
	SASL_FRAME_CODEC_INSTANCE* result;

	if ((frame_codec == NULL) ||
		(frame_received_callback == NULL))
	{
		result = NULL;
	}
	else
	{
		result = (SASL_FRAME_CODEC_INSTANCE*)amqpalloc_malloc(sizeof(SASL_FRAME_CODEC_INSTANCE));
		if (result != NULL)
		{
			result->frame_codec = frame_codec;
			result->frame_received_callback = frame_received_callback;
			result->callback_context = frame_received_callback_context;
			result->decode_state = SASL_FRAME_DECODE_FRAME_HEADER;

			result->decoder = amqpvalue_decoder_create(amqp_value_decoded, result);
			if (result->decoder == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (frame_codec_subscribe(frame_codec, 1, frame_begin, frame_body_bytes_received, result) != 0)
				{
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
}

int sasl_frame_codec_encode_frame(SASL_FRAME_CODEC_HANDLE sasl_frame_codec, const AMQP_VALUE sasl_frame)
{
	int result;
	uint32_t amqp_frame_payload_size;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)sasl_frame_codec;

	if ((sasl_frame_codec == NULL) ||
		(sasl_frame == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE descriptor;
		uint64_t performative_ulong;

		if (((descriptor = amqpvalue_get_descriptor(sasl_frame)) == NULL) ||
			(amqpvalue_get_ulong(descriptor, &performative_ulong) != 0) ||
			(performative_ulong < SASL_MECHANISMS) ||
			(performative_ulong > SASL_OUTCOME))
		{
			result = __LINE__;
		}
		else if (amqpvalue_get_encoded_size(sasl_frame, &amqp_frame_payload_size) != 0)
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
				if (amqpvalue_encode(sasl_frame, frame_codec_encode_frame_bytes, sasl_frame_codec_instance->frame_codec) != 0)
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}
			}
		}
	}

	return result;
}
