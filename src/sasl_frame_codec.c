#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "sasl_frame_codec.h"
#include "frame_codec.h"
#include "amqpalloc.h"
#include "amqpvalue.h"
#include "amqp_definitions.h"

/* Requirements implemented by design or by other modules */
/* Codes_SRS_SASL_FRAME_CODEC_01_011: [A SASL frame has a type code of 0x01.] */
/* Codes_SRS_SASL_FRAME_CODEC_01_016: [The maximum size of a SASL frame is defined by MIN-MAX-FRAME-SIZE.] */

#define MIX_MAX_FRAME_SIZE 512

typedef enum SASL_FRAME_DECODE_STATE_TAG
{
	SASL_FRAME_DECODE_FRAME,
	SASL_FRAME_DECODE_ERROR
} SASL_FRAME_DECODE_STATE;

typedef struct SASL_FRAME_CODEC_INSTANCE_TAG
{
	FRAME_CODEC_HANDLE frame_codec;

	/* decode */
	SASL_FRAME_RECEIVED_CALLBACK frame_received_callback;
	void* callback_context;
	AMQPVALUE_DECODER_HANDLE decoder;
	SASL_FRAME_DECODE_STATE decode_state;
	AMQP_VALUE decoded_sasl_frame_value;
} SASL_FRAME_CODEC_INSTANCE;

static void amqp_value_decoded(void* context, AMQP_VALUE decoded_value)
{
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;
	AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(decoded_value);

	if (descriptor == NULL)
	{
		sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
	}
	else
	{
		/* Codes_SRS_SASL_FRAME_CODEC_01_009: [The frame body of a SASL frame MUST contain exactly one AMQP type, whose type encoding MUST have provides=“sasl-frame”.] */
		if (!is_sasl_mechanisms_type_by_descriptor(descriptor) &&
			!is_sasl_init_type_by_descriptor(descriptor) &&
			!is_sasl_challenge_type_by_descriptor(descriptor) &&
			!is_sasl_response_type_by_descriptor(descriptor) &&
			!is_sasl_outcome_type_by_descriptor(descriptor))
		{
			sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
		}
		else
		{
			sasl_frame_codec_instance->decoded_sasl_frame_value = decoded_value;
		}
	}
}

static int frame_received(void* context, const unsigned char* type_specific, uint32_t type_specific_size, const unsigned char* frame_body, uint32_t frame_body_size)
{
	int result;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)context;

	/* Codes_SRS_SASL_FRAME_CODEC_01_006: [Bytes 6 and 7 of the header are ignored.] */
	(void)type_specific;
	/* Codes_SRS_SASL_FRAME_CODEC_01_007: [The extended header is ignored.] */

	/* Codes_SRS_SASL_FRAME_CODEC_01_008: [The maximum size of a SASL frame is defined by MIN-MAX-FRAME-SIZE.] */
	if ((type_specific_size + frame_body_size + 6 > MIX_MAX_FRAME_SIZE) ||
		/* Codes_SRS_SASL_FRAME_CODEC_01_010: [Receipt of an empty frame is an irrecoverable error.] */
		(frame_body_size == 0))
	{
		result = __LINE__;
	}
	else
	{
		switch (sasl_frame_codec_instance->decode_state)
		{
		default:
		case SASL_FRAME_DECODE_ERROR:
			result = __LINE__;
			break;

		case SASL_FRAME_DECODE_FRAME:
			sasl_frame_codec_instance->decoded_sasl_frame_value = NULL;

			/* Codes_SRS_SASL_FRAME_CODEC_01_039: [sasl_frame_codec shall decode the sasl-frame value as a described type.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_048: [Receipt of an empty frame is an irrecoverable error.] */
			while ((frame_body_size > 0) &&
				(sasl_frame_codec_instance->decoded_sasl_frame_value == NULL) &&
				(sasl_frame_codec_instance->decode_state != SASL_FRAME_DECODE_ERROR))
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_040: [Decoding the sasl-frame type shall be done by feeding the bytes to the decoder create in sasl_frame_codec_create.] */
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

			/* Codes_SRS_SASL_FRAME_CODEC_01_009: [The frame body of a SASL frame MUST contain exactly one AMQP type, whose type encoding MUST have provides=“sasl-frame”.] */
			if (frame_body_size > 0)
			{
				sasl_frame_codec_instance->decode_state = SASL_FRAME_DECODE_ERROR;
			}

			if (sasl_frame_codec_instance->decode_state == SASL_FRAME_DECODE_ERROR)
			{
				result = __LINE__;
			}
			else
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_041: [Once the sasl frame is decoded, the callback frame_received_callback shall be called.] */
				/* Codes_SRS_SASL_FRAME_CODEC_01_042: [The decoded sasl-frame value and the context passed in sasl_frame_codec_create shall be passed to frame_received_callback.] */
				sasl_frame_codec_instance->frame_received_callback(sasl_frame_codec_instance->callback_context, sasl_frame_codec_instance->decoded_sasl_frame_value);
				result = 0;
			}
			break;
		}
	}

	return result;
}

