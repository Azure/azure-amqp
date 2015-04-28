#include <stdlib.h>
#include "frame_codec.h"
#include "io.h"

typedef struct FRAME_CODEC_DATA_TAG
{
	IO_HANDLE io;
} FRAME_CODEC_DATA;

FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io)
{
	FRAME_CODEC_DATA* result;
	result = malloc(sizeof(FRAME_CODEC_DATA));
	if (result != NULL)
	{
		result->io = io;
	}

	return result;
}

void frame_codec_free(FRAME_CODEC_HANDLE handle)
{
	free(handle);
}
