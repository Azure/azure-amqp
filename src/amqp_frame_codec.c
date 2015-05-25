#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "amqp_frame_codec.h"
#include "frame_codec.h"
#include "encoder.h"

typedef struct AMQP_FRAME_CODEC_DATA_TAG
{
	FRAME_CODEC_HANDLE frame_codec_handle;
} AMQP_FRAME_CODEC_DATA;

AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec_handle)
{
	AMQP_FRAME_CODEC_DATA* result = (AMQP_FRAME_CODEC_DATA*)malloc(sizeof(AMQP_FRAME_CODEC_DATA));
	if (result != NULL)
	{
		result->frame_codec_handle = frame_codec_handle;
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
