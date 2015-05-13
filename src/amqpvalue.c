#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "amqp_types.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

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
	uint32_t length;
} AMQP_STRING_VALUE;

typedef struct AMQP_BINARY_VALUE_TAG
{
	unsigned char* bytes;
	uint32_t length;
} AMQP_BINARY_VALUE;

typedef union AMQP_VALUE_UNION_TAG
{
	AMQP_VALUE descriptor;
	uint64_t ulong;
	uint32_t uint;
	uint16_t ushort;
	unsigned char ubyte;
	bool bool_value;
	AMQP_STRING_VALUE string_value;
	AMQP_BINARY_VALUE binary_value;
	AMQP_LIST_VALUE list_value;
	AMQP_COMPOSITE_VALUE composite_value;
} AMQP_VALUE_UNION;

typedef struct AMQP_VALUE_DATA_TAG
{
	AMQP_TYPE type;
	AMQP_VALUE_UNION value;
} AMQP_VALUE_DATA;

/* Codes_SRS_AMQPVALUE_01_003: [1.6.1 null Indicates an empty value.] */
AMQP_VALUE amqpvalue_create_null(void)
{
	/* Codes_SRS_AMQPVALUE_01_002: [If allocating the AMQP_VALUE fails then amqpvalue_create_null shall return NULL.] */
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_001: [amqpvalue_create_null shall return a handle to an AMQP_VALUE that stores a null value.] */
		result->type = AMQP_TYPE_NULL;
	}
	return result;
}

/* Codes_SRS_AMQPVALUE_01_004: [1.6.2 boolean Represents a true or false value.] */
AMQP_VALUE amqpvalue_create_boolean(bool value)
{
	/* Codes_SRS_AMQPVALUE_01_007: [If allocating the AMQP_VALUE fails then amqpvalue_create_boolean shall return NULL.] */
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_006: [amqpvalue_create_boolean shall return a handle to an AMQP_VALUE that stores a boolean value.] */
		result->type = AMQP_TYPE_BOOL;
		result->value.bool_value = value;
	}

	return result;
}

AMQP_VALUE amqpvalue_create_descriptor(AMQP_VALUE value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_DESCRIPTOR;
		result->value.descriptor = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_ulong(uint64_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_ULONG;
		result->value.ulong = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_ushort(uint16_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
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

AMQP_VALUE amqpvalue_create_string_with_length(const char* value, uint32_t length)
{
	AMQP_VALUE_DATA* result;
	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
		if (result != NULL)
		{
			result->type = AMQP_TYPE_STRING;
			result->value.string_value.chars = amqpalloc_malloc(length + 1);
			result->value.string_value.length = length;
			if (result->value.string_value.chars == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (memcpy(result->value.string_value.chars, value, length) == NULL)
				{
					amqpalloc_free(result->value.string_value.chars);
					amqpalloc_free(result);
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

AMQP_VALUE amqpvalue_create_binary(const void* value, uint32_t length)
{
	AMQP_VALUE_DATA* result;
	if ((value == NULL) &&
		(length > 0))
	{
		result = NULL;
	}
	else
	{
		result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
		if (result != NULL)
		{
			result->type = AMQP_TYPE_BINARY;
			result->value.binary_value.bytes = amqpalloc_malloc(length);
			result->value.binary_value.length = length;
			if (result->value.binary_value.bytes == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (memcpy(result->value.binary_value.bytes, value, length) == NULL)
				{
					amqpalloc_free(result->value.binary_value.bytes);
					amqpalloc_free(result);
					result = NULL;
				}
			}
		}
	}
	return result;
}

AMQP_VALUE amqpvalue_create_list(size_t size)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_LIST;
		result->value.list_value.count = size;
		result->value.list_value.items = (AMQP_VALUE*)amqpalloc_malloc(sizeof(AMQP_VALUE*) * size);
		if (result->value.list_value.items == NULL)
		{
			amqpalloc_free(result);
			result = NULL;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_create_composite_with_ulong_descriptor(uint64_t descriptor, size_t size)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		AMQP_VALUE descriptor_ulong_value = amqpvalue_create_ulong(descriptor);
		if (descriptor_ulong_value == NULL)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			result->type = AMQP_TYPE_COMPOSITE;
			result->value.composite_value.descriptor = amqpvalue_create_descriptor(descriptor_ulong_value);
			if (result->value.composite_value.descriptor == NULL)
			{
				amqpalloc_free(descriptor_ulong_value);
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->value.composite_value.list = amqpvalue_create_list(size);
				if (result->value.composite_value.list == NULL)
				{
					amqpalloc_free(result);
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
		switch (value_data->type)
		{
		default:
			break;
		case AMQP_TYPE_LIST:
		{
			size_t i;
			for (i = 0; i < value_data->value.list_value.count; i++)
			{
				amqpvalue_destroy(value_data->value.list_value.items[i]);
			}

			amqpalloc_free(value_data->value.list_value.items);
			break;
		}
		case AMQP_TYPE_BINARY:
			amqpalloc_free(value_data->value.binary_value.bytes);
			break;
		case AMQP_TYPE_STRING:
			amqpalloc_free(value_data->value.string_value.chars);
			break;
		case AMQP_TYPE_COMPOSITE:
			amqpvalue_destroy(value_data->value.composite_value.descriptor);
			amqpvalue_destroy(value_data->value.composite_value.list);
			break;
		case AMQP_TYPE_DESCRIPTOR:
			amqpvalue_destroy(value_data->value.descriptor);
			break;
		}

		amqpalloc_free(value);
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

const unsigned char* amqpvalue_get_binary_content(AMQP_VALUE value, uint32_t* length)
{
	const char* result;

	if ((value == NULL) ||
		(length == NULL))
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_BINARY)
		{
			result = NULL;
		}
		else
		{
			*length = value_data->value.binary_value.length;
			result = value_data->value.binary_value.bytes;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_create_uint(uint32_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_UINT;
		result->value.uint = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_ubyte(unsigned char value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_UBYTE;
		result->value.ubyte = value;
	}
	return result;
}

int amqpvalue_get_boolean(AMQP_VALUE value, bool* bool_value)
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

		case AMQP_TYPE_DESCRIPTOR:
		case AMQP_TYPE_NULL:
		case AMQP_TYPE_LIST:
			result = NULL;
			break;

		case AMQP_TYPE_STRING:
			result = amqpvalue_create_string_with_length(value_data->value.string_value.chars, value_data->value.string_value.length);
			break;

		case AMQP_TYPE_ULONG:
		case AMQP_TYPE_UINT:
		case AMQP_TYPE_USHORT:
		case AMQP_TYPE_BOOL:
		case AMQP_TYPE_UBYTE:
		case AMQP_TYPE_COMPOSITE:
		case AMQP_TYPE_BINARY:
			result = NULL;
			break;
		}
	}

	return result;
}
