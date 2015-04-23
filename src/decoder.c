#include <stdlib.h>
#include "decoder.h"

typedef struct DECODER_DATA_TAG
{
	unsigned char* buffer;
	size_t size;
	size_t pos;
} DECODER_DATA;

DECODER_HANDLE decoder_create(void* buffer, size_t size)
{
	DECODER_DATA* decoderData = (DECODER_DATA*)malloc(sizeof(DECODER_DATA));
	if (decoderData != NULL)
	{
		decoderData->buffer = buffer;
		decoderData->size = size;
		decoderData->pos = 0;
	}

	return decoderData;
}

void decoder_destroy(DECODER_HANDLE handle)
{
	free(handle);
}

int decoder_decode(DECODER_HANDLE handle, AMQP_VALUE* amqp_value, bool* more)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DECODER_DATA* decoderData = (DECODER_DATA*)handle;
		if (decoderData->pos < decoderData->size)
		{
			unsigned char first_constructor_byte = decoderData->buffer[decoderData->pos++];
			if (first_constructor_byte == 0x00)
			{
				/* descriptor */
				AMQP_VALUE descriptorValue;
				if (decoder_decode(handle, &descriptorValue, NULL) != 0)
				{
					result = __LINE__;
				}
				else
				{
					*amqp_value = amqpvalue_create_descriptor(descriptorValue);
					if (*amqp_value == NULL)
					{
						amqpvalue_destroy(descriptorValue);
						result = __LINE__;
					}
					else
					{
						if (more != NULL)
						{
							*more = (decoderData->pos < decoderData->size);
						}

						result = 0;
					}
				}
			}
		}
		result = 0;
	}

	return result;
}
