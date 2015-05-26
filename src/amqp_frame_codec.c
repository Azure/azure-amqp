#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "amqp_frame_codec.h"
#include "frame_codec.h"
#include "encoder.h"
#include "decoder.h"

typedef struct AMQP_FRAME_CODEC_DATA_TAG
{
	FRAME_CODEC_HANDLE frame_codec_handle;
	AMQP_FRAME_RECEIVED_CALLBACK frame_receive_callback;
	void* frame_receive_callback_context;
} AMQP_FRAME_CODEC_DATA;

static void frame_received(void* context, uint8_t type, const unsigned char* frame_body, uint32_t frame_body_size, const unsigned char* type_specific, uint32_t type_specific_size)
{
	AMQP_FRAME_CODEC_DATA* amqp_frame_codec = (AMQP_FRAME_CODEC_DATA*)context;
	uint16_t channel;
	DECODER_HANDLE decoder_handle;
	AMQP_VALUE descriptor = NULL;
	AMQP_VALUE frame_list_value = NULL;
	int result;
	bool more;
	uint64_t descriptor_ulong_value;

	channel = type_specific[0] << 8;
	channel += type_specific[1];

	decoder_handle = decoder_create(frame_body, frame_body_size);
	if (decoder_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if ((decoder_decode(decoder_handle, &descriptor, &more) != 0) ||
			(!more) ||
			(decoder_decode(decoder_handle, &frame_list_value, &more) != 0) ||
			(amqpvalue_get_ulong(amqpvalue_get_descriptor(descriptor), &descriptor_ulong_value) != 0))
		{
			result = __LINE__;
		}
		else
		{

			/* notify of received frame */
			if (amqp_frame_codec->frame_receive_callback != NULL)
			{
				amqp_frame_codec->frame_receive_callback(amqp_frame_codec->frame_receive_callback_context, descriptor_ulong_value, frame_list_value);
			}

			result = 0;
		}

		amqpvalue_destroy(descriptor);
		amqpvalue_destroy(frame_list_value);

		decoder_destroy(decoder_handle);
	}
}

AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec_handle, AMQP_FRAME_RECEIVED_CALLBACK frame_receive_callback, void* frame_receive_callback_context)
{
	AMQP_FRAME_CODEC_DATA* result = (AMQP_FRAME_CODEC_DATA*)malloc(sizeof(AMQP_FRAME_CODEC_DATA));
	if (result != NULL)
	{
		result->frame_codec_handle = frame_codec_handle;
		result->frame_receive_callback = frame_receive_callback;
		result->frame_receive_callback_context = frame_receive_callback_context;

		frame_codec_subscribe(frame_codec_handle, 0, frame_received, result);
	}

	return result;
}

void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_handle)
{
	if (amqp_frame_codec_handle != NULL)
	{
		free(amqp_frame_codec_handle);
	}
}

int amqp_frame_codec_encode(FRAME_CODEC_HANDLE frame_codec_handle, uint64_t performative, const AMQP_VALUE* frame_content_chunks, size_t frame_content_chunk_count)
{
	int result;
	ENCODER_HANDLE encoder_handle = encoder_create(NULL, NULL);
	uint32_t amqp_frame_payload_size;

	if ((encoder_handle == NULL) ||
		(frame_content_chunks == NULL) ||
		frame_content_chunk_count == 0)
	{
		result = __LINE__;
	}
	else
	{
		if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
			(encoder_encode_ulong(encoder_handle, performative) != 0))
		{
			result = __LINE__;
		}
		else
		{
			size_t i;

			for (i = 0; i < frame_content_chunk_count; i++)
			{
				if (encoder_encode_amqp_value(encoder_handle, frame_content_chunks[i]) != 0)
				{
					break;
				}
			}

			if ((i < frame_content_chunk_count) ||
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
		if (frame_codec_start_encode_frame(frame_codec_handle, amqp_frame_payload_size) != 0)
		{
			result = __LINE__;
		}
		else
		{
			encoder_handle = encoder_create(frame_codec_encode_frame_bytes, frame_codec_handle);
			if (encoder_handle == NULL)
			{
				result = __LINE__;
			}
			else
			{
				if ((encoder_encode_descriptor_header(encoder_handle) != 0) ||
					(encoder_encode_ulong(encoder_handle, performative) != 0))
				{
					result = __LINE__;
				}
				else
				{
					size_t i;

					for (i = 0; i < frame_content_chunk_count; i++)
					{
						if (encoder_encode_amqp_value(encoder_handle, frame_content_chunks[i]) != 0)
						{
							break;
						}
					}

					if (i < frame_content_chunk_count)
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
