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
	unsigned char ubyte_value;
	uint16_t ushort_value;
	uint32_t uint_value;
	uint64_t ulong_value;
	char byte_value;
	int16_t short_value;
	int32_t int_value;
	int64_t long_value;
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

int amqpvalue_get_boolean(AMQP_VALUE value, bool* bool_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_009: [If any of the arguments is NULL then amqpvalue_get_boolean shall return a non-zero value.] */
	if ((value == NULL) ||
		(bool_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_011: [If the type of the value is not Boolean, then amqpvalue_get_boolean shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_BOOL)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_008: [amqpvalue_get_boolean shall fill in the bool_value argument the Boolean value stored by the AMQP value indicated by the value argument.] */
			*bool_value = value_data->value.bool_value;

			/* Codes_SRS_AMQPVALUE_01_010: [On success amqpvalue_get_boolean shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_005: [1.6.3 ubyte Integer in the range 0 to 28 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_ubyte(unsigned char value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_032: [amqpvalue_create_ubyte shall return a handle to an AMQP_VALUE that stores a unsigned char value.] */
		result->type = AMQP_TYPE_UBYTE;
		result->value.ubyte_value = value;
	}

	return result;
}

int amqpvalue_get_ubyte(AMQP_VALUE value, unsigned char* ubyte_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_036: [If any of the arguments is NULL then amqpvalue_get_ubyte shall return a non-zero value.] */
	if ((value == NULL) ||
		(ubyte_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_037: [If the type of the value is not ubyte (was not created with amqpvalue_create_ubyte), then amqpvalue_get_ubyte shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_UBYTE)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_034: [amqpvalue_get_ubyte shall fill in the ubyte_value argument the unsigned char value stored by the AMQP value indicated by the value argument.] */
			*ubyte_value = value_data->value.ubyte_value;

			/* Codes_SRS_AMQPVALUE_01_035: [On success amqpvalue_get_ubyte shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_012: [1.6.4 ushort Integer in the range 0 to 216 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_ushort(uint16_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_039: [If allocating the AMQP_VALUE fails then amqpvalue_create_ushort shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_038: [amqpvalue_create_ushort shall return a handle to an AMQP_VALUE that stores an uint16_t value.] */
		result->type = AMQP_TYPE_USHORT;
		result->value.ushort_value = value;
	}
	return result;
}

int amqpvalue_get_ushort(AMQP_VALUE value, uint16_t* ushort_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_042: [If any of the arguments is NULL then amqpvalue_get_ushort shall return a non-zero value.] */
	if ((value == NULL) ||
		(ushort_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_043: [If the type of the value is not ushort (was not created with amqpvalue_create_ushort), then amqpvalue_get_ushort shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_USHORT)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_040: [amqpvalue_get_ushort shall fill in the ushort_value argument the uint16_t value stored by the AMQP value indicated by the value argument.] */
			*ushort_value = value_data->value.ushort_value;

			/* Codes_SRS_AMQPVALUE_01_041: [On success amqpvalue_get_ushort shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_013: [1.6.5 uint Integer in the range 0 to 232 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_uint(uint32_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_045: [If allocating the AMQP_VALUE fails then amqpvalue_create_uint shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_044: [amqpvalue_create_uint shall return a handle to an AMQP_VALUE that stores an uint32_t value.] */
		result->type = AMQP_TYPE_UINT;
		result->value.uint_value = value;
	}
	return result;
}

int amqpvalue_get_uint(AMQP_VALUE value, uint32_t* uint_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_079: [If any of the arguments is NULL then amqpvalue_get_uint shall return a non-zero value.] */
	if ((value == NULL) ||
		(uint_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_048: [If the type of the value is not uint (was not created with amqpvalue_create_uint), then amqpvalue_get_uint shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_UINT)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_046: [amqpvalue_get_uint shall fill in the uint_value argument the uint32_t value stored by the AMQP value indicated by the value argument.] */
			*uint_value = value_data->value.uint_value;

			/* Codes_SRS_AMQPVALUE_01_047: [On success amqpvalue_get_uint shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_014: [1.6.6 ulong Integer in the range 0 to 264 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_ulong(uint64_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_050: [If allocating the AMQP_VALUE fails then amqpvalue_create_ulong shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_049: [amqpvalue_create_ulong shall return a handle to an AMQP_VALUE that stores an uint64_t value.] */
		result->type = AMQP_TYPE_ULONG;
		result->value.ulong_value = value;
	}
	return result;
}

int amqpvalue_get_ulong(AMQP_VALUE value, uint64_t* ulong_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_053: [If any of the arguments is NULL then amqpvalue_get_ulong shall return a non-zero value.] */
	if ((value == NULL) ||
		(ulong_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_054: [If the type of the value is not ulong (was not created with amqpvalue_create_ulong), then amqpvalue_get_ulong shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_ULONG)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_051: [amqpvalue_get_ulong shall fill in the ulong_value argument the ulong64_t value stored by the AMQP value indicated by the value argument.] */
			*ulong_value = value_data->value.ulong_value;

			/* Codes_SRS_AMQPVALUE_01_052: [On success amqpvalue_get_ulong shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_015: [1.6.7 byte Integer in the range -(27) to 27 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_byte(char value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_056: [If allocating the AMQP_VALUE fails then amqpvalue_create_byte shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_055: [amqpvalue_create_byte shall return a handle to an AMQP_VALUE that stores a char value.] */
		result->type = AMQP_TYPE_BYTE;
		result->value.byte_value = value;
	}
	return result;
}

int amqpvalue_get_byte(AMQP_VALUE value, char* byte_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_059: [If any of the arguments is NULL then amqpvalue_get_byte shall return a non-zero value.] */
	if ((value == NULL) ||
		(byte_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_060: [If the type of the value is not byte (was not created with amqpvalue_create_byte), then amqpvalue_get_byte shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_BYTE)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_057: [amqpvalue_get_byte shall fill in the byte_value argument the char value stored by the AMQP value indicated by the value argument.] */
			*byte_value = value_data->value.byte_value;

			/* Codes_SRS_AMQPVALUE_01_058: [On success amqpvalue_get_byte shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_016: [1.6.8 short Integer in the range -(215) to 215 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_short(int16_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_062: [If allocating the AMQP_VALUE fails then amqpvalue_create_short shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_061: [amqpvalue_create_short shall return a handle to an AMQP_VALUE that stores an int16_t value.] */
		result->type = AMQP_TYPE_SHORT;
		result->value.short_value = value;
	}
	return result;
}

int amqpvalue_get_short(AMQP_VALUE value, int16_t* short_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_065: [If any of the arguments is NULL then amqpvalue_get_short shall return a non-zero value.] */
	if ((value == NULL) ||
		(short_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_066: [If the type of the value is not short (was not created with amqpvalue_create_short), then amqpvalue_get_short shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_SHORT)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_063: [amqpvalue_get_short shall fill in the short_value argument the int16_t value stored by the AMQP value indicated by the value argument.] */
			*short_value = value_data->value.short_value;

			/* Codes_SRS_AMQPVALUE_01_064: [On success amqpvalue_get_short shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_017: [1.6.9 int Integer in the range -(231) to 231 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_int(int32_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_068: [If allocating the AMQP_VALUE fails then amqpvalue_create_int shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_067: [amqpvalue_create_int shall return a handle to an AMQP_VALUE that stores an int32_t value.] */
		result->type = AMQP_TYPE_INT;
		result->value.int_value = value;
	}
	return result;
}

int amqpvalue_get_int(AMQP_VALUE value, int32_t* int_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_071: [If any of the arguments is NULL then amqpvalue_get_int shall return a non-zero value.] */
	if ((value == NULL) ||
		(int_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_072: [If the type of the value is not int (was not created with amqpvalue_create_int), then amqpvalue_get_int shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_INT)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_069: [amqpvalue_get_int shall fill in the int_value argument the int32_t value stored by the AMQP value indicated by the value argument.] */
			*int_value = value_data->value.int_value;

			/* Codes_SRS_AMQPVALUE_01_070: [On success amqpvalue_get_int shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_018: [1.6.10 long Integer in the range -(263) to 263 - 1 inclusive.] */
AMQP_VALUE amqpvalue_create_long(int64_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_074: [If allocating the AMQP_VALUE fails then amqpvalue_create_long shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_073: [amqpvalue_create_long shall return a handle to an AMQP_VALUE that stores an int64_t value.] */
		result->type = AMQP_TYPE_LONG;
		result->value.long_value = value;
	}
	return result;
}

int amqpvalue_get_long(AMQP_VALUE value, int64_t* long_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_077: [If any of the arguments is NULL then amqpvalue_get_long shall return a non-zero value.] */
	if ((value == NULL) ||
		(long_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_078: [If the type of the value is not long (was not created with amqpvalue_create_long), then amqpvalue_get_long shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_LONG)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_075: [amqpvalue_get_long shall fill in the long_value argument the int64_t value stored by the AMQP value indicated by the value argument.] */
			*long_value = value_data->value.long_value;

			/* Codes_SRS_AMQPVALUE_01_076: [On success amqpvalue_get_long shall return 0.] */
			result = 0;
		}
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
