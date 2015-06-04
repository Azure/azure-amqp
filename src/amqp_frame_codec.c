#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "amqp_frame_codec.h"
#include "frame_codec.h"
#include "amqpalloc.h"
#include "amqpvalue.h"

typedef enum AMQP_FRAME_DECODE_STATE_TAG
{
	AMQP_FRAME_DECODE_DESCRIPTOR,
	AMQP_FRAME_DECODE_FRAME_LIST,
	AMQP_FRAME_DECODE_ERROR
} AMQP_FRAME_DECODE_STATE;

typedef enum AMQP_FRAME_ENCODE_STATE_TAG
{
	AMQP_FRAME_ENCODE_FRAME_HEADER,
	AMQP_FRAME_ENCODE_FRAME_PAYLOAD,
	AMQP_FRAME_ENCODE_ERROR
} AMQP_FRAME_ENCODE_STATE;

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

	/* encode */
	AMQP_FRAME_ENCODE_STATE encode_state;
	uint32_t encode_payload_bytes_left;
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
			result->encode_state = AMQP_FRAME_ENCODE_FRAME_HEADER;

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

int amqp_frame_codec_begin_encode_frame(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, uint16_t channel, const AMQP_VALUE performative, uint32_t payload_size)
{
	int result;
	uint32_t amqp_frame_payload_size;
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec_instance = (AMQP_FRAME_CODEC_DATA*)amqp_frame_codec;

	/* Codes_SRS_AMQP_FRAME_CODEC_01_024: [If frame_codec or performative_fields is NULL, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
	if ((amqp_frame_codec == NULL) ||
		(performative == NULL) ||
		/* Codes_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.]  */
		(amqp_frame_codec_instance->encode_state != AMQP_FRAME_ENCODE_FRAME_HEADER))
	{
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_AMQP_FRAME_CODEC_01_027: [The encoded size of the performative and its fields shall be obtained by calling amqpvalue_get_encoded_size.] */
		if (amqpvalue_get_encoded_size(performative, &amqp_frame_payload_size) != 0)
		{
			/* Codes_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			unsigned char channel_bytes[2] =
			{
				channel >> 8,
				channel & 0xFF
			};

			/* Codes_SRS_AMQP_FRAME_CODEC_01_026: [The payload frame size shall be computed based on the encoded size of the performative and its fields plus the payload_size argument.] */
			amqp_frame_payload_size += payload_size;

			/* Codes_SRS_AMQP_FRAME_CODEC_01_005: [Bytes 6 and 7 of an AMQP frame contain the channel number ] */
			/* Codes_SRS_AMQP_FRAME_CODEC_01_025: [amqp_frame_codec_begin_encode_frame shall encode the frame header by using frame_codec_begin_encode_frame.] */
			/* Codes_SRS_AMQP_FRAME_CODEC_01_006: [The frame body is defined as a performative followed by an opaque payload.] */
			if (frame_codec_begin_encode_frame(amqp_frame_codec_instance->frame_codec, FRAME_TYPE_AMQP, amqp_frame_payload_size, channel_bytes, sizeof(channel_bytes)) != 0)
			{
				/* Codes_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				/* Codes_SRS_AMQP_FRAME_CODEC_01_030: [Encoding of the AMQP performative and its fields shall be done by calling amqpvalue_encode.] */
				/* Codes_SRS_AMQP_FRAME_CODEC_01_028: [The encode result for the performative and its fields shall be given to frame_codec by calling frame_codec_encode_frame_bytes.] */
				if (amqpvalue_encode(performative, frame_codec_encode_frame_bytes, amqp_frame_codec_instance->frame_codec) != 0)
				{
					/* Codes_SRS_AMQP_FRAME_CODEC_01_029: [If any error occurs during encoding, amqp_frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
					result = __LINE__;
				}
				else
				{
					/* Codes_SRS_AMQP_FRAME_CODEC_01_009: [The remaining bytes in the frame body form the payload for that frame.] */
					if (payload_size == 0)
					{
						amqp_frame_codec_instance->encode_state = AMQP_FRAME_ENCODE_FRAME_HEADER;
					}
					else
					{
						/* Codes_SRS_AMQP_FRAME_CODEC_01_023: [amqp_frame_codec_begin_encode_frame shall not encode the frame payload bytes, but switch to a state where it expects the bytes to be passed by using amqp_frame_codec_encode_payload_bytes.] */
						amqp_frame_codec_instance->encode_state = AMQP_FRAME_ENCODE_FRAME_PAYLOAD;
						amqp_frame_codec_instance->encode_payload_bytes_left = payload_size;
					}

					/* Codes_SRS_AMQP_FRAME_CODEC_01_022: [amqp_frame_codec_begin_encode_frame shall encode the frame header and AMQP performative in an AMQP frame and on success it shall return 0.] */
					result = 0;
				}
			}
		}
	}

	return result;
}

