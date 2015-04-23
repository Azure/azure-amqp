#include <stdlib.h>
#include "decoder.h"

typedef struct DECODER_DATA_TAG
{
	void* buffer;
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
	(void)handle;
	(void)amqp_value;
	return 0;
}
