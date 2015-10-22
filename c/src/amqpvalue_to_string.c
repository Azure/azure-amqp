#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "amqpvalue_to_string.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

#if _WIN32
/* The MS runtime does not have snprintf */
#define snprintf _snprintf
#endif

static int string_concat(char** string, const char* to_concat)
{
	int result;

	if ((string == NULL) ||
		(to_concat == NULL))
	{
		result = __LINE__;
	}
	else
	{
		size_t length = strlen(to_concat) + 1;
		size_t src_length;

		if (*string != NULL)
		{
			src_length = strlen(*string);
		}
		else
		{
			src_length = 0;
		}

		length += src_length;

		char* new_string = amqpalloc_realloc(*string, length);
		if (new_string == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*string = new_string;
			(void)strcpy(*string + src_length, to_concat);
			result = 0;
		}
	}

	return result;
}

char* amqpvalue_to_string(AMQP_VALUE amqp_value)
{
	char* result = NULL;

	if (amqp_value != NULL)
	{
		AMQP_TYPE amqp_type = amqpvalue_get_type(amqp_value);
		switch (amqp_type)
		{
		default:
			result = NULL;
			break;

		case AMQP_TYPE_NULL:
			if (string_concat(&result, "NULL") != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			break;
		case AMQP_TYPE_BOOL:
		{
			bool value;
			if ((amqpvalue_get_boolean(amqp_value, &value) != 0) ||
				(string_concat(&result, (value == true) ? "true" : "false") != 0))
			{
				amqpalloc_free(result);
				result = NULL;
			}
			break;
		}
		case AMQP_TYPE_UBYTE:
		{
			char str_value[4];
			uint8_t value;
			if (amqpvalue_get_ubyte(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				unsigned int uint_value = value;
				if ((sprintf(str_value, "%u", uint_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_USHORT:
		{
			char str_value[6];
			uint16_t value;
			if (amqpvalue_get_ushort(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				unsigned int uint_value = value;
				if ((sprintf(str_value, "%u", uint_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_UINT:
		{
			char str_value[11];
			uint32_t value;
			if (amqpvalue_get_uint(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				unsigned long uint_value = value;
				if ((sprintf(str_value, "%lu", uint_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_ULONG:
		{
			char str_value[20];
			uint64_t value;
			if (amqpvalue_get_ulong(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				unsigned long long uint_value = value;
				if ((sprintf(str_value, "%llu", uint_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_BYTE:
		{
			char str_value[5];
			int8_t value;
			if (amqpvalue_get_ubyte(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				int int_value = value;
				if ((sprintf(str_value, "%d", int_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_SHORT:
		{
			char str_value[7];
			int16_t value;
			if (amqpvalue_get_ushort(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				int int_value = value;
				if ((sprintf(str_value, "%d", int_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_INT:
		{
			char str_value[12];
			int32_t value;
			if (amqpvalue_get_uint(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				unsigned long int_value = value;
				if ((sprintf(str_value, "%ld", int_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_LONG:
		{
			char str_value[21];
			int64_t value;
			if (amqpvalue_get_ulong(amqp_value, &value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				long long int_value = value;
				if ((sprintf(str_value, "%lld", int_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_FLOAT:
		{
			float float_value;
			if (amqpvalue_get_float(amqp_value, &float_value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				char str_value[25];
				if ((snprintf(str_value, sizeof(str_value), "%.02f", float_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
				if (string_concat(&result, str_value) != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_DOUBLE:
		{
			double double_value;
			if (amqpvalue_get_double(amqp_value, &double_value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				char str_value[25];
				if ((snprintf(str_value, sizeof(str_value), "%.02F", double_value) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
				if (string_concat(&result, str_value) != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_CHAR:
		{
			uint32_t char_code;
			if (amqpvalue_get_char(amqp_value, &char_code) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				char str_value[25];
				if ((snprintf(str_value, sizeof(str_value), "U%02X%02X%02X%02X", char_code >> 24, (char_code >> 16) & 0xFF, (char_code >> 8) & 0xFF, char_code & 0xFF) < 0) ||
					(string_concat(&result, str_value) != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
				if (string_concat(&result, str_value) != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_TIMESTAMP:
			break;
		case AMQP_TYPE_UUID:
			break;
		case AMQP_TYPE_BINARY:
		{
			amqp_binary binary_value;
			if (amqpvalue_get_binary(amqp_value, &binary_value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (string_concat(&result, "<") != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					uint64_t i;

					for (i = 0; i < binary_value.length; i++)
					{
						char str_value[4];
						if ((snprintf(str_value, sizeof(str_value), "%s%02X", (i > 0) ? " " : "", ((unsigned char*)binary_value.bytes)[i]) < 0) ||
							(string_concat(&result, str_value) != 0))
						{
							break;
						}
					}

					if (i < binary_value.length)
					{
						amqpalloc_free(result);
						result = NULL;
					}
					else if (string_concat(&result, ">") != 0)
					{
						amqpalloc_free(result);
						result = NULL;
					}
				}
			}
			break;
		}
		case AMQP_TYPE_STRING:
		{
			const char* string_value;
			if (amqpvalue_get_string(amqp_value, &string_value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (string_concat(&result, string_value) != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_SYMBOL:
		{
			const char* string_value;
			if (amqpvalue_get_symbol(amqp_value, &string_value) != 0)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (string_concat(&result, string_value) != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_LIST:
		{
			uint32_t count;
			if ((amqpvalue_get_list_item_count(amqp_value, &count) != 0) ||
				(string_concat(&result, "{") != 0))
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				size_t i;
				for (i = 0; i < count; i++)
				{
					AMQP_VALUE item = amqpvalue_get_list_item(amqp_value, i);
					if (item == NULL)
					{
						break;
					}
					else
					{
						char* item_string = amqpvalue_to_string(item);
						if (item_string == NULL)
						{
							amqpvalue_destroy(item);
							break;
						}
						else
						{
							if ((i > 0) && (string_concat(&result, ",") != 0))
							{
								amqpalloc_free(result);
								result = NULL;
								break;
							}
							else if (string_concat(&result, item_string) != 0)
							{
								amqpalloc_free(result);
								result = NULL;
								break;
							}

							amqpalloc_free(item_string);
						}

						amqpvalue_destroy(item);
					}
				}

				if ((i < count) ||
					(string_concat(&result, "}") != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_MAP:
		{
			uint32_t count;
			if ((amqpvalue_get_map_pair_count(amqp_value, &count) != 0) ||
				(string_concat(&result, "{") != 0))
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				size_t i;
				for (i = 0; i < count; i++)
				{
					AMQP_VALUE key;
					AMQP_VALUE value;
					if (amqpvalue_get_map_key_value_pair(amqp_value, i, &key, &value) != 0)
					{
						break;
					}
					else
					{
						char* key_string = amqpvalue_to_string(key);
						if (key_string == NULL)
						{
							amqpvalue_destroy(key);
							amqpvalue_destroy(value);
							break;
						}
						else
						{
							char* value_string = amqpvalue_to_string(value);
							if (key_string == NULL)
							{
								amqpalloc_free(key_string);
								amqpvalue_destroy(key);
								amqpvalue_destroy(value);
								break;
							}
							else
							{
								if (((i > 0) && (string_concat(&result, ",") != 0)) ||
									(string_concat(&result, "[") != 0) ||
									(string_concat(&result, key_string) != 0) ||
									(string_concat(&result, ":") != 0) ||
									(string_concat(&result, value_string) != 0) ||
									(string_concat(&result, "]") != 0))
								{
									amqpalloc_free(key_string);
									amqpalloc_free(value_string);
									amqpvalue_destroy(key);
									amqpvalue_destroy(value);
									break;
								}

								amqpalloc_free(value_string);
							}

							amqpalloc_free(key_string);
						}

						amqpvalue_destroy(key);
						amqpvalue_destroy(value);
					}
				}

				if ((i < count) ||
					(string_concat(&result, "}") != 0))
				{
					amqpalloc_free(result);
					result = NULL;
				}
			}
			break;
		}
		case AMQP_TYPE_COMPOSITE:
		case AMQP_TYPE_DESCRIBED:
		{
			AMQP_VALUE described_value = amqpvalue_get_described_value(amqp_value);
			if (described_value == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				if (string_concat(&result, "* ") != 0)
				{
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					char* described_value_string = amqpvalue_to_string(described_value);
					if (described_value_string == NULL)
					{
						amqpalloc_free(result);
						result = NULL;
					}
					else
					{
						if (string_concat(&result, described_value_string) != 0)
						{
							amqpalloc_free(result);
							result = NULL;
						}

						amqpalloc_free(described_value_string);
					}
				}
			}
			break;
		}
		}
	}

	return result;
}
