#include <stdlib.h>
#include <string.h>
#include "encoder.h"
#include "amqpvalue.h"
#include "amqp_types.h"

typedef struct ENCODER_DATA_TAG
{
	size_t encodedBytes;
	ENCODER_OUTPUT encoderOutput;
	void* context;
} ENCODER_DATA;

ENCODER_HANDLE encoder_create(ENCODER_OUTPUT encoderOutput, void* context)
{
	ENCODER_DATA* encoder_data = (ENCODER_DATA*)malloc(sizeof(ENCODER_DATA));
	if (encoder_data != NULL)
	{
		encoder_data->encodedBytes = 0;
		encoder_data->encoderOutput = encoderOutput;
		encoder_data->context = context;
	}

	return encoder_data;
}

void encoder_destroy(ENCODER_HANDLE handle)
{
	free(handle);
}

static int output_byte(ENCODER_DATA* encoder_data, unsigned char b)
{
	int result;

	encoder_data->encodedBytes++;
	if (encoder_data->encoderOutput != NULL)
	{
		result = encoder_data->encoderOutput(encoder_data->context, &b, 1);
	}
	else
	{
		result = 0;
	}

	return result;
}

static int output_bytes(ENCODER_DATA* encoder_data, const void* bytes, size_t length)
{
	int result;

	encoder_data->encodedBytes += length;
	if (encoder_data->encoderOutput != NULL)
	{
		result = encoder_data->encoderOutput(encoder_data->context, bytes, length);
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
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;
		size_t length = strlen(value);
		
		if (length <= 255)
		{
			output_byte(encoder_data, (unsigned char)0xA1);
			output_byte(encoder_data, (unsigned char)length);
			output_bytes(encoder_data, value, length);
		}
		else
		{
			output_byte(encoder_data, 0xB1);
			output_byte(encoder_data, (length >> 24) & 0xFF);
			output_byte(encoder_data, (length >> 16) & 0xFF);
			output_byte(encoder_data, (length >> 8) & 0xFF);
			output_byte(encoder_data, length & 0xFF);
			output_bytes(encoder_data, value, length);
		}

		result = 0;
	}

	return result;
}

int encoder_encode_null(ENCODER_HANDLE handle)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;
		if (output_byte(encoder_data, (unsigned char)0x40) != 0)
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

int encoder_encode_ulong(ENCODER_HANDLE handle, uint64_t value)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;

		if (value == 0)
		{
			/* ulong0 */
			output_byte(encoder_data, 0x44);
		}
		else if (value <= 255)
		{
			/* smallulong */
			output_byte(encoder_data, 0x53);
			output_byte(encoder_data, value & 0xFF);
		}
		else
		{
			output_byte(encoder_data, 0x70);
			output_byte(encoder_data, (value >> 56) & 0xFF);
			output_byte(encoder_data, (value >> 48) & 0xFF);
			output_byte(encoder_data, (value >> 40) & 0xFF);
			output_byte(encoder_data, (value >> 32) & 0xFF);
			output_byte(encoder_data, (value >> 24) & 0xFF);
			output_byte(encoder_data, (value >> 16) & 0xFF);
			output_byte(encoder_data, (value >> 8) & 0xFF);
			output_byte(encoder_data, value & 0xFF);
		}

		result = 0;
	}

	return result;
}

int encoder_encode_bool(ENCODER_HANDLE handle, bool value)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;

		if (value == false)
		{
			/* false */
			output_byte(encoder_data, 0x42);
		}
		else
		{
			/* true */
			output_byte(encoder_data, 0x41);
		}

		result = 0;
	}

	return result;
}

int encoder_encode_ubyte(ENCODER_HANDLE handle, unsigned char value)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;

		/* ubyte */
		output_byte(encoder_data, 0x50);
		output_byte(encoder_data, value);

		result = 0;
	}

	return result;
}

