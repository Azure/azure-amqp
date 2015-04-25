#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "amqp_types.h"
#include "amqpvalue.h"

typedef struct AMQP_LIST_VALUE_TAG
{
	AMQP_VALUE* items;
	size_t size;
} AMQP_LIST_VALUE;

typedef struct AMQP_STRING_VALUE_TAG
{
	char* chars;
	size_t length;
} AMQP_STRING_VALUE;

typedef union AMQP_VALUE_UNION_TAG
{
	AMQP_VALUE descriptor;
	uint64_t ulong;
	AMQP_STRING_VALUE string_value;
	AMQP_LIST_VALUE list_value;
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
		result->value.string_value.length = strlen(string);
		result->value.string_value.chars = malloc(result->value.string_value.length + 1);
		if (result->value.string_value.chars == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			if (strncpy(result->value.string_value.chars, string, length) != 0)
			{
				free(result->value.string_value.chars);
				free(result);
				result = NULL;
			}
		}
	}
	return result;
}

AMQP_VALUE amqpvalue_create_list(size_t size)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_LIST;
		result->value.list_value.items = (AMQP_VALUE*)malloc(sizeof(AMQP_VALUE*) * size);
		if (result->value.list_value.items == NULL)
		{
			free(result);
			result = NULL;
		}
	}

	return result;
}

int amqpvalue_set_list_item(AMQP_VALUE value, size_t index, AMQP_VALUE list_item_value)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_LIST)
		{
			result = __LINE__;
		}
		else
		{
			if (index >= value_data->value.list_value.size)
			{
				result = __LINE__;
			}
			else
			{
				value_data->value.list_value.items[index] = list_item_value;
				result = 0;
			}
		}
	}

	return result;
}

void amqpvalue_destroy(AMQP_VALUE value)
{
	if (value != NULL)
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type == AMQP_TYPE_LIST)
		{
			free(value_data->value.list_value.items);
		}

		free(value);
	}
}
