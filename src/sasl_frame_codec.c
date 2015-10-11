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
	SASL_FRAME_ENCODE_FRAME_HEADER,
	SASL_FRAME_ENCODE_FRAME_PAYLOAD,
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
	uint32_t encode_payload_bytes_left;
} SASL_FRAME_CODEC_INSTANCE;

static void amqp_value_decoded(void* context, AMQP_VALUE decoded_value)
{
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;
	uint64_t performative_descriptor_ulong;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(decoded_value);

	/* Codes_SRS_SASL_FRAME_CODEC_01_060: [If any error occurs while decoding a frame, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
	if ((descriptor == NULL) ||
		(amqpvalue_get_ulong(descriptor, &performative_descriptor_ulong) != 0) ||
		/* Codes_SRS_SASL_FRAME_CODEC_01_003: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the SASL type system.] */
		(performative_descriptor_ulong < SASL_MECHANISMS) ||
		(performative_descriptor_ulong > SASL_OUTCOME))
	{
		/* Codes_SRS_SASL_FRAME_CODEC_01_060: [If any error occurs while decoding a frame, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
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
		/* Codes_SRS_SASL_FRAME_CODEC_01_050: [All subsequent decoding shall fail and no SASL frames shall be indicated from that point on to the consumers of sasl_frame_codec.] */
	case SASL_FRAME_DECODE_ERROR:
		result = __LINE__;
		break;

	case SASL_FRAME_DECODE_FRAME:
		/* Codes_SRS_SASL_FRAME_CODEC_01_049: [If not enough type specific bytes are received to decode the channel number, the decoding shall stop with an error.] */
		if (type_specific_size < 2)
		{
			sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_001: [Bytes 6 and 7 of an SASL frame contain the channel number ] */
			channel = ((uint16_t)type_specific[0]) << 8;
			channel += type_specific[1];

			/* Codes_SRS_SASL_FRAME_CODEC_01_051: [If the frame payload is greater than 0, sasl_frame_codec shall decode the performative as a described SASL type.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_002: [The frame body is defined as a performative followed by an opaque payload.] */
			sasl_frame_codec_instance->decoded_performative = NULL;

			while ((frame_body_size > 0) &&
				(sasl_frame_codec_instance->decoded_performative == NULL) &&
				(sasl_frame_codec_instance->decode_state != SASL_FRAME_DECODE_ERROR))
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_052: [Decoding the performative shall be done by feeding the bytes to the decoder create in sasl_frame_codec_create.] */
				if (amqpvalue_decode_bytes(sasl_frame_codec_instance->decoder, frame_body, 1) != 0)
				{
					/* Codes_SRS_SASL_FRAME_CODEC_01_060: [If any error occurs while decoding a frame, the decoder shall switch to an error state where decoding shall not be possible anymore.] */
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
				/* Codes_SRS_SASL_FRAME_CODEC_01_004: [The remaining bytes in the frame body form the payload for that frame.] */
				/* Codes_SRS_SASL_FRAME_CODEC_01_067: [When the performative is decoded, the rest of the frame_bytes shall not be given to the SASL decoder, but they shall be buffered so that later they are given to the frame_received callback.] */
				/* Codes_SRS_SASL_FRAME_CODEC_01_054: [Once the performative is decoded and all frame payload bytes are received, the callback frame_received_callback shall be called.] */
				/* Codes_SRS_SASL_FRAME_CODEC_01_068: [A pointer to all the payload bytes shall also be passed to frame_received_callback.] */
				sasl_frame_codec_instance->frame_received_callback(sasl_frame_codec_instance->callback_context, sasl_frame_codec_instance->decoded_performative);
				result = 0;
			}
		}
		break;
	}

	return result;
}

