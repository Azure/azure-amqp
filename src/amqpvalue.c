#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "amqp_types.h"
#include "amqpvalue.h"

typedef struct AMQP_LIST_VALUE_TAG
{
	AMQP_VALUE* items;
	size_t count;
} AMQP_LIST_VALUE;

typedef struct AMQP_COMPOSITE_VALUE_TAG
{
	AMQP_VALUE descriptor;
	AMQP_VALUE list;
} AMQP_COMPOSITE_VALUE;

typedef struct AMQP_STRING_VALUE_TAG
{
	char* chars;
	size_t length;
} AMQP_STRING_VALUE;

typedef union AMQP_VALUE_UNION_TAG
{
	AMQP_VALUE descriptor;
	uint64_t ulong;
	uint32_t uint;
	uint16_t ushort;
	unsigned char ubyte;
	bool bool_value;
	AMQP_STRING_VALUE string_value;
	AMQP_LIST_VALUE list_value;
	AMQP_COMPOSITE_VALUE composite_value;
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

AMQP_VALUE amqpvalue_create_ushort(uint16_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_USHORT;
		result->value.ushort = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_string(const char* value)
{
	AMQP_VALUE_DATA* result;
	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		size_t length = strlen(value);
		result = amqpvalue_create_string_with_length(value, length);
	}
	return result;
}

AMQP_VALUE amqpvalue_create_string_with_length(const char* value, size_t length)
{
	AMQP_VALUE_DATA* result;
	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
		if (result != NULL)
		{
			result->type = AMQP_TYPE_STRING;
			result->value.string_value.chars = malloc(length + 1);
			result->value.string_value.length = length;
			if (result->value.string_value.chars == NULL)
			{
				free(result);
				result = NULL;
			}
			else
			{
				if (strncpy(result->value.string_value.chars, value, length) == NULL)
				{
					free(result->value.string_value.chars);
					free(result);
					result = NULL;
				}
				else
				{
					result->value.string_value.chars[length] = 0;
				}
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
		result->value.list_value.count = size;
		result->value.list_value.items = (AMQP_VALUE*)malloc(sizeof(AMQP_VALUE*) * size);
		if (result->value.list_value.items == NULL)
		{
			free(result);
			result = NULL;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_create_composite_with_ulong_descriptor(uint64_t descriptor, size_t size)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		AMQP_VALUE descriptor_ulong_value = amqpvalue_create_ulong(descriptor);
		if (descriptor_ulong_value == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->type = AMQP_TYPE_COMPOSITE;
			result->value.composite_value.descriptor = amqpvalue_create_descriptor(descriptor_ulong_value);
			if (result->value.composite_value.descriptor == NULL)
			{
				free(descriptor_ulong_value);
				free(result);
				result = NULL;
			}
			else
			{
				result->value.composite_value.list = amqpvalue_create_list(size);
				if (result->value.composite_value.list == NULL)
				{
					free(result);
					result = NULL;
				}
			}
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
			if (index >= value_data->value.list_value.count)
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

int amqpvalue_get_type(AMQP_VALUE value, AMQP_TYPE* type)
{
	int result;

	if ((value == NULL) ||
		(type == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		*type = value_data->type;
		result = 0;
	}

	return result;
}

int amqpvalue_get_list_item_count(AMQP_VALUE value, size_t* count)
{
	int result;

	if ((value == NULL) ||
		(count == NULL))
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
			*count = value_data->value.list_value.count;
			result = 0;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_list_item(AMQP_VALUE value, size_t index)
{
	AMQP_VALUE result;

	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_LIST)
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.list_value.items[index];
		}
	}

	return result;
}

const char* amqpvalue_get_string(AMQP_VALUE value)
{
	const char* result;

	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_STRING)
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.string_value.chars;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_create_uint(uint32_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_UINT;
		result->value.uint = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_ubyte(unsigned char value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_UBYTE;
		result->value.ubyte = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_bool(bool value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_BOOL;
		result->value.bool_value = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_null(void)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_NULL;
	}
	return result;
}

int amqpvalue_get_bool(AMQP_VALUE value, bool* bool_value)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_BOOL)
		{
			result = __LINE__;
		}
		else
		{
			*bool_value = value_data->value.bool_value;
			result = 0;
		}
	}

	return result;
}

int amqpvalue_get_ubyte(AMQP_VALUE value, unsigned char* ubyte_value)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_UBYTE)
		{
			result = __LINE__;
		}
		else
		{
			*ubyte_value = value_data->value.ubyte;
			result = 0;
		}
	}

	return result;
}

int amqpvalue_get_uint(AMQP_VALUE value, uint32_t* uint_value)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_UINT)
		{
			result = __LINE__;
		}
		else
		{
			*uint_value = value_data->value.uint;
			result = 0;
		}
	}

	return result;
}

int amqpvalue_get_ulong(AMQP_VALUE value, uint64_t* ulong_value)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_ULONG)
		{
			result = __LINE__;
		}
		else
		{
			*ulong_value = value_data->value.ulong;
			result = 0;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_descriptor(AMQP_VALUE value)
{
	AMQP_VALUE result;

	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_DESCRIPTOR)
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.descriptor;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_composite_descriptor(AMQP_VALUE value)
{
	AMQP_VALUE result;

	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_COMPOSITE)
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.composite_value.descriptor;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_composite_list(AMQP_VALUE value)
{
	AMQP_VALUE result;

	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_COMPOSITE)
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.composite_value.list;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_clone(AMQP_VALUE value)
{
	AMQP_VALUE result;
	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		switch (value_data->type)
		{
		default:
			result = NULL;
			break;

		case AMQP_TYPE_STRING:
			result = amqpvalue_create_string(value_data->value.string_value.chars);
			break;
		}
	}

	return result;
}
