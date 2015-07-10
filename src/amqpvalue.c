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
	uint32_t count;
} AMQP_LIST_VALUE;

typedef struct AMQP_MAP_KEY_VALUE_PAIR_TAG
{
	AMQP_VALUE key;
	AMQP_VALUE value;
} AMQP_MAP_KEY_VALUE_PAIR;

typedef struct AMQP_MAP_VALUE_TAG
{
	AMQP_MAP_KEY_VALUE_PAIR* pairs;
	uint32_t pair_count;
} AMQP_MAP_VALUE;

typedef struct AMQP_STRING_VALUE_TAG
{
	char* chars;
} AMQP_STRING_VALUE;

typedef struct AMQP_BINARY_VALUE_TAG
{
	unsigned char* bytes;
	uint32_t length;
} AMQP_BINARY_VALUE;

typedef struct DESCRIBED_VALUE_TAG
{
	AMQP_VALUE descriptor;
	AMQP_VALUE value;
} DESCRIBED_VALUE;

typedef union AMQP_VALUE_UNION_TAG
{
	DESCRIBED_VALUE described_value;
	unsigned char ubyte_value;
	uint16_t ushort_value;
	uint32_t uint_value;
	uint64_t ulong_value;
	char byte_value;
	int16_t short_value;
	int32_t int_value;
	int64_t long_value;
	bool bool_value;
	float float_value;
	double double_value;
	uint32_t char_value;
	uint64_t timestamp_value;
	amqp_uuid uuid_value;
	AMQP_STRING_VALUE string_value;
	amqp_binary binary_value;
	AMQP_LIST_VALUE list_value;
	AMQP_MAP_VALUE map_value;
	uint32_t symbol_value;
} AMQP_VALUE_UNION;

typedef enum DECODE_LIST_STEP_TAG
{
	DECODE_LIST_STEP_SIZE,
	DECODE_LIST_STEP_COUNT,
	DECODE_LIST_STEP_ITEMS
} DECODE_LIST_STEP;

typedef enum DECODE_DESCRIBED_VALUE_STEP_TAG
{
	DECODE_DESCRIBED_VALUE_STEP_DESCRIPTOR,
	DECODE_DESCRIBED_VALUE_STEP_VALUE
} DECODE_DESCRIBED_VALUE_STEP;

typedef struct DECODE_LIST_VALUE_STATE_TAG
{
	DECODE_LIST_STEP list_value_state;
	uint32_t item;
} DECODE_LIST_VALUE_STATE;

typedef struct DECODE_DESCRIBED_VALUE_STATE_TAG
{
	DECODE_DESCRIBED_VALUE_STEP described_value_state;
} DECODE_DESCRIBED_VALUE_STATE;

typedef struct STRING_VALUE_STATE_TAG
{
	uint32_t length;
} STRING_VALUE_STATE;

typedef union DECODE_VALUE_STATE_UNION_TAG
{
	DECODE_LIST_VALUE_STATE list_value_state;
	DECODE_DESCRIBED_VALUE_STATE described_value_state;
	STRING_VALUE_STATE string_value_state;
} DECODE_VALUE_STATE_UNION;

typedef struct AMQP_VALUE_DATA_TAG
{
	AMQP_TYPE type;
	AMQP_VALUE_UNION value;
} AMQP_VALUE_DATA;

typedef enum DECODER_STATE_TAG
{
	DECODER_STATE_CONSTRUCTOR,
	DECODER_STATE_TYPE_DATA,
	DECODER_STATE_DONE,
	DECODER_STATE_ERROR
} DECODER_STATE;

typedef struct INTERNAL_DECODER_DATA_TAG
{
	VALUE_DECODED_CALLBACK value_decoded_callback;
	void* value_decoded_callback_context;
	size_t bytes_decoded;
	DECODER_STATE decoder_state;
	uint8_t constructor_byte;
	AMQP_VALUE_DATA* decode_to_value;
	void* inner_decoder;
	DECODE_VALUE_STATE_UNION decode_value_state;
} INTERNAL_DECODER_DATA;

typedef struct DECODER_DATA_TAG
{
	INTERNAL_DECODER_DATA* internal_decoder;
	AMQP_VALUE_DATA* decode_to_value;
} DECODER_DATA;

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

/* Codes_SRS_AMQPVALUE_01_019: [1.6.11 float 32-bit floating point number (IEEE 754-2008 binary32).]  */
AMQP_VALUE amqpvalue_create_float(float value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_081: [If allocating the AMQP_VALUE fails then amqpvalue_create_float shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_080: [amqpvalue_create_float shall return a handle to an AMQP_VALUE that stores a float value.] */
		result->type = AMQP_TYPE_FLOAT;
		result->value.float_value = value;
	}
	return result;
}