int encoder_encode_uint(ENCODER_HANDLE handle, uint32_t value)
{
	int result;
	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;

		if (value == 0)
		{
			/* uint0 */
			output_byte(encoder_data, 0x43);
		}
		else if (value <= 255)
		{
			/* smalluint */
			output_byte(encoder_data, 0x52);
			output_byte(encoder_data, value & 0xFF);
		}
		else
		{
			output_byte(encoder_data, 0x70);
			output_byte(encoder_data, (value >> 24) & 0xFF);
			output_byte(encoder_data, (value >> 16) & 0xFF);
			output_byte(encoder_data, (value >> 8) & 0xFF);
			output_byte(encoder_data, value & 0xFF);
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
		ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;
		output_byte(encoder_data, 0x00);

		result = 0;
	}

	return result;
}

int encoder_encode_amqp_value(ENCODER_HANDLE handle, AMQP_VALUE value)
{
	int result;
	AMQP_TYPE amqp_type;

	if (amqpvalue_get_type(value, &amqp_type) != 0)
	{
		result = __LINE__;
	}
	else
	{
		switch (amqp_type)
		{
		default:
			result = __LINE__;
			break;

		case AMQP_TYPE_NULL:
			if (encoder_encode_null(handle) != 0)
			{
				return __LINE__;
			}
			else
			{
				return 0;
			}
			break;

		case AMQP_TYPE_STRING:
			if (encoder_encode_string(handle, amqpvalue_get_string(value)) != 0)
			{
				return __LINE__;
			}
			else
			{
				return 0;
			}
			break;

		case AMQP_TYPE_BOOL:
		{
			bool bool_value;
			if ((amqpvalue_get_bool(value, &bool_value) != 0) ||
				(encoder_encode_bool(handle, bool_value) != 0))
			{
				return __LINE__;
			}
			else
			{
				return 0;
			}
			break;
		}

		case AMQP_TYPE_UBYTE:
		{
			unsigned char ubyte_value;
			if ((amqpvalue_get_ubyte(value, &ubyte_value) != 0) ||
				(encoder_encode_ubyte(handle, ubyte_value) != 0))
			{
				return __LINE__;
			}
			else
			{
				return 0;
			}
			break;
		}

		case AMQP_TYPE_UINT:
		{
			uint32_t uint_value;
			if ((amqpvalue_get_uint(value, &uint_value) != 0) ||
				(encoder_encode_uint(handle, uint_value) != 0))
			{
				return __LINE__;
			}
			else
			{
				return 0;
			}
			break;
		}

		case AMQP_TYPE_COMPOSITE:
		{
			AMQP_VALUE descriptor_value = amqpvalue_get_composite_descriptor(value);
			AMQP_VALUE list_value = amqpvalue_get_composite_list(value);
			if ((encoder_encode_amqp_value(handle, descriptor_value) != 0) ||
				(encoder_encode_amqp_value(handle, list_value) != 0))
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
			size_t item_count;
			size_t i;
			ENCODER_DATA* get_size_encoder_handle = encoder_create(NULL, NULL);
			if (get_size_encoder_handle == NULL)
			{
				result = __LINE__;
			}
			else
			{
				if (amqpvalue_get_list_item_count(value, &item_count) != 0)
				{
					result = __LINE__;
				}
				else
				{
					ENCODER_DATA* encoder_data = (ENCODER_DATA*)handle;
					uint32_t size;

					output_byte(encoder_data, 0xD0);

					for (i = 0; i < item_count; i++)
					{
						if (encoder_encode_amqp_value(get_size_encoder_handle, amqpvalue_get_list_item(value, i)) != 0)
						{
							break;
						}
					}

					if ((i < item_count) ||
						(encoder_get_encoded_size(get_size_encoder_handle, &size) != 0))
					{
						result = __LINE__;
					}
					else
					{
						output_byte(encoder_data, (size >> 24) & 0xFF);
						output_byte(encoder_data, (size >> 16) & 0xFF);
						output_byte(encoder_data, (size >> 8) & 0xFF);
						output_byte(encoder_data, size & 0xFF);

						output_byte(encoder_data, (item_count >> 24) & 0xFF);
						output_byte(encoder_data, (item_count >> 16) & 0xFF);
						output_byte(encoder_data, (item_count >> 8) & 0xFF);
						output_byte(encoder_data, item_count & 0xFF);

						for (i = 0; i < item_count; i++)
						{
							if (encoder_encode_amqp_value(handle, amqpvalue_get_list_item(value, i)) != 0)
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
				}

				encoder_destroy(get_size_encoder_handle);
			}

			break;
		}
		}
	}
	return result;
}