/* Codes_SRS_SASL_FRAME_CODEC_01_011: [sasl_frame_codec_create shall create an instance of an sasl_frame_codec and return a non-NULL handle to it.] */
SASL_FRAME_CODEC_HANDLE sasl_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, SASL_FRAME_RECEIVED_CALLBACK frame_received_callback, void* frame_received_callback_context)
{
	SASL_FRAME_CODEC_INSTANCE* result;

	/* Codes_SRS_SASL_FRAME_CODEC_01_012: [If any of the arguments frame_codec, frame_received_callback or empty_frame_received_callback is NULL, sasl_frame_codec_create shall return NULL.] */
	if ((frame_codec == NULL) ||
		(frame_received_callback == NULL))
	{
		result = NULL;
	}
	else
	{
		result = (SASL_FRAME_CODEC_INSTANCE*)amqpalloc_malloc(sizeof(SASL_FRAME_CODEC_INSTANCE));
		/* Codes_SRS_SASL_FRAME_CODEC_01_020: [If allocating memory for the new sasl_frame_codec fails, then sasl_frame_codec_create shall fail and return NULL.] */
		if (result != NULL)
		{
			result->frame_codec = frame_codec;
			result->frame_received_callback = frame_received_callback;
			result->callback_context = frame_received_callback_context;
			result->decode_state = SASL_FRAME_DECODE_FRAME;
			result->encode_state = SASL_FRAME_ENCODE_FRAME_HEADER;

			/* Codes_SRS_SASL_FRAME_CODEC_01_018: [sasl_frame_codec_create shall create a decoder to be used for decoding SASL values.] */
			result->decoder = amqpvalue_decoder_create(amqp_value_decoded, result);
			if (result->decoder == NULL)
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_019: [If creating the decoder fails, sasl_frame_codec_create shall fail and return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_013: [sasl_frame_codec_create shall subscribe for SASL frames with the given frame_codec.] */
				if (frame_codec_subscribe(frame_codec, 0, frame_received, result) != 0)
				{
					/* Codes_SRS_SASL_FRAME_CODEC_01_014: [If subscribing for SASL frames fails, sasl_frame_codec_create shall fail and return NULL.] */
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

		/* Codes_SRS_SASL_FRAME_CODEC_01_017: [sasl_frame_codec_destroy shall unsubscribe from receiving SASL frames from the frame_codec that was passed to sasl_frame_codec_create.] */
		(void)frame_codec_unsubscribe(sasl_frame_codec_instance->frame_codec, FRAME_TYPE_SASL);

		/* Codes_SRS_SASL_FRAME_CODEC_01_021: [The decoder created in sasl_frame_codec_create shall be destroyed by sasl_frame_codec_destroy.] */
		amqpvalue_decoder_destroy(sasl_frame_codec_instance->decoder);

		/* Codes_SRS_SASL_FRAME_CODEC_01_015: [sasl_frame_codec_destroy shall free all resources associated with the sasl_frame_codec instance.] */
		amqpalloc_free(sasl_frame_codec_instance);
	}
}

int sasl_frame_codec_encode_frame(SASL_FRAME_CODEC_HANDLE sasl_frame_codec, const AMQP_VALUE performative)
{
	int result;
	uint32_t amqp_frame_payload_size;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)sasl_frame_codec;

	/* Codes_SRS_SASL_FRAME_CODEC_01_024: [If frame_codec or performative_fields is NULL, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
	if ((sasl_frame_codec == NULL) ||
		(performative == NULL) ||
		/* Codes_SRS_SASL_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.]  */
		(sasl_frame_codec_instance->encode_state != SASL_FRAME_ENCODE_FRAME_HEADER))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE descriptor;
		uint64_t performative_ulong;

		if (((descriptor = amqpvalue_get_inplace_descriptor(performative)) == NULL) ||
			(amqpvalue_get_ulong(descriptor, &performative_ulong) != 0) ||
			/* Codes_SRS_SASL_FRAME_CODEC_01_008: [The performative MUST be one of those defined in section 2.7 and is encoded as a described type in the SASL type system.] */
			(performative_ulong < SASL_MECHANISMS) ||
			(performative_ulong > SASL_OUTCOME))
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		/* Codes_SRS_SASL_FRAME_CODEC_01_027: [The encoded size of the performative and its fields shall be obtained by calling amqpvalue_get_encoded_size.] */
		else if (amqpvalue_get_encoded_size(performative, &amqp_frame_payload_size) != 0)
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_005: [Bytes 6 and 7 of an SASL frame contain the channel number ] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_025: [sasl_frame_codec_begin_encode_frame shall encode the frame header by using frame_codec_begin_encode_frame.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_006: [The frame body is defined as a performative followed by an opaque payload.] */
			if (frame_codec_begin_encode_frame(sasl_frame_codec_instance->frame_codec, FRAME_TYPE_SASL, amqp_frame_payload_size, NULL, 0) != 0)
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_030: [Encoding of the SASL performative and its fields shall be done by calling amqpvalue_encode.] */
				/* Codes_SRS_SASL_FRAME_CODEC_01_028: [The encode result for the performative and its fields shall be given to frame_codec by calling frame_codec_encode_frame_bytes.] */
				if (amqpvalue_encode(performative, frame_codec_encode_frame_bytes, sasl_frame_codec_instance->frame_codec) != 0)
				{
					/* Codes_SRS_SASL_FRAME_CODEC_01_029: [If any error occurs during encoding, sasl_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
					result = __LINE__;
				}
				else
				{
					sasl_frame_codec_instance->encode_state = SASL_FRAME_ENCODE_FRAME_HEADER;

					/* Codes_SRS_SASL_FRAME_CODEC_01_022: [sasl_frame_codec_begin_encode_frame shall encode the frame header and SASL performative in an SASL frame and on success it shall return 0.] */
					result = 0;
				}
			}
		}
	}

	return result;
}
