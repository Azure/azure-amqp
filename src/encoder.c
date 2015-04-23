#include "encoder.h"
#include <stdlib.h>
#include <string.h>

typedef struct ENCODER_DATA_TAG
{
	size_t encodedBytes;
	ENCODER_OUTPUT encoderOutput;
	void* context;
} ENCODER_DATA;

ENCODER_HANDLE encoder_create(ENCODER_OUTPUT encoderOutput, void* context)
{
	ENCODER_DATA* encoderData = (ENCODER_DATA*)malloc(sizeof(ENCODER_DATA));
	if (encoderData != NULL)
	{
		encoderData->encodedBytes = 0;
		encoderData->encoderOutput = encoderOutput;
		encoderData->context = context;
	}

	return encoderData;
}

void encoder_destroy(ENCODER_HANDLE handle)
{
	free(handle);
}

static int output_byte(ENCODER_DATA* encoderData, unsigned char b)
{
	int result;

	encoderData->encodedBytes++;
	if (encoderData->encoderOutput != NULL)
	{
		result = encoderData->encoderOutput(encoderData->context, &b, 1);
	}
	else
	{
		result = 0;
	}

	return result;
}

static int output_bytes(ENCODER_DATA* encoderData, const void* bytes, size_t length)
{
	int result;

	encoderData->encodedBytes += length;
	if (encoderData->encoderOutput != NULL)
	{
		result = encoderData->encoderOutput(encoderData->context, bytes, length);
	}
	else
	{
		result = 0;
	}

	return result;
}

int encoder_encode_string(ENCODER_HANDLE handle, const char* value)
{
	int result;
	if ((handle == NULL) ||
		(value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoderData = (ENCODER_DATA*)handle;
		size_t length = strlen(value);
		
		if (length <= 255)
		{
			output_byte(encoderData, (unsigned char)0xA1);
			output_byte(encoderData, (unsigned char)length);
			output_bytes(encoderData, value, length);
		}
		else
		{
			output_byte(encoderData, 0xB1);
			output_byte(encoderData, (length >> 24) & 0xFF);
			output_byte(encoderData, (length >> 16) & 0xFF);
			output_byte(encoderData, (length >> 8) & 0xFF);
			output_byte(encoderData, length & 0xFF);
			output_bytes(encoderData, value, length);
		}

		result = 0;
	}

	return result;
}

int encoder_encode_ulong(ENCODER_HANDLE handle, uint64_t value)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoderData = (ENCODER_DATA*)handle;

		if (value == 0)
		{
			output_byte(encoderData, 0x44);
		}
		else if (value <= 255)
		{
			output_byte(encoderData, 0x53);
			output_byte(encoderData, value & 0xFF);
		}
		else
		{
			output_byte(encoderData, (value >> 56) & 0xFF);
			output_byte(encoderData, (value >> 48) & 0xFF);
			output_byte(encoderData, (value >> 40) & 0xFF);
			output_byte(encoderData, (value >> 32) & 0xFF);
			output_byte(encoderData, (value >> 24) & 0xFF);
			output_byte(encoderData, (value >> 16) & 0xFF);
			output_byte(encoderData, (value >> 8) & 0xFF);
			output_byte(encoderData, value & 0xFF);
		}

		result = 0;
	}

	return result;
}

int encoder_get_encoded_size(ENCODER_HANDLE handle, size_t* size)
{
	int result;
	if ((handle == NULL) ||
		(size == NULL))
	{
		result = __LINE__;
	}
	else
	{
		*size = ((ENCODER_DATA*)handle)->encodedBytes;
		result = 0;
	}

	return result;
}

int encoder_encode_descriptor_header(ENCODER_HANDLE handle)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoderData = (ENCODER_DATA*)handle;
		output_byte(encoderData, 0x00);

		result = 0;
	}

	return result;
}