int amqpvalue_get_float(AMQP_VALUE value, float* float_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_084: [If any of the arguments is NULL then amqpvalue_get_float shall return a non-zero value.] */
	if ((value == NULL) ||
		(float_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_085: [If the type of the value is not float (was not created with amqpvalue_create_float), then amqpvalue_get_float shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_FLOAT)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_082: [amqpvalue_get_float shall fill in the float_value argument the float value stored by the AMQP value indicated by the value argument.] */
			*float_value = value_data->value.float_value;

			/* Codes_SRS_AMQPVALUE_01_083: [On success amqpvalue_get_float shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_020: [1.6.12 double 64-bit floating point number (IEEE 754-2008 binary64).] */
AMQP_VALUE amqpvalue_create_double(double value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_087: [If allocating the AMQP_VALUE fails then amqpvalue_create_double shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_086: [amqpvalue_create_double shall return a handle to an AMQP_VALUE that stores a double value.] */
		result->type = AMQP_TYPE_DOUBLE;
		result->value.double_value = value;
	}
	return result;
}

int amqpvalue_get_double(AMQP_VALUE value, double* double_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_090: [If any of the arguments is NULL then amqpvalue_get_double shall return a non-zero value.] */
	if ((value == NULL) ||
		(double_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_091: [If the type of the value is not double (was not created with amqpvalue_create_double), then amqpvalue_get_double shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_DOUBLE)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_088: [amqpvalue_get_double shall fill in the double_value argument the double value stored by the AMQP value indicated by the value argument.] */
			*double_value = value_data->value.double_value;

			/* Codes_SRS_AMQPVALUE_01_089: [On success amqpvalue_get_double shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_024: [1.6.16 char A single Unicode character.] */
AMQP_VALUE amqpvalue_create_char(uint32_t value)
{
	AMQP_VALUE_DATA* result;

	/* Codes_SRS_AMQPVALUE_01_098: [If the code point value is outside of the allowed range [0, 0x10FFFF] then amqpvalue_create_char shall return NULL.] */
	if (value > 0x10FFFF)
	{
		result = NULL;
	}
	else
	{
		result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
		/* Codes_SRS_AMQPVALUE_01_093: [If allocating the AMQP_VALUE fails then amqpvalue_create_char shall return NULL.] */
		if (result != NULL)
		{
			/* Codes_SRS_AMQPVALUE_01_092: [amqpvalue_create_char shall return a handle to an AMQP_VALUE that stores a single UTF-32 character value.] */
			result->type = AMQP_TYPE_CHAR;
			result->value.char_value = value;
		}
	}
	return result;
}

int amqpvalue_get_char(AMQP_VALUE value, uint32_t* char_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_096: [If any of the arguments is NULL then amqpvalue_get_char shall return a non-zero value.] */
	if ((value == NULL) ||
		(char_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_097: [If the type of the value is not char (was not created with amqpvalue_create_char), then amqpvalue_get_char shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_CHAR)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_094: [amqpvalue_get_char shall fill in the char_value argument the UTF32 char value stored by the AMQP value indicated by the value argument.] */
			*char_value = value_data->value.char_value;

			/* Codes_SRS_AMQPVALUE_01_095: [On success amqpvalue_get_char shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_025: [1.6.17 timestamp An absolute point in time.] */
AMQP_VALUE amqpvalue_create_timestamp(uint64_t value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_108: [If allocating the AMQP_VALUE fails then amqpvalue_create_timestamp shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_107: [amqpvalue_create_timestamp shall return a handle to an AMQP_VALUE that stores an uint64_t value that represents a millisecond precision Unix time.] */
		result->type = AMQP_TYPE_TIMESTAMP;
		result->value.timestamp_value = value;
	}
	return result;
}

int amqpvalue_get_timestamp(AMQP_VALUE value, uint64_t* timestamp_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_111: [If any of the arguments is NULL then amqpvalue_get_timestamp shall return a non-zero value.] */
	if ((value == NULL) ||
		(timestamp_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_112: [If the type of the value is not timestamp (was not created with amqpvalue_create_timestamp), then amqpvalue_get_timestamp shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_TIMESTAMP)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_109: [amqpvalue_get_timestamp shall fill in the timestamp_value argument the timestamp value stored by the AMQP value indicated by the value argument.] */
			*timestamp_value = value_data->value.timestamp_value;

			/* Codes_SRS_AMQPVALUE_01_110: [On success amqpvalue_get_timestamp shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_026: [1.6.18 uuid A universally unique identifier as defined by RFC-4122 section 4.1.2 .] */
AMQP_VALUE amqpvalue_create_uuid(amqp_uuid value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	/* Codes_SRS_AMQPVALUE_01_114: [If allocating the AMQP_VALUE fails then amqpvalue_create_uuid shall return NULL.] */
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_113: [amqpvalue_create_uuid shall return a handle to an AMQP_VALUE that stores an amqp_uuid value that represents a unique identifier per RFC-4122 section 4.1.2.] */
		result->type = AMQP_TYPE_UUID;
		if (memcpy(&result->value.uuid_value, value, sizeof(amqp_uuid)) == NULL)
		{
			free(result);
			result = NULL;
		}
	}
	return result;
}

int amqpvalue_get_uuid(AMQP_VALUE value, amqp_uuid* uuid_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_117: [If any of the arguments is NULL then amqpvalue_get_uuid shall return a non-zero value.] */
	if ((value == NULL) ||
		(uuid_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_118: [If the type of the value is not uuid (was not created with amqpvalue_create_uuid), then amqpvalue_get_uuid shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_UUID)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_115: [amqpvalue_get_uuid shall fill in the uuid_value argument the uuid value stored by the AMQP value indicated by the value argument.] */
			if (memcpy(*uuid_value, value_data->value.uuid_value, sizeof(amqp_uuid)) == NULL)
			{
				result = __LINE__;
			}
			else
			{
				/* Codes_SRS_AMQPVALUE_01_116: [On success amqpvalue_get_uuid shall return 0.] */
				result = 0;
			}
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_027: [1.6.19 binary A sequence of octets.] */
AMQP_VALUE amqpvalue_create_binary(amqp_binary value)
{
	AMQP_VALUE_DATA* result;
	if ((value.bytes == NULL) &&
		(value.length > 0))
	{
		/* Codes_SRS_AMQPVALUE_01_129: [If value.data is NULL and value.length is positive then amqpvalue_create_binary shall return NULL.] */
		result = NULL;
	}
	else
	{
		/* Codes_SRS_AMQPVALUE_01_128: [If allocating the AMQP_VALUE fails then amqpvalue_create_binary shall return NULL.] */
		result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
		if (result != NULL)
		{
			/* Codes_SRS_AMQPVALUE_01_127: [amqpvalue_create_binary shall return a handle to an AMQP_VALUE that stores a sequence of bytes.] */
			result->type = AMQP_TYPE_BINARY;
			if (value.length > 0)
			{
				result->value.binary_value.bytes = amqpalloc_malloc(value.length);
			}
			else
			{
				result->value.binary_value.bytes = NULL;
			}

			result->value.binary_value.length = value.length;

			if ((result->value.binary_value.bytes == NULL) && (value.length > 0))
			{
				/* Codes_SRS_AMQPVALUE_01_128: [If allocating the AMQP_VALUE fails then amqpvalue_create_binary shall return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if ((value.length > 0) && (memcpy(result->value.binary_value.bytes, value.bytes, value.length) == NULL))
				{
					/* Codes_SRS_AMQPVALUE_01_130: [If any other error occurs, amqpvalue_create_binary shall return NULL.] */
					amqpalloc_free(result->value.binary_value.bytes);
					amqpalloc_free(result);
					result = NULL;
				}
			}
		}
	}
	return result;
}

int amqpvalue_get_binary(AMQP_VALUE value, amqp_binary* binary_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_132: [If any of the arguments is NULL then amqpvalue_get_binary shall return NULL.] */
	if ((value == NULL) ||
		(binary_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		/* Codes_SRS_AMQPVALUE_01_133: [If the type of the value is not binary (was not created with amqpvalue_create_binary), then amqpvalue_get_binary shall return NULL.] */
		if (value_data->type != AMQP_TYPE_BINARY)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_131: [amqpvalue_get_binary shall yield a pointer to the sequence of bytes held by the AMQP_VALUE in binary_value.data and fill in the binary_value.length argument the number of bytes held in the binary value.] */
			binary_value->length = value_data->value.binary_value.length;
			binary_value->bytes = value_data->value.binary_value.bytes;

			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_135: [amqpvalue_create_string shall return a handle to an AMQP_VALUE that stores a sequence of Unicode characters.] */
/* Codes_SRS_AMQPVALUE_01_028: [1.6.20 string A sequence of Unicode characters.] */
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
		
		/* Codes_SRS_AMQPVALUE_01_136: [If allocating the AMQP_VALUE fails then amqpvalue_create_string shall return NULL.] */
		result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
		if (result != NULL)
		{
			result->type = AMQP_TYPE_STRING;
			result->value.string_value.chars = amqpalloc_malloc(length + 1);
			if (result->value.string_value.chars == NULL)
			{
				/* Codes_SRS_AMQPVALUE_01_136: [If allocating the AMQP_VALUE fails then amqpvalue_create_string shall return NULL.] */
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (strcpy(result->value.string_value.chars, value) == NULL)
				{
					/* Codes_SRS_AMQPVALUE_01_137: [If any other error occurs, amqpvalue_create_string shall return NULL.] */
					amqpalloc_free(result->value.string_value.chars);
					amqpalloc_free(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}

int amqpvalue_get_string(AMQP_VALUE value, const char** string_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_139: [If any of the arguments is NULL then amqpvalue_get_string shall return a non-zero value.] */
	if ((value == NULL) ||
		(string_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;

		/* Codes_SRS_AMQPVALUE_01_140: [If the type of the value is not string (was not created with amqpvalue_create_string), then amqpvalue_get_string shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_STRING)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_138: [amqpvalue_get_string shall yield a pointer to the sequence of bytes held by the AMQP_VALUE in string_value.] */
			*string_value = value_data->value.string_value.chars;

			/* Codes_SRS_AMQPVALUE_01_141: [On success, amqpvalue_get_string shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_029: [1.6.21 symbol Symbolic values from a constrained domain.] */
AMQP_VALUE amqpvalue_create_symbol(uint32_t value)
{
	/* Codes_SRS_AMQPVALUE_01_143: [If allocating the AMQP_VALUE fails then amqpvalue_create_symbol shall return NULL.] */
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_142: [amqpvalue_create_symbol shall return a handle to an AMQP_VALUE that stores a uint32_t value.] */
		result->type = AMQP_TYPE_SYMBOL;
		result->value.symbol_value = value;
	}

	return result;
}

int amqpvalue_get_symbol(AMQP_VALUE value, uint32_t* symbol_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_147: [If any of the arguments is NULL then amqpvalue_get_symbol shall return a non-zero value.] */
	if ((value == NULL) ||
		(symbol_value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;

		/* Codes_SRS_AMQPVALUE_01_148: [If the type of the value is not symbol (was not created with amqpvalue_create_symbol), then amqpvalue_get_symbol shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_SYMBOL)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_145: [amqpvalue_get_symbol shall fill in the symbol_value the uint32_t symbol value held by the AMQP_VALUE.] */
			*symbol_value = value_data->value.symbol_value;

			/* Codes_SRS_AMQPVALUE_01_146: [On success, amqpvalue_get_symbol shall return 0.] */
			result = 0;
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_030: [1.6.22 list A sequence of polymorphic values.] */
AMQP_VALUE amqpvalue_create_list(void)
{
	/* Codes_SRS_AMQPVALUE_01_150: [If allocating the AMQP_VALUE fails then amqpvalue_create_list shall return NULL.] */
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_149: [amqpvalue_create_list shall return a handle to an AMQP_VALUE that stores a list.] */
		result->type = AMQP_TYPE_LIST;

		/* Codes_SRS_AMQPVALUE_01_151: [The list shall have an initial size of zero.] */
		result->value.list_value.count = 0;
		result->value.list_value.items = NULL;
	}

	return result;
}

int amqpvalue_set_list_item_count(AMQP_VALUE value, uint32_t list_size)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_155: [If the value argument is NULL, amqpvalue_set_list_item_count shall return a non-zero value.] */
	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;

		if (value_data->type != AMQP_TYPE_LIST)
		{
			/* Codes_SRS_AMQPVALUE_01_156: [If the value is not of type list, then amqpvalue_set_list_item_count shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			if (value_data->value.list_value.count < list_size)
			{
				AMQP_VALUE* new_list;

				/* Codes_SRS_AMQPVALUE_01_152: [amqpvalue_set_list_item_count shall resize an AMQP list.] */
				new_list = (AMQP_VALUE*)amqpalloc_realloc(value_data->value.list_value.items, list_size * sizeof(AMQP_VALUE));
				if (new_list == NULL)
				{
					/* Codes_SRS_AMQPVALUE_01_154: [If allocating memory for the list according to the new size fails, then amqpvalue_set_list_item_count shall return a non-zero value, while preserving the existing list contents.] */
					result = __LINE__;
				}
				else
				{
					value_data->value.list_value.items = new_list;

					/* Codes_SRS_AMQPVALUE_01_162: [When a list is grown a null AMQP_VALUE shall be inserted as new list items to fill the list up to the new size.] */
					uint32_t i;

					/* Codes_SRS_AMQPVALUE_01_162: [When a list is grown a null AMQP_VALUE shall be inserted as new list items to fill the list up to the new size.] */
					for (i = value_data->value.list_value.count; i < list_size; i++)
					{
						new_list[i] = amqpvalue_create_null();
						if (new_list[i] == NULL)
						{
							break;
						}
					}

					if (i < list_size)
					{
						/* Codes_SRS_AMQPVALUE_01_154: [If allocating memory for the list according to the new size fails, then amqpvalue_set_list_item_count shall return a non-zero value, while preserving the existing list contents.] */
						uint32_t j;
						for (j = value_data->value.list_value.count; j < i; j++)
						{
							amqpvalue_destroy(new_list[j]);
						}

						result = __LINE__;
					}
					else
					{
						value_data->value.list_value.count = list_size;

						/* Codes_SRS_AMQPVALUE_01_153: [On success amqpvalue_set_list_item_count shall return 0.] */
						result = 0;
					}
				}
			}
			else if (value_data->value.list_value.count > list_size)
			{
				uint32_t i;

				/* Codes_SRS_AMQPVALUE_01_161: [When the list is shrunk, the extra items shall be freed by using amqp_value_destroy.] */
				for (i = list_size; i < value_data->value.list_value.count; i++)
				{
					amqpvalue_destroy(value_data->value.list_value.items[i]);
				}

				value_data->value.list_value.count = list_size;

				/* Codes_SRS_AMQPVALUE_01_153: [On success amqpvalue_set_list_item_count shall return 0.] */
				result = 0;
			}
			else
			{
				/* Codes_SRS_AMQPVALUE_01_153: [On success amqpvalue_set_list_item_count shall return 0.] */
				result = 0;
			}
		}
	}
	
	return result;
}

int amqpvalue_get_list_item_count(AMQP_VALUE value, size_t* size)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_159: [If any of the arguments are NULL, amqpvalue_get_list_item_count shall return a non-zero value.] */
	if ((value == NULL) ||
		(size == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;

		/* Codes_SRS_AMQPVALUE_01_160: [If the AMQP_VALUE is not a list then amqpvalue_get_list_item_count shall return a non-zero value.] */
		if (value_data->type != AMQP_TYPE_LIST)
		{
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_157: [amqpvalue_get_list_item_count shall fill in the size argument the number of items held by the AMQP list.] */
			*size = value_data->value.list_value.count;

			/* Codes_SRS_AMQPVALUE_01_158: [On success amqpvalue_get_list_item_count shall return 0.] */
			result = 0;
		}
	}

	return result;
}

int amqpvalue_set_list_item(AMQP_VALUE value, uint32_t index, AMQP_VALUE list_item_value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_165: [If value or list_item_value is NULL, amqpvalue_set_list_item shall fail and return a non-zero value.] */
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
			/* Codes_SRS_AMQPVALUE_01_168: [The item stored at the index-th position in the list shall be a clone of list_item_value.] */
			AMQP_VALUE cloned_item = amqpvalue_clone(list_item_value);

			if (cloned_item == NULL)
			{
				/* Codes_SRS_AMQPVALUE_01_170: [When amqpvalue_set_list_item fails due to not being able to clone the item or grow the list, the list shall not be altered.] */
				/* Codes_SRS_AMQPVALUE_01_169: [If cloning the item fails, amqpvalue_set_list_item shall fail and return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				if (index >= value_data->value.list_value.count)
				{
					AMQP_VALUE* new_list = (AMQP_VALUE*)amqpalloc_realloc(value_data->value.list_value.items, (index + 1) * sizeof(AMQP_VALUE));
					if (new_list == NULL)
					{
						/* Codes_SRS_AMQPVALUE_01_170: [When amqpvalue_set_list_item fails due to not being able to clone the item or grow the list, the list shall not be altered.] */
						amqpvalue_destroy(cloned_item);
						result = __LINE__;
					}
					else
					{
						uint32_t i;

						value_data->value.list_value.items = new_list;

						for (i = value_data->value.list_value.count; i < index; i++)
						{
							new_list[i] = amqpvalue_create_null();
							if (new_list[i] == NULL)
							{
								break;
							}
						}

						if (i < index)
						{
							/* Codes_SRS_AMQPVALUE_01_170: [When amqpvalue_set_list_item fails due to not being able to clone the item or grow the list, the list shall not be altered.] */
							uint32_t j;

							for (j = value_data->value.list_value.count; j < i; j++)
							{
								amqpvalue_destroy(new_list[j]);
							}

							amqpvalue_destroy(cloned_item);

							/* Codes_SRS_AMQPVALUE_01_172: [If growing the list fails, then amqpvalue_set_list_item shall fail and return a non-zero value.] */
							result = __LINE__;
						}
						else
						{
							value_data->value.list_value.count = index + 1;
							value_data->value.list_value.items[index] = cloned_item;

							/* Codes_SRS_AMQPVALUE_01_164: [On success amqpvalue_set_list_item shall return 0.] */
							result = 0;
						}
					}
				}
				else
				{
					/* Codes_SRS_AMQPVALUE_01_167: [Any previous value stored at the position index in the list shall be freed by using amqpvalue_destroy.] */
					amqpvalue_destroy(value_data->value.list_value.items[index]);

					/* Codes_SRS_AMQPVALUE_01_163: [amqpvalue_set_list_item shall replace the item at the 0 based index-th position in the list identified by the value argument with the AMQP_VALUE specified by list_item_value.] */
					value_data->value.list_value.items[index] = cloned_item;

					/* Codes_SRS_AMQPVALUE_01_164: [On success amqpvalue_set_list_item shall return 0.] */
					result = 0;
				}
			}
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_list_item(AMQP_VALUE value, size_t index)
{
	AMQP_VALUE result;

	if (value == NULL)
	{
		/* Codes_SRS_AMQPVALUE_01_174: [If the value argument is NULL, amqpvalue_get_list_item shall fail and return NULL.] */
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;

		/* Codes_SRS_AMQPVALUE_01_177: [If value is not a list then amqpvalue_get_list_item shall fail and return NULL.] */
		if ((value_data->type != AMQP_TYPE_LIST) ||
			/* Codes_SRS_AMQPVALUE_01_175: [If index is greater or equal to the number of items in the list then amqpvalue_get_list_item shall fail and return NULL.] */
			(value_data->value.list_value.count <= index))
		{
			result = NULL;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_173: [amqpvalue_get_list_item shall return a copy of the AMQP_VALUE stored at the 0 based position index in the list identified by value.] */
			/* Codes_SRS_AMQPVALUE_01_176: [If cloning the item at position index fails, then amqpvalue_get_list_item shall fail and return NULL.] */
			result = amqpvalue_clone(value_data->value.list_value.items[index]);
		}
	}

	return result;
}

/* Codes_SRS_AMQPVALUE_01_178: [amqpvalue_create_map shall create an AMQP value that holds a map and return a handle to it.] */
/* Codes_SRS_AMQPVALUE_01_031: [1.6.23 map A polymorphic mapping from distinct keys to values.] */
AMQP_VALUE amqpvalue_create_map(void)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));

	/* Codes_SRS_AMQPVALUE_01_179: [If allocating memory for the map fails, then amqpvalue_create_map shall return NULL.] */
	if (result != NULL)
	{
		result->type = AMQP_TYPE_MAP;

		/* Codes_SRS_AMQPVALUE_01_180: [The number of key/value pairs in the newly created map shall be zero.] */
		result->value.map_value.pairs = NULL;
		result->value.map_value.pair_count = 0;
	}

	return result;
}

int amqpvalue_set_map_value(AMQP_VALUE map, AMQP_VALUE key, AMQP_VALUE value)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_183: [If any of the arguments are NULL, amqpvalue_set_map_value shall fail and return a non-zero value.] */
	if ((map == NULL) ||
		(key == NULL) ||
		(value == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)map;
		AMQP_VALUE cloned_value;

		/* Codes_SRS_AMQPVALUE_01_185: [When storing the key or value, their contents shall be cloned.] */
		cloned_value = amqpvalue_clone(value);
		if (cloned_value == NULL)
		{
			/* Codes_SRS_AMQPVALUE_01_188: [If cloning the value fails, amqpvalue_set_map_value shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			uint32_t i;
			AMQP_VALUE cloned_key;

			for (i = 0; i < value_data->value.map_value.pair_count; i++)
			{
				if (amqpvalue_are_equal(value_data->value.map_value.pairs[i].key, key))
				{
					break;
				}
			}

			if (i < value_data->value.map_value.pair_count)
			{
				/* Codes_SRS_AMQPVALUE_01_184: [If the key already exists in the map, its value shall be replaced with the value provided by the value argument.] */
				amqpvalue_destroy(value_data->value.map_value.pairs[i].value);
				value_data->value.map_value.pairs[i].value = cloned_value;

				/* Codes_SRS_AMQPVALUE_01_182: [On success amqpvalue_set_map_value shall return 0.] */
				result = 0;
			}
			else
			{
				/* Codes_SRS_AMQPVALUE_01_185: [When storing the key or value, their contents shall be cloned.] */
				cloned_key = amqpvalue_clone(key);
				if (cloned_key == NULL)
				{
					/* Codes_SRS_AMQPVALUE_01_187: [If cloning the key fails, amqpvalue_set_map_value shall fail and return a non-zero value.] */
					amqpvalue_destroy(cloned_value);
					result = __LINE__;
				}
				else
				{
					AMQP_MAP_KEY_VALUE_PAIR* new_pairs = (AMQP_MAP_KEY_VALUE_PAIR*)amqpalloc_realloc(value_data->value.map_value.pairs, (value_data->value.map_value.pair_count + 1) * sizeof(AMQP_MAP_KEY_VALUE_PAIR));
					if (new_pairs == NULL)
					{
						/* Codes_SRS_AMQPVALUE_01_186: [If allocating memory to hold a new key/value pair fails, amqpvalue_set_map_value shall fail and return a non-zero value.] */
						amqpvalue_destroy(cloned_key);
						amqpvalue_destroy(cloned_value);
						result = __LINE__;
					}
					else
					{
						value_data->value.map_value.pairs = new_pairs;

						/* Codes_SRS_AMQPVALUE_01_181: [amqpvalue_set_map_value shall set the value in the map identified by the map argument for a key/value pair identified by the key argument.] */
						value_data->value.map_value.pairs[value_data->value.map_value.pair_count].key = cloned_key;
						value_data->value.map_value.pairs[value_data->value.map_value.pair_count].value = cloned_value;
						value_data->value.map_value.pair_count++;

						/* Codes_SRS_AMQPVALUE_01_182: [On success amqpvalue_set_map_value shall return 0.] */
						result = 0;
					}
				}
			}
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_map_value(AMQP_VALUE map, AMQP_VALUE key)
{
	AMQP_VALUE result;

	/* Codes_SRS_AMQPVALUE_01_190: [If any argument is NULL, amqpvalue_get_map_value shall return NULL.] */
	if ((map == NULL) ||
		(key == NULL))
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)map;
		uint32_t i;

		for (i = 0; i < value_data->value.map_value.pair_count; i++)
		{
			if (amqpvalue_are_equal(value_data->value.map_value.pairs[i].key, key))
			{
				break;
			}
		}

		if (i == value_data->value.map_value.pair_count)
		{
			/* Codes_SRS_AMQPVALUE_01_191: [If the key cannot be found, amqpvalue_get_map_value shall return NULL.] */
			result = NULL;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_189: [amqpvalue_get_map_value shall return the value whose key is identified by the key argument.] */
			/* Codes_SRS_AMQPVALUE_01_192: [The returned value shall be a clone of the actual value stored in the map.] */
			result = amqpvalue_clone(value_data->value.map_value.pairs[i].value);
		}
	}

	return result;
}

int amqpvalue_get_map_pair_count(AMQP_VALUE map, uint32_t* pair_count)
{
	int result;

	/* Codes_SRS_AMQPVALUE_01_195: [If any of the arguments is NULL, amqpvalue_get_map_pair_count shall fail and return a non-zero value.] */
	if ((map == NULL) ||
		(pair_count == NULL))
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)map;

		if (value_data->type != AMQP_TYPE_MAP)
		{
			/* Tests_SRS_AMQPVALUE_01_198: [If the map argument is not an AMQP value created with the amqpvalue_create_map function than amqpvalue_get_map_pair_count shall fail and return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_AMQPVALUE_01_193: [amqpvalue_get_map_pair_count shall fill in the number of key/value pairs in the map in the pair_count argument.] */
			*pair_count = value_data->value.map_value.pair_count;

			/* Codes_SRS_AMQPVALUE_01_194: [On success amqpvalue_get_map_pair_count shall return 0.] */
			result = 0;
		}
	}

	return result;
}

bool amqpvalue_are_equal(AMQP_VALUE value1, AMQP_VALUE value2)
{
	bool result;
	if ((value1 == NULL) &&
		(value2 == NULL))
	{
		result = true;
	}
	else if ((value1 != value2) && ((value1 == NULL) || (value2 == NULL)))
	{
		result = false;
	}
	else
	{
		AMQP_VALUE_DATA* value1_data = (AMQP_VALUE_DATA*)value1;
		AMQP_VALUE_DATA* value2_data = (AMQP_VALUE_DATA*)value2;

		switch (value1_data->type)
		{
		case AMQP_TYPE_UINT:
			result = (value1_data->value.uint_value == value2_data->value.uint_value);
			break;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_create_described(AMQP_VALUE descriptor, AMQP_VALUE value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_DESCRIBED;
		result->value.described_value.descriptor = descriptor;
		result->value.described_value.value = value;
	}
	return result;
}

AMQP_VALUE amqpvalue_create_composite(AMQP_VALUE descriptor, uint32_t list_size)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_COMPOSITE;
		result->value.described_value.descriptor = amqpvalue_clone(descriptor);
		if (result->value.described_value.descriptor == NULL)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->value.described_value.value = amqpvalue_create_list();
			if (result->value.described_value.value == NULL)
			{
				amqpvalue_destroy(result->value.described_value.descriptor);
				free(result);
				result = NULL;
			}
			else
			{
				if (amqpvalue_set_list_item_count(result->value.described_value.value, list_size) != 0)
				{
					amqpvalue_destroy(result->value.described_value.descriptor);
					amqpvalue_destroy(result->value.described_value.value);
					free(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_create_composite_with_ulong_descriptor(uint64_t descriptor)
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
			result->value.described_value.descriptor = descriptor_ulong_value;
			if (result->value.described_value.descriptor == NULL)
			{
				amqpalloc_free(descriptor_ulong_value);
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				result->value.described_value.value = amqpvalue_create_list();
				if (result->value.described_value.value == NULL)
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
		}
	}

	return result;
}

static void amqpvalue_clear(AMQP_VALUE_DATA* value_data)
{
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
		if (value_data->value.binary_value.bytes != NULL)
		{
			amqpalloc_free(value_data->value.binary_value.bytes);
		}
		break;
	case AMQP_TYPE_STRING:
		if (value_data->value.string_value.chars != NULL)
		{
			amqpalloc_free(value_data->value.string_value.chars);
		}
		break;
	case AMQP_TYPE_COMPOSITE:
	case AMQP_TYPE_DESCRIBED:
		amqpvalue_destroy(value_data->value.described_value.descriptor);
		amqpvalue_destroy(value_data->value.described_value.value);
		break;
	}

	value_data->type = AMQP_TYPE_UNKNOWN;
}

void amqpvalue_destroy(AMQP_VALUE value)
{
	if (value != NULL)
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		amqpvalue_clear(value_data);
		amqpalloc_free(value);
	}
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
		if ((value_data->type != AMQP_TYPE_DESCRIBED) &&
			(value_data->type != AMQP_TYPE_COMPOSITE))
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.described_value.descriptor;
		}
	}

	return result;
}

AMQP_VALUE amqpvalue_get_described_value(AMQP_VALUE value)
{
	AMQP_VALUE result;

	if (value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_DESCRIBED)
		{
			result = NULL;
		}
		else
		{
			result = value_data->value.described_value.value;
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

		case AMQP_TYPE_DESCRIBED:
			result = amqpvalue_create_described(value_data->value.described_value.descriptor, value_data->value.described_value.value);
			break;

		case AMQP_TYPE_COMPOSITE:
		{
			AMQP_VALUE_DATA* result_data = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
			AMQP_VALUE cloned_descriptor;
			AMQP_VALUE cloned_list;

			if (result_data == NULL)
			{
				result = NULL;
			}
			else if ((cloned_descriptor = amqpvalue_clone(value_data->value.described_value.descriptor)) == NULL)
			{
				free(result_data);
				result = NULL;
			}
			else if ((cloned_list = amqpvalue_clone(value_data->value.described_value.value)) == NULL)
			{
				amqpvalue_destroy(cloned_descriptor);
				free(result_data);
				result = NULL;
			}
			else
			{
				result_data->value.described_value.descriptor = cloned_descriptor;
				result_data->value.described_value.value = cloned_list;
				result_data->type = AMQP_TYPE_COMPOSITE;

				result = (AMQP_VALUE)result_data;
			}
			break;
		}

		case AMQP_TYPE_NULL:
			result = amqpvalue_create_null();
			break;

		case AMQP_TYPE_LIST:
		{
			uint32_t list_size;
			if (amqpvalue_get_list_item_count(value, &list_size) != 0)
			{
				result = NULL;
			}
			else
			{
				uint32_t i;
				result = amqpvalue_create_list();
				for (i = 0; i < list_size; i++)
				{
					amqpvalue_set_list_item(result, i, value_data->value.list_value.items[i]);
				}

				if (i < list_size)
				{
					amqpvalue_destroy(result);
					result = NULL;
				}
			}
			break;
		}

		case AMQP_TYPE_STRING:
			result = amqpvalue_create_string(value_data->value.string_value.chars);
			break;

		case AMQP_TYPE_ULONG:
			result = amqpvalue_create_ulong(value_data->value.ulong_value);
			break;

		case AMQP_TYPE_UINT:
			result = amqpvalue_create_uint(value_data->value.uint_value);
			break;

		case AMQP_TYPE_USHORT:
			result = amqpvalue_create_ushort(value_data->value.ushort_value);
			break;

		case AMQP_TYPE_BOOL:
			result = amqpvalue_create_boolean(value_data->value.bool_value);
			break;

		case AMQP_TYPE_UBYTE:
			result = amqpvalue_create_ubyte(value_data->value.ubyte_value);
			break;

		case AMQP_TYPE_BINARY:
			result = amqpvalue_create_binary(value_data->value.binary_value);
			break;
		}
	}

	return result;
}

static INTERNAL_DECODER_DATA* internal_decoder_create(VALUE_DECODED_CALLBACK value_decoded_callback, void* value_decoded_callback_context, AMQP_VALUE_DATA* value_data)
{
	INTERNAL_DECODER_DATA* internal_decoder_data = (INTERNAL_DECODER_DATA*)amqpalloc_malloc(sizeof(INTERNAL_DECODER_DATA));
	if (internal_decoder_data != NULL)
	{
		internal_decoder_data->value_decoded_callback = value_decoded_callback;
		internal_decoder_data->value_decoded_callback_context = value_decoded_callback_context;
		internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
		internal_decoder_data->inner_decoder = NULL;
		internal_decoder_data->decode_to_value = value_data;
	}

	return internal_decoder_data;
}

static void internal_decoder_destroy(INTERNAL_DECODER_DATA* internal_decoder)
{
	if (internal_decoder != NULL)
	{
		internal_decoder_destroy(internal_decoder->inner_decoder);
		amqpalloc_free(internal_decoder);
	}
}

static int inner_decoder_callback(void* context, AMQP_VALUE decoded_value)
{
	INTERNAL_DECODER_DATA* internal_decoder_data = (INTERNAL_DECODER_DATA*)context;
	INTERNAL_DECODER_DATA* inner_decoder = (INTERNAL_DECODER_DATA*)internal_decoder_data->inner_decoder;
	inner_decoder->decoder_state = DECODER_STATE_DONE;
	return 0;
}

int internal_decoder_decode_bytes(INTERNAL_DECODER_DATA* internal_decoder_data, const unsigned char* buffer, size_t size, size_t* used_bytes)
{
	int result;
	size_t initial_size = size;

	if (internal_decoder_data == NULL)
	{
		result = __LINE__;
	}
	else
	{
		while ((size > 0) && (internal_decoder_data->decoder_state != DECODER_STATE_DONE))
		{
			switch (internal_decoder_data->decoder_state)
			{
			default:
				break;
			case DECODER_STATE_CONSTRUCTOR:
			{
				amqpvalue_clear(internal_decoder_data->decode_to_value);
				internal_decoder_data->constructor_byte = buffer[0];
				buffer++;
				size--;
				switch (internal_decoder_data->constructor_byte)
				{
				default:
					internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
					result = __LINE__;
					break;
				case 0x00: /* descriptor */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_DESCRIBED;
					AMQP_VALUE_DATA* descriptor = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
					if (descriptor == NULL)
					{
						internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
						result = __LINE__;
					}
					else
					{
						descriptor->type = AMQP_TYPE_UNKNOWN;
						internal_decoder_data->decode_to_value->value.described_value.descriptor = descriptor;
						internal_decoder_data->inner_decoder = internal_decoder_create(inner_decoder_callback, internal_decoder_data, descriptor);
						if (internal_decoder_data->inner_decoder == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
							internal_decoder_data->decode_value_state.described_value_state.described_value_state = DECODE_DESCRIBED_VALUE_STEP_DESCRIPTOR;
							result = 0;
						}
					}

					break;
				case 0x40:
				{
					/* null */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_NULL;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x41:
				{
					/* true */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_BOOL;
					internal_decoder_data->decode_to_value->value.bool_value = true;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x42:
				{
					/* false */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_BOOL;
					internal_decoder_data->decode_to_value->value.bool_value = false;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x43:
				{
					/* uint0 */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_UINT;
					internal_decoder_data->decode_to_value->value.uint_value = 0;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x44:
				{
					/* ulong0 */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_ULONG;
					internal_decoder_data->decode_to_value->value.ulong_value = 0;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x50: /* ubyte */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_UBYTE;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.ubyte_value = 0;
					result = 0;
					break;
				}
				case 0x53: /* smallulong */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_ULONG;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.ulong_value = 0;
					internal_decoder_data->bytes_decoded = 0;
					result = 0;
					break;
				}
				case 0x60: /* ushort */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_USHORT;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.ushort_value = 0;
					internal_decoder_data->bytes_decoded = 0;
					result = 0;
					break;
				}
				case 0x52: /* smalluint */
				case 0x70: /* uint */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_UINT;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.uint_value = 0;
					internal_decoder_data->bytes_decoded = 0;
					result = 0;
					break;
				}
				case 0x80: /* ulong */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_ULONG;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.ulong_value = 0;
					internal_decoder_data->bytes_decoded = 0;
					result = 0;
					break;
				}
				case 0xA0: /* vbin8 */
				case 0xB0: /* vbin32 */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_BINARY;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.binary_value.length = 0;
					internal_decoder_data->bytes_decoded = 0;
					result = 0;
					break;
				}
				case 0xA1: /* str8-utf8 */
				case 0xB1: /* str32-utf8 */
				{
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_STRING;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.string_value.chars = malloc(1);
					if (internal_decoder_data->decode_to_value->value.string_value.chars == NULL)
					{
						result = __LINE__;
					}
					else
					{
						internal_decoder_data->decode_to_value->value.string_value.chars[0] = '\0';
						internal_decoder_data->bytes_decoded = 0;
						result = 0;
					}

					break;
				}
				case 0xD0: /* list32 */
					internal_decoder_data->decode_to_value->type = AMQP_TYPE_LIST;
					internal_decoder_data->decoder_state = DECODER_STATE_TYPE_DATA;
					internal_decoder_data->decode_to_value->value.list_value.count = 0;
					internal_decoder_data->bytes_decoded = 0;
					internal_decoder_data->decode_value_state.list_value_state.list_value_state = DECODE_LIST_STEP_SIZE;
					result = 0;
					break;
				}
				break;
			}

			case DECODER_STATE_TYPE_DATA:
			{
				switch (internal_decoder_data->constructor_byte)
				{
				default:
					result = __LINE__;
					break;

				case 0x00: /* descriptor */
				{
					DECODE_DESCRIBED_VALUE_STEP step = internal_decoder_data->decode_value_state.described_value_state.described_value_state;
					switch (step)
					{
					default:
						result = __LINE__;
						break;

					case DECODE_DESCRIBED_VALUE_STEP_DESCRIPTOR:
					{
						size_t used_bytes;
						if (internal_decoder_decode_bytes(internal_decoder_data->inner_decoder, buffer, size, &used_bytes) != 0)
						{
							result = __LINE__;
						}
						else
						{
							INTERNAL_DECODER_DATA* inner_decoder = (INTERNAL_DECODER_DATA*)internal_decoder_data->inner_decoder;
							buffer += used_bytes;
							size -= used_bytes;

							if (inner_decoder->decoder_state == DECODER_STATE_DONE)
							{
								internal_decoder_destroy(inner_decoder);

								AMQP_VALUE_DATA* described_value = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
								if (described_value == NULL)
								{
									internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
									result = __LINE__;
								}
								else
								{
									described_value->type = AMQP_TYPE_UNKNOWN;
									internal_decoder_data->decode_to_value->value.described_value.value = (AMQP_VALUE)described_value;
									internal_decoder_data->inner_decoder = internal_decoder_create(inner_decoder_callback, internal_decoder_data, described_value);
									if (internal_decoder_data->inner_decoder == NULL)
									{
										internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
										result = __LINE__;
									}
									else
									{
										internal_decoder_data->decode_value_state.described_value_state.described_value_state = DECODE_DESCRIBED_VALUE_STEP_VALUE;
										result = 0;
									}
								}
							}
							else
							{
								result = 0;
							}
						}
						break;
					}
					case DECODE_DESCRIBED_VALUE_STEP_VALUE:
					{
						size_t used_bytes;
						if (internal_decoder_decode_bytes(internal_decoder_data->inner_decoder, buffer, size, &used_bytes) != 0)
						{
							result = __LINE__;
						}
						else
						{
							INTERNAL_DECODER_DATA* inner_decoder = (INTERNAL_DECODER_DATA*)internal_decoder_data->inner_decoder;
							buffer += used_bytes;
							size -= used_bytes;

							if (inner_decoder->decoder_state == DECODER_STATE_DONE)
							{
								internal_decoder_destroy(inner_decoder);
								internal_decoder_data->inner_decoder = NULL;

								internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
								if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
								{
									result = __LINE__;
								}
								else
								{
									result = 0;
								}
							}
							else
							{
								result = 0;
							}
						}
						break;
					}
					}
					break;
				}
				case 0x50:
				{
					/* ubyte */
					internal_decoder_data->decode_to_value->value.ubyte_value = buffer[0];
					buffer++;
					size--;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x52:
				{
					/* smalluint */
					internal_decoder_data->decode_to_value->value.uint_value = buffer[0];
					buffer++;
					size--;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x53:
				{
					/* smallulong */
					internal_decoder_data->decode_to_value->value.ulong_value = buffer[0];
					buffer++;
					size--;
					internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
					if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x60:
				{
					/* ushort */
					internal_decoder_data->decode_to_value->value.ushort_value += ((uint16_t)buffer[0]) << ((1 - internal_decoder_data->bytes_decoded) * 8);
					internal_decoder_data->bytes_decoded++;
					buffer++;
					size--;
					if (internal_decoder_data->bytes_decoded == 2)
					{
						internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
						if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
						{
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x70:
				{
					/* ushort */
					internal_decoder_data->decode_to_value->value.uint_value += ((uint32_t)buffer[0]) << ((3 - internal_decoder_data->bytes_decoded) * 8);
					internal_decoder_data->bytes_decoded++;
					buffer++;
					size--;
					if (internal_decoder_data->bytes_decoded == 4)
					{
						internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
						if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
						{
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0x80:
				{
					/* ulong */
					internal_decoder_data->decode_to_value->value.ulong_value += ((uint64_t)buffer[0]) << ((7 - internal_decoder_data->bytes_decoded) * 8);
					internal_decoder_data->bytes_decoded++;
					buffer++;
					size--;
					if (internal_decoder_data->bytes_decoded == 8)
					{
						internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
						if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
						{
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						result = 0;
					}
					break;
				}
				case 0xA0:
				{
					/* vbin8 */
					if (internal_decoder_data->bytes_decoded == 0)
					{
						internal_decoder_data->decode_to_value->value.binary_value.length = buffer[0];
						internal_decoder_data->bytes_decoded++;
						buffer++;
						size--;

						internal_decoder_data->decode_to_value->value.binary_value.bytes = (unsigned char*)malloc(internal_decoder_data->decode_to_value->value.binary_value.length);
						if (internal_decoder_data->decode_to_value->value.binary_value.bytes == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						uint32_t to_copy = internal_decoder_data->decode_to_value->value.binary_value.length - (internal_decoder_data->bytes_decoded - 1);
						if (to_copy > size)
						{
							to_copy = size;
						}

						if (memcpy((unsigned char*)(internal_decoder_data->decode_to_value->value.binary_value.bytes) + (internal_decoder_data->bytes_decoded - 1), buffer, to_copy) == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							buffer += to_copy;
							size -= to_copy;
							internal_decoder_data->bytes_decoded += to_copy;

							if (internal_decoder_data->bytes_decoded == internal_decoder_data->decode_to_value->value.binary_value.length + 1)
							{
								internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
								if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
								{
									result = __LINE__;
								}
								else
								{
									result = 0;
								}
							}
							else
							{
								result = 0;
							}
						}
					}
				}
				case 0xA1:
				{
					/* str8-utf8 */
					if (internal_decoder_data->bytes_decoded == 0)
					{
						internal_decoder_data->bytes_decoded++;
						buffer++;
						size--;

						internal_decoder_data->decode_value_state.string_value_state.length = buffer[0];

						internal_decoder_data->decode_to_value->value.string_value.chars = (char*)malloc(internal_decoder_data->decode_value_state.string_value_state.length + 1);
						if (internal_decoder_data->decode_to_value->value.string_value.chars == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						uint32_t to_copy = internal_decoder_data->decode_value_state.string_value_state.length - (internal_decoder_data->bytes_decoded - 1);
						if (to_copy > size)
						{
							to_copy = size;
						}

						if (memcpy(internal_decoder_data->decode_to_value->value.string_value.chars + (internal_decoder_data->bytes_decoded - 1), buffer, to_copy) == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							buffer += to_copy;
							size -= to_copy;
							internal_decoder_data->bytes_decoded += to_copy;

							if (internal_decoder_data->bytes_decoded == internal_decoder_data->decode_value_state.string_value_state.length + 1)
							{
								internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
								if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
								{
									result = __LINE__;
								}
								else
								{
									result = 0;
								}
							}
							else
							{
								result = 0;
							}
						}
					}
					break;
				}
				case 0xB0:
				{
					/* vbin32 */
					if (internal_decoder_data->bytes_decoded < 4)
					{
						internal_decoder_data->decode_to_value->value.binary_value.length += buffer[0] << ((3 - internal_decoder_data->bytes_decoded) * 8);
						internal_decoder_data->bytes_decoded++;
						buffer++;
						size--;

						internal_decoder_data->decode_to_value->value.binary_value.bytes = (unsigned char*)malloc(internal_decoder_data->decode_to_value->value.binary_value.length + 1);
						if (internal_decoder_data->decode_to_value->value.binary_value.bytes == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						uint32_t to_copy = internal_decoder_data->decode_to_value->value.binary_value.length - (internal_decoder_data->bytes_decoded - 4);
						if (to_copy > size)
						{
							to_copy = size;
						}

						if (memcpy((unsigned char*)(internal_decoder_data->decode_to_value->value.binary_value.bytes) + (internal_decoder_data->bytes_decoded - 4), buffer, to_copy) == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							buffer += to_copy;
							size -= to_copy;
							internal_decoder_data->bytes_decoded += to_copy;

							if (internal_decoder_data->bytes_decoded == internal_decoder_data->decode_to_value->value.binary_value.length + 4)
							{
								internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
								if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
								{
									result = __LINE__;
								}
								else
								{
									result = 0;
								}
							}
							else
							{
								result = 0;
							}
						}
					}
				}
				case 0xB1:
				{
					/* str32-utf8 */
					if (internal_decoder_data->bytes_decoded < 4)
					{
						internal_decoder_data->decode_value_state.string_value_state.length += buffer[0] << ((3 - internal_decoder_data->bytes_decoded) * 8);
						internal_decoder_data->bytes_decoded++;
						buffer++;
						size--;

						internal_decoder_data->decode_to_value->value.string_value.chars = (char*)malloc(internal_decoder_data->decode_value_state.string_value_state.length + 1);
						if (internal_decoder_data->decode_to_value->value.string_value.chars == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
					}
					else
					{
						uint32_t to_copy = internal_decoder_data->decode_value_state.string_value_state.length - (internal_decoder_data->bytes_decoded - 4);
						if (to_copy > size)
						{
							to_copy = size;
						}

						if (memcpy(internal_decoder_data->decode_to_value->value.string_value.chars + (internal_decoder_data->bytes_decoded - 4), buffer, to_copy) == NULL)
						{
							internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							buffer += to_copy;
							size -= to_copy;
							internal_decoder_data->bytes_decoded += to_copy;

							if (internal_decoder_data->bytes_decoded == internal_decoder_data->decode_value_state.string_value_state.length + 4)
							{
								internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
								if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
								{
									result = __LINE__;
								}
								else
								{
									result = 0;
								}
							}
							else
							{
								result = 0;
							}
						}
					}
					break;
				}
				case 0xD0: /* list32 */
				{
					DECODE_LIST_STEP step = internal_decoder_data->decode_value_state.list_value_state.list_value_state;
					switch (step)
					{
					default:
						result = __LINE__;
						break;

					case DECODE_LIST_STEP_SIZE:
						internal_decoder_data->bytes_decoded++;
						buffer++;
						size--;

						if (internal_decoder_data->bytes_decoded == 4)
						{
							internal_decoder_data->decode_value_state.list_value_state.list_value_state = DECODE_LIST_STEP_COUNT;
							internal_decoder_data->bytes_decoded = 0;
							internal_decoder_data->decode_to_value->value.list_value.count = 0;
						}
						result = 0;

						break;

					case DECODE_LIST_STEP_COUNT:
						internal_decoder_data->decode_to_value->value.list_value.count += buffer[0] << ((3 - internal_decoder_data->bytes_decoded) * 8);
						internal_decoder_data->bytes_decoded++;
						buffer++;
						size--;

						if (internal_decoder_data->bytes_decoded == 4)
						{
							uint32_t i;
							internal_decoder_data->decode_to_value->value.list_value.items = (AMQP_VALUE*)amqpalloc_malloc(sizeof(AMQP_VALUE) * internal_decoder_data->decode_to_value->value.list_value.count);
							if (internal_decoder_data->decode_to_value->value.list_value.items == NULL)
							{
								result = __LINE__;
							}
							else
							{
								for (i = 0; i < internal_decoder_data->decode_to_value->value.list_value.count; i++)
								{
									internal_decoder_data->decode_to_value->value.list_value.items[i] = NULL;
								}
								internal_decoder_data->decode_value_state.list_value_state.list_value_state = DECODE_LIST_STEP_ITEMS;
								internal_decoder_data->bytes_decoded = 0;
								internal_decoder_data->inner_decoder = NULL;
								internal_decoder_data->decode_value_state.list_value_state.item = 0;
								result = 0;
							}
						}
						else
						{
							result = 0;
						}
						break;

					case DECODE_LIST_STEP_ITEMS:
					{
						size_t used_bytes;

						if (internal_decoder_data->bytes_decoded == 0)
						{
							AMQP_VALUE_DATA* list_item = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
							if (list_item == NULL)
							{
								internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
								result = __LINE__;
							}
							else
							{
								list_item->type = AMQP_TYPE_UNKNOWN;
								internal_decoder_data->decode_to_value->value.list_value.items[internal_decoder_data->decode_value_state.list_value_state.item] = list_item;
								internal_decoder_data->inner_decoder = internal_decoder_create(inner_decoder_callback, internal_decoder_data, list_item);
								if (internal_decoder_data->inner_decoder == NULL)
								{
									internal_decoder_data->decoder_state = DECODER_STATE_ERROR;
									result = __LINE__;
								}
								else
								{
									result = 0;
								}
							}
						}

						if (internal_decoder_data->inner_decoder == NULL)
						{
							result = __LINE__;
						}
						else if (internal_decoder_decode_bytes(internal_decoder_data->inner_decoder, buffer, size, &used_bytes) != 0)
						{
							result = __LINE__;
						}
						else
						{
							INTERNAL_DECODER_DATA* inner_decoder = (INTERNAL_DECODER_DATA*)internal_decoder_data->inner_decoder;
							internal_decoder_data->bytes_decoded += used_bytes;
							buffer += used_bytes;
							size -= used_bytes;

							if (inner_decoder->decoder_state == DECODER_STATE_DONE)
							{
								internal_decoder_destroy(inner_decoder);
								internal_decoder_data->inner_decoder = NULL;
								internal_decoder_data->bytes_decoded = 0;

								internal_decoder_data->decode_value_state.list_value_state.item++;
								if (internal_decoder_data->decode_value_state.list_value_state.item == internal_decoder_data->decode_to_value->value.list_value.count)
								{
									internal_decoder_data->decoder_state = DECODER_STATE_CONSTRUCTOR;
									if (internal_decoder_data->value_decoded_callback(internal_decoder_data->value_decoded_callback_context, internal_decoder_data->decode_to_value) != 0)
									{
										result = __LINE__;
									}
									else
									{
										result = 0;
									}
								}
								else
								{
									result = 0;
								}
							}
							else
							{
								result = 0;
							}
						}

						break;
					}
					}

					break;
				}
				}
				break;
			}
			}
		}
	}

	*used_bytes = initial_size - size;

	return result;
}

DECODER_HANDLE decoder_create(VALUE_DECODED_CALLBACK value_decoded_callback, void* value_decoded_callback_context)
{
	DECODER_DATA* decoder_data = (DECODER_DATA*)amqpalloc_malloc(sizeof(DECODER_DATA));
	if (decoder_data != NULL)
	{
		decoder_data->decode_to_value = (AMQP_VALUE_DATA*)amqpalloc_malloc(sizeof(AMQP_VALUE_DATA));
		if (decoder_data->decode_to_value == NULL)
		{
			free(decoder_data);
			decoder_data = NULL;
		}
		else
		{
			decoder_data->decode_to_value->type = AMQP_TYPE_UNKNOWN;
			decoder_data->internal_decoder = internal_decoder_create(value_decoded_callback, value_decoded_callback_context, decoder_data->decode_to_value);
		}
	}

	return decoder_data;
}

void decoder_destroy(DECODER_HANDLE handle)
{
	DECODER_DATA* decoder_data = (DECODER_DATA*)handle;
	if (decoder_data != NULL)
	{
		amqpvalue_destroy(decoder_data->decode_to_value);
		internal_decoder_destroy(decoder_data->internal_decoder);
		amqpalloc_free(handle);
	}
}

int decoder_decode_bytes(DECODER_HANDLE handle, const unsigned char* buffer, size_t size)
{
	int result;

	DECODER_DATA* decoder_data = (DECODER_DATA*)handle;
	if (decoder_data == NULL)
	{
		result = __LINE__;
	}
	else
	{
		size_t used_bytes;
		result = internal_decoder_decode_bytes(decoder_data->internal_decoder, buffer, size, &used_bytes);
		if (used_bytes < size)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

static int output_byte(ENCODER_OUTPUT encoder_output, void* context, unsigned char b)
{
	int result;

	if (encoder_output != NULL)
	{
		result = encoder_output(context, &b, 1);
	}
	else
	{
		result = 0;
	}

	return result;
}

static int output_bytes(ENCODER_OUTPUT encoder_output, void* context, const void* bytes, size_t length)
{
	int result;

	if (encoder_output != NULL)
	{
		result = encoder_output(context, bytes, length);
	}
	else
	{
		result = 0;
	}

	return result;
}

static int encode_string(ENCODER_OUTPUT encoder_output, void* context, const char* value)
{
	int result;
	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		size_t length = strlen(value);

		if (length <= 255)
		{
			output_byte(encoder_output, context, (unsigned char)0xA1);
			output_byte(encoder_output, context, (unsigned char)length);
			output_bytes(encoder_output, context, value, length);
		}
		else
		{
			output_byte(encoder_output, context, 0xB1);
			output_byte(encoder_output, context, (length >> 24) & 0xFF);
			output_byte(encoder_output, context, (length >> 16) & 0xFF);
			output_byte(encoder_output, context, (length >> 8) & 0xFF);
			output_byte(encoder_output, context, length & 0xFF);
			output_bytes(encoder_output, context, value, length);
		}

		result = 0;
	}

	return result;
}

static int encode_binary(ENCODER_OUTPUT encoder_output, void* context, const unsigned char* value, uint32_t length)
{
	int result;
	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if (length <= 255)
		{
			output_byte(encoder_output, context, 0xA0);
			output_byte(encoder_output, context, (unsigned char)length);
			output_bytes(encoder_output, context, value, length);
		}
		else
		{
			output_byte(encoder_output, context, 0xB0);
			output_byte(encoder_output, context, (length >> 24) & 0xFF);
			output_byte(encoder_output, context, (length >> 16) & 0xFF);
			output_byte(encoder_output, context, (length >> 8) & 0xFF);
			output_byte(encoder_output, context, length & 0xFF);
			output_bytes(encoder_output, context, value, length);
		}

		result = 0;
	}

	return result;
}

int encoder_encode_null(ENCODER_OUTPUT encoder_output, void* context)
{
	int result;
	if (output_byte(encoder_output, context, (unsigned char)0x40) != 0)
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

static int encode_ulong(ENCODER_OUTPUT encoder_output, void* context, uint64_t value)
{
	int result;
	if (value == 0)
	{
		/* ulong0 */
		output_byte(encoder_output, context, 0x44);
	}
	else if (value <= 255)
	{
		/* smallulong */
		output_byte(encoder_output, context, 0x53);
		output_byte(encoder_output, context, value & 0xFF);
	}
	else
	{
		output_byte(encoder_output, context, 0x70);
		output_byte(encoder_output, context, (value >> 56) & 0xFF);
		output_byte(encoder_output, context, (value >> 48) & 0xFF);
		output_byte(encoder_output, context, (value >> 40) & 0xFF);
		output_byte(encoder_output, context, (value >> 32) & 0xFF);
		output_byte(encoder_output, context, (value >> 24) & 0xFF);
		output_byte(encoder_output, context, (value >> 16) & 0xFF);
		output_byte(encoder_output, context, (value >> 8) & 0xFF);
		output_byte(encoder_output, context, value & 0xFF);
	}

	result = 0;

	return result;
}

static int encode_bool(ENCODER_OUTPUT encoder_output, void* context, bool value)
{
	int result;

	if (value == false)
	{
		/* false */
		output_byte(encoder_output, context, 0x42);
	}
	else
	{
		/* true */
		output_byte(encoder_output, context, 0x41);
	}

	result = 0;

	return result;
}

static int encode_ubyte(ENCODER_OUTPUT encoder_output, void* context, unsigned char value)
{
	int result;

	/* ubyte */
	output_byte(encoder_output, context, 0x50);
	output_byte(encoder_output, context, value);

	result = 0;

	return result;
}

static int encode_uint(ENCODER_OUTPUT encoder_output, void* context, uint32_t value)
{
	int result;

	if (value == 0)
	{
		/* uint0 */
		output_byte(encoder_output, context, 0x43);
	}
	else if (value <= 255)
	{
		/* smalluint */
		output_byte(encoder_output, context, 0x52);
		output_byte(encoder_output, context, value & 0xFF);
	}
	else
	{
		output_byte(encoder_output, context, 0x70);
		output_byte(encoder_output, context, (value >> 24) & 0xFF);
		output_byte(encoder_output, context, (value >> 16) & 0xFF);
		output_byte(encoder_output, context, (value >> 8) & 0xFF);
		output_byte(encoder_output, context, value & 0xFF);
	}

	result = 0;

	return result;
}

static int encode_descriptor_header(ENCODER_OUTPUT encoder_output, void* context)
{
	int result;

	output_byte(encoder_output, context, 0x00);
	result = 0;

	return result;
}

int amqpvalue_encode(AMQP_VALUE value, ENCODER_OUTPUT encoder_output, void* context)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;

		switch (value_data->type)
		{
		default:
			result = __LINE__;
			break;

		case AMQP_TYPE_NULL:
			if (encoder_encode_null(encoder_output, context) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;

		case AMQP_TYPE_COMPOSITE:
		case AMQP_TYPE_DESCRIBED:
		{
			if ((encode_descriptor_header(encoder_output, context) != 0) ||
				(amqpvalue_encode(value_data->value.described_value.descriptor, encoder_output, context) != 0) ||
				(amqpvalue_encode(value_data->value.described_value.value, encoder_output, context) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;
		}

		case AMQP_TYPE_STRING:
			if (encode_string(encoder_output, context, value_data->value.string_value.chars) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;

		case AMQP_TYPE_BINARY:
		{
			amqp_binary binary_value;

			if ((amqpvalue_get_binary(value, &binary_value) != 0) ||
				(encode_binary(encoder_output, context, binary_value.bytes, binary_value.length) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			break;
		}

		case AMQP_TYPE_BOOL:
		{
			bool bool_value;
			if ((amqpvalue_get_boolean(value, &bool_value) != 0) ||
				(encode_bool(encoder_output, context, bool_value) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;
		}

		case AMQP_TYPE_UBYTE:
		{
			unsigned char ubyte_value;
			if ((amqpvalue_get_ubyte(value, &ubyte_value) != 0) ||
				(encode_ubyte(encoder_output, context, ubyte_value) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;
		}

		case AMQP_TYPE_UINT:
		{
			uint32_t uint_value;
			if ((amqpvalue_get_uint(value, &uint_value) != 0) ||
				(encode_uint(encoder_output, context, uint_value) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;
		}

		case AMQP_TYPE_ULONG:
		{
			uint64_t ulong_value;
			if ((amqpvalue_get_ulong(value, &ulong_value) != 0) ||
				(encode_ulong(encoder_output, context, ulong_value) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
			break;
		}

		case AMQP_TYPE_LIST:
		{
			size_t i;
			uint32_t size = 0;
			AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
			size_t item_count = value_data->value.list_value.count;

			output_byte(encoder_output, context, 0xD0);

			for (i = 0; i < item_count; i++)
			{
				size_t item_size;
				if (amqpvalue_get_encoded_size(value_data->value.list_value.items[i], &item_size) != 0)
				{
					break;
				}

				size += item_size;
			}

			if (i < item_count)
			{
				result = __LINE__;
			}
			else
			{
				output_byte(encoder_output, context, (size >> 24) & 0xFF);
				output_byte(encoder_output, context, (size >> 16) & 0xFF);
				output_byte(encoder_output, context, (size >> 8) & 0xFF);
				output_byte(encoder_output, context, size & 0xFF);

				output_byte(encoder_output, context, (item_count >> 24) & 0xFF);
				output_byte(encoder_output, context, (item_count >> 16) & 0xFF);
				output_byte(encoder_output, context, (item_count >> 8) & 0xFF);
				output_byte(encoder_output, context, item_count & 0xFF);

				for (i = 0; i < item_count; i++)
				{
					if (amqpvalue_encode(value_data->value.list_value.items[i], encoder_output, context) != 0)
					{
						break;
					}
				}

				if (i < item_count)
				{
					result = __LINE__;
				}
				else
				{
					result = 0;
				}
			}

			break;
		}
		}
	}

	return result;
}

int amqpvalue_set_composite_item(AMQP_VALUE value, size_t index, AMQP_VALUE item_value)
{
	int result;

	if (value == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE_DATA* value_data = (AMQP_VALUE_DATA*)value;
		if (value_data->type != AMQP_TYPE_COMPOSITE)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_list_item(value_data->value.described_value.value, index, item_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int count_bytes(void* context, const void* bytes, size_t length)
{
	size_t* byte_count = (uint32_t*)context;
	*byte_count += length;
	return 0;
}

int amqpvalue_get_encoded_size(AMQP_VALUE value, size_t* encoded_size)
{
	int result;

	if ((value == NULL) ||
		(encoded_size == NULL))
	{
		result = __LINE__;
	}
	else
	{
		*encoded_size = 0;
		result = amqpvalue_encode(value, count_bytes, encoded_size);
	}

	return result;
}
