#include "encoder.h"
#include <stdlib.h>

ENCODER_HANDLE encoder_create(void)
{
	return NULL;
}

void encoder_destroy(ENCODER_HANDLE handle)
{
	(void)handle;
}

int encoder_encode_string(ENCODER_HANDLE handle, const char* value)
{
	(void)handle;
	(void)value;

	return 0;
}