SASL_FRAME_CODEC_HANDLE sasl_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, SASL_FRAME_RECEIVED_CALLBACK frame_received_callback, SASL_FRAME_CODEC_ERROR_CALLBACK error_callback, void* callback_context)
{
	SASL_FRAME_CODEC_INSTANCE* result;

	/* Codes_SRS_SASL_FRAME_CODEC_01_019: [If any of the arguments frame_codec, frame_received_callback or error_callback is NULL, sasl_frame_codec_create shall return NULL.] */
	if ((frame_codec == NULL) ||
		(frame_received_callback == NULL) ||
		(error_callback == NULL))
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
			result->callback_context = callback_context;
			result->decode_state = SASL_FRAME_DECODE_FRAME;

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
				/* Codes_SRS_SASL_FRAME_CODEC_01_001: [A SASL frame has a type code of 0x01.] */
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
	/* Codes_SRS_SASL_FRAME_CODEC_01_026: [If sasl_frame_codec is NULL, sasl_frame_codec_destroy shall do nothing.] */
	if (sasl_frame_codec != NULL)
	{
		/* Codes_SRS_SASL_FRAME_CODEC_01_025: [sasl_frame_codec_destroy shall free all resources associated with the sasl_frame_codec instance.] */
		SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)sasl_frame_codec;

		/* Codes_SRS_SASL_FRAME_CODEC_01_027: [sasl_frame_codec_destroy shall unsubscribe from receiving SASL frames from the frame_codec that was passed to sasl_frame_codec_create.] */
		(void)frame_codec_unsubscribe(sasl_frame_codec_instance->frame_codec, FRAME_TYPE_SASL);

		/* Codes_SRS_SASL_FRAME_CODEC_01_028: [The decoder created in sasl_frame_codec_create shall be destroyed by sasl_frame_codec_destroy.] */
		amqpvalue_decoder_destroy(sasl_frame_codec_instance->decoder);
		amqpalloc_free(sasl_frame_codec_instance);
	}
}

/* Codes_SRS_SASL_FRAME_CODEC_01_029: [sasl_frame_codec_encode_frame shall encode the frame header and AMQP value in a SASL frame and on success it shall return 0.] */
int sasl_frame_codec_encode_frame(SASL_FRAME_CODEC_HANDLE sasl_frame_codec, const AMQP_VALUE performative)
{
	int result;
	uint32_t amqp_frame_payload_size;
	SASL_FRAME_CODEC_INSTANCE* sasl_frame_codec_instance = (SASL_FRAME_CODEC_INSTANCE*)sasl_frame_codec;

	if ((sasl_frame_codec == NULL) ||
		(performative == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE descriptor;
		uint64_t sasl_frame_descriptor_ulong;

		if (((descriptor = amqpvalue_get_inplace_descriptor(performative)) == NULL) ||
			(amqpvalue_get_ulong(descriptor, &sasl_frame_descriptor_ulong) != 0) ||
			/* Codes_SRS_SASL_FRAME_CODEC_01_047: [The frame body of a SASL frame MUST contain exactly one AMQP type, whose type encoding MUST have provides=“sasl-frame”.] */
			(sasl_frame_descriptor_ulong < SASL_MECHANISMS) ||
			(sasl_frame_descriptor_ulong > SASL_OUTCOME))
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		/* Codes_SRS_SASL_FRAME_CODEC_01_032: [The payload frame size shall be computed based on the encoded size of the sasl_frame and its fields.] */
		/* Codes_SRS_SASL_FRAME_CODEC_01_033: [The encoded size of the sasl_frame and its fields shall be obtained by calling amqpvalue_get_encoded_size.] */
		else if ((amqpvalue_get_encoded_size(performative, &amqp_frame_payload_size) != 0) ||
			/* Codes_SRS_SASL_FRAME_CODEC_01_016: [The maximum size of a SASL frame is defined by MIN-MAX-FRAME-SIZE.] */
			(amqp_frame_payload_size > MIX_MAX_FRAME_SIZE - 8))
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_SASL_FRAME_CODEC_01_031: [sasl_frame_codec_encode_frame shall encode the frame header by using frame_codec_begin_encode_frame.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_012: [Bytes 6 and 7 of the header are ignored.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_013: [Implementations SHOULD set these to 0x00.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_014: [The extended header is ignored.] */
			/* Codes_SRS_SASL_FRAME_CODEC_01_015: [Implementations SHOULD therefore set DOFF to 0x02.] */
			if (frame_codec_begin_encode_frame(sasl_frame_codec_instance->frame_codec, FRAME_TYPE_SASL, amqp_frame_payload_size, NULL, 0) != 0)
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				/* Codes_SRS_SASL_FRAME_CODEC_01_035: [Encoding of the sasl_frame and its fields shall be done by calling amqpvalue_encode.] */
				/* Codes_SRS_SASL_FRAME_CODEC_01_036: [The encode result for the sasl_frame and its fields shall be given to frame_codec by calling frame_codec_encode_frame_bytes.] */
				if (amqpvalue_encode(performative, frame_codec_encode_frame_bytes, sasl_frame_codec_instance->frame_codec) != 0)
				{
					/* Codes_SRS_SASL_FRAME_CODEC_01_034: [If any error occurs during encoding, sasl_frame_codec_encode_frame shall fail and return a non-zero value.] */
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
