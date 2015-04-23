#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "amqp_types.h"
#include "amqpvalue.h"

typedef union AMQP_VALUE_UNION_TAG
{
	AMQP_VALUE descriptor;
	uint64_t ulong;
	char* string;
	size_t stringLength;
} AMQP_VALUE_UNION;

typedef struct AMQP_VALUE_DATA_TAG
{
	AMQP_TYPE type;
	AMQP_VALUE_UNION value;
} AMQP_VALUE_DATA;

AMQP_VALUE amqpvalue_create_descriptor(AMQP_VALUE value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_DESCRIPTOR;
		result->value.descriptor = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_ulong(uint64_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_ULONG;
		result->value.ulong = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_string(const char* string, uint32_t length)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_STRING;
		result->value.stringLength = strlen(string);
		result->value.string = malloc(result->value.stringLength + 1);
		if (result->value.string == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			if (strncpy(result->value.string, string, length) != 0)
			{
				free(result->value.string);
				free(result);
				result = NULL;
			}
		}
	}
	return result;
}

void amqpvalue_destroy(AMQP_VALUE value)
{
	if (value != NULL)
	{
		free(value);
	}
}