/* Codes_SRS_AMQP_FRAME_CODEC_01_031: [amqp_frame_codec_encode_payload_bytes shall encode the bytes for a frame that was begun with amqp_frame_codec_begin_encode_frame.] */
extern int amqp_frame_codec_encode_payload_bytes(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, const unsigned char* bytes, uint32_t count)
{
	int result;
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec_instance = (AMQP_FRAME_CODEC_DATA*)amqp_frame_codec;

	/* Codes_SRS_AMQP_FRAME_CODEC_01_033: [If amqp_frame_codec or bytes is NULL, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
	if ((amqp_frame_codec == NULL) ||
		(bytes == NULL) ||
		/* Codes_SRS_AMQP_FRAME_CODEC_01_034: [If count is 0, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
		(count == 0) ||
		/* Codes_SRS_AMQP_FRAME_CODEC_01_047: [If amqp_frame_codec_encode_payload_bytes is called without starting a frame encode, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
		(amqp_frame_codec_instance->encode_state != AMQP_FRAME_ENCODE_FRAME_PAYLOAD) ||
		/* Codes_.] SRS_AMQP_FRAME_CODEC_01_036: [If count is greater than the number of bytes still left to be encoded, amqp_frame_codec_encode_payload_bytes shall fail and return a non-zero value.] */
		(amqp_frame_codec_instance->encode_payload_bytes_left < count))
	{
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_AMQP_FRAME_CODEC_01_037: [The bytes shall be passed to frame codec by a call to frame_codec_encode_frame_bytes.] */
		if (frame_codec_encode_frame_bytes(amqp_frame_codec_instance->frame_codec, bytes, count) != 0)
		{
			/* Codes_SRS_AMQP_FRAME_CODEC_01_040: [In case encoding fails due to inability to give the data to the frame_codec, any subsequent attempt to begin encoding a frame shall fail.] */
			amqp_frame_codec_instance->encode_state = AMQP_FRAME_ENCODE_ERROR;

			/* Codes_SRS_AMQP_FRAME_CODEC_01_038: [If encoding fails in any way not specified here, amqp_frame_codec_encode_payload_bytes shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQP_FRAME_CODEC_01_041: [If all bytes for the frame payload were given and amqp_frame_codec_encode_payload_bytes is called again, it shall return a non-zero value, but subsequent encoding attempts shall succeed.] */
			/* Codes_SRS_AMQP_FRAME_CODEC_01_035: [amqp_frame_codec shall maintain the number of bytes needed to be sent as the payload for the frame started with amqp_frame_codec_begin_encode_frame.] */
			/* Codes_SRS_AMQP_FRAME_CODEC_01_039: [The frame payload bytes for one frame shall be allowed to be given by multiple calls to amqp_frame_codec_encode_payload_bytes.] */
			amqp_frame_codec_instance->encode_payload_bytes_left -= count;
			if (amqp_frame_codec_instance->encode_payload_bytes_left == 0)
			{
				amqp_frame_codec_instance->encode_state = AMQP_FRAME_ENCODE_FRAME_HEADER;
			}

			/* Codes_SRS_AMQP_FRAME_CODEC_01_032: [On success amqp_frame_codec_encode_payload_bytes shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQP_FRAME_CODEC_01_042: [amqp_frame_codec_encode_empty_frame shall encode a frame with no payload.] */
/* Codes_SRS_AMQP_FRAME_CODEC_01_010: [An AMQP frame with no body MAY be used to generate artificial traffic as needed to satisfy any negotiated idle timeout interval ] */
int amqp_frame_codec_encode_empty_frame(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, uint16_t channel)
{
	int result;
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec_instance = (AMQP_FRAME_CODEC_DATA*)amqp_frame_codec;

	/* Codes_SRS_AMQP_FRAME_CODEC_01_045: [If amqp_frame_codec is NULL, amqp_frame_codec_encode_empty_frame shall fail and return a non-zero value.] */
	if (amqp_frame_codec == NULL)
	{
		result = __LINE__;
	}
	else
	{
		unsigned char channel_bytes[2] =
		{
			channel >> 8,
			channel & 0xFF
		};

		/* Codes_SRS_AMQP_FRAME_CODEC_01_044: [amqp_frame_codec_encode_empty_frame shall use frame_codec_begin_encode_frame to send the frame.] */
		if (frame_codec_begin_encode_frame(amqp_frame_codec_instance->frame_codec, FRAME_TYPE_AMQP, 0, channel_bytes, sizeof(channel_bytes)) != 0)
		{
			/* Codes_SRS_AMQP_FRAME_CODEC_01_046: [If encoding fails in any way, amqp_frame_codec_encode_empty_frame shall fail and return a non-zero value.]  */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQP_FRAME_CODEC_01_043: [On success, amqp_frame_codec_encode_empty_frame shall return 0.] */
			result = 0;
		}
	}

	return result;
}
