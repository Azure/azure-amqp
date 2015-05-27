#include <stdlib.h>
#include "decoder.h"
#include "amqpalloc.h"

typedef enum DECODER_STATE_TAG
{
	DECODER_STATE_CONSTRUCTOR,
	DECODER_STATE_TYPE_DATA,
	DECODER_STATE_ERROR
} DECODER_STATE;

typedef struct DECODER_DATA_TAG
{
	VALUE_DECODED_CALLBACK value_decoded_callback;
	void* value_decoded_callback_context;
	void* inner_decoder;
	size_t bytes_received;
	DECODER_STATE decoder_state;
	uint8_t constructor_byte;
} DECODER_DATA;

DECODER_HANDLE decoder_create(VALUE_DECODED_CALLBACK value_decoded_callback, void* value_decoded_callback_context)
{
	DECODER_DATA* decoderData = (DECODER_DATA*)amqpalloc_malloc(sizeof(DECODER_DATA));
	if (decoderData != NULL)
	{
		decoderData->value_decoded_callback = value_decoded_callback;
		decoderData->value_decoded_callback_context = value_decoded_callback_context;
		decoderData->inner_decoder = NULL;
		decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
	}

	return decoderData;
}

void decoder_destroy(DECODER_HANDLE handle)
{
	amqpalloc_free(handle);
}

int decoder_decode_bytes(DECODER_HANDLE handle, const unsigned char* buffer, size_t size)
{
	int result;

	if (handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DECODER_DATA* decoderData = (DECODER_DATA*)handle;
		switch (decoderData->decoder_state)
		{
			default:
				break;
			case DECODER_STATE_CONSTRUCTOR:
			{
				decoderData->constructor_byte = buffer[0];
				buffer++;
				size--;
				switch (decoderData->constructor_byte)
				{
					default:
						result = __LINE__;
						break;
					case 0x40:
					{
						/* null */
						AMQP_VALUE amqp_value = amqpvalue_create_null();
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}

						break;
					}
					case 0x41:
					{
						/* true */
						AMQP_VALUE amqp_value = amqpvalue_create_boolean(true);
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}
						break;
					}
					case 0x42:
					{
						/* false */
						AMQP_VALUE amqp_value = amqpvalue_create_boolean(false);
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}
						break;
					}
					case 0x43:
					{
						/* uint0 */
						AMQP_VALUE amqp_value = amqpvalue_create_uint(0);
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}
						break;
					}
					case 0x44:
					{
						/* ulong0 */
						AMQP_VALUE amqp_value = amqpvalue_create_ulong(0);
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}
						break;
					}
					case 0x50: /* ubyte */
					case 0x53: /* smallulong */
					{
						decoderData->decoder_state = DECODER_STATE_TYPE_DATA;
						break;
					}
				}
				decoderData->decoder_state = DECODER_STATE_TYPE_DATA;
				break;
			}

			case DECODER_STATE_TYPE_DATA:
			{
				switch (decoderData->constructor_byte)
				{
					default:
						break;
					case 0x50:
					{
						/* ubyte */
						AMQP_VALUE amqp_value = amqpvalue_create_ubyte(buffer[0]);
						buffer++;
						size--;
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}
						break;
					}
					case 0x53:
					{
						/* smallulong */
						AMQP_VALUE amqp_value = amqpvalue_create_ulong(buffer[0]);
						buffer++;
						size--;
						if (amqp_value == NULL)
						{
							decoderData->decoder_state = DECODER_STATE_ERROR;
							result = __LINE__;
						}
						else
						{
							decoderData->value_decoded_callback(decoderData->value_decoded_callback_context, amqp_value);
							amqpvalue_destroy(amqp_value);
							decoderData->decoder_state = DECODER_STATE_CONSTRUCTOR;
							result = 0;
						}
						break;
					}
				}
				break;
			}
		}
	}
#if 0
		if (decoderData->pos < decoderData->size)
		{
			unsigned char first_constructor_byte = decoderData->buffer[decoderData->pos++];
			switch (first_constructor_byte)
			{
			case 0x00:
			{
				/* descriptor */
				AMQP_VALUE descriptorValue;
				if (decoder_decode(handle, &descriptorValue, NULL) != 0)
				{
					result = __LINE__;
				}
				else
				{
					*amqp_value = amqpvalue_create_descriptor(descriptorValue);
					if (*amqp_value == NULL)
					{
						amqpvalue_destroy(descriptorValue);
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}
				break;
			}

			case 0x60:
				/* ushort */
				if (decoderData->size - decoderData->pos < 2)
				{
					result = __LINE__;
				}
				else
				{
					uint16_t ushort_value = decoderData->buffer[decoderData->pos++] << 8;
					ushort_value += decoderData->buffer[decoderData->pos++];
					*amqp_value = amqpvalue_create_ushort(ushort_value);
					if (*amqp_value == NULL)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}
				break;

			case 0x80:
				/* ulong */
				if (decoderData->size - decoderData->pos < 8)
				{
					result = __LINE__;
				}
				else
				{
					uint64_t value = (uint64_t)decoderData->buffer[decoderData->pos++] << 56;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 48;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 40;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 32;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 24;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 16;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 8;
					value += (uint64_t)decoderData->buffer[decoderData->pos++];
					*amqp_value = amqpvalue_create_ulong(value);
					if (*amqp_value == NULL)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}
				break;

			case 0x52:
				/* smalluint */
				if (decoderData->pos >= decoderData->size)
				{
					result = __LINE__;
				}
				else
				{
					*amqp_value = amqpvalue_create_uint(decoderData->buffer[decoderData->pos++]);
					if (*amqp_value == NULL)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}
				break;

			case 0x70:
				/* uint */
				if (decoderData->size - decoderData->pos < 4)
				{
					result = __LINE__;
				}
				else
				{
					uint32_t value = (uint64_t)decoderData->buffer[decoderData->pos++] << 24;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 16;
					value += (uint64_t)decoderData->buffer[decoderData->pos++] << 8;
					value += (uint64_t)decoderData->buffer[decoderData->pos++];
					*amqp_value = amqpvalue_create_uint(value);
					if (*amqp_value == NULL)
					{
						result = __LINE__;
					}
					else
					{
						result = 0;
					}
				}
				break;

			case 0xA1:
				/* str8-utf8 */
				if (decoderData->pos >= decoderData->size)
				{
					result = __LINE__;
				}
				else
				{
					size_t length = decoderData->buffer[decoderData->pos++];

					if (decoderData->size - decoderData->pos < length)
					{
						result = __LINE__;
					}
					else
					{
						*amqp_value = amqpvalue_create_string_with_length(&decoderData->buffer[decoderData->pos], length);
						if (*amqp_value == NULL)
						{
							result = __LINE__;
						}
						else
						{
							decoderData->pos += length;
							result = 0;
						}
					}
				}
				break;

			case 0xB1:
				/* str32-utf8 */
				if (decoderData->size - decoderData->pos < 4)
				{
					result = __LINE__;
				}
				else
				{
					uint32_t length = (uint32_t)decoderData->buffer[decoderData->pos++] << 24;
					length += (uint32_t)decoderData->buffer[decoderData->pos++] << 16;
					length += (uint32_t)decoderData->buffer[decoderData->pos++] << 8;
					length += (uint32_t)decoderData->buffer[decoderData->pos++];

					if (decoderData->size - decoderData->pos < length)
					{
						result = __LINE__;
					}
					else
					{
						*amqp_value = amqpvalue_create_string_with_length(&decoderData->buffer[decoderData->pos], length);
						if (*amqp_value == NULL)
						{
							result = __LINE__;
						}
						else
						{
							decoderData->pos += length;
							result = 0;
						}
					}
				}
				break;

			case 0xA0:
				/* vbin8 */
				if (decoderData->pos >= decoderData->size)
				{
					result = __LINE__;
				}
				else
				{
					size_t length = decoderData->buffer[decoderData->pos++];

					if (decoderData->size - decoderData->pos < length)
					{
						result = __LINE__;
					}
					else
					{
						*amqp_value = amqpvalue_create_binary(&decoderData->buffer[decoderData->pos], length);
						if (*amqp_value == NULL)
						{
							result = __LINE__;
						}
						else
						{
							decoderData->pos += length;
							result = 0;
						}
					}
				}
				break;

			case 0xB0:
				/* vbin32 */
				if (decoderData->size - decoderData->pos < 4)
				{
					result = __LINE__;
				}
				else
				{
					uint32_t length = (uint32_t)decoderData->buffer[decoderData->pos++] << 24;
					length += (uint32_t)decoderData->buffer[decoderData->pos++] << 16;
					length += (uint32_t)decoderData->buffer[decoderData->pos++] << 8;
					length += (uint32_t)decoderData->buffer[decoderData->pos++];

					if (decoderData->size - decoderData->pos < length)
					{
						result = __LINE__;
					}
					else
					{
						*amqp_value = amqpvalue_create_binary(&decoderData->buffer[decoderData->pos], length);
						if (*amqp_value == NULL)
						{
							result = __LINE__;
						}
						else
						{
							decoderData->pos += length;
							result = 0;
						}
					}
				}
				break;

            case 0xD0:
                /* list32 */
                if (decoderData->size - decoderData->pos < 8)
                {
                    result = __LINE__;
                }
                else
                {
                    uint32_t i;
                    uint32_t size = (uint32_t)decoderData->buffer[decoderData->pos++] << 24;
					size += (uint32_t)decoderData->buffer[decoderData->pos++] << 16;
					size += (uint32_t)decoderData->buffer[decoderData->pos++] << 8;
					size += (uint32_t)decoderData->buffer[decoderData->pos++];

					uint32_t count = (uint32_t)decoderData->buffer[decoderData->pos++] << 24;
					count += (uint32_t)decoderData->buffer[decoderData->pos++] << 16;
					count += (uint32_t)decoderData->buffer[decoderData->pos++] << 8;
					count += (uint32_t)decoderData->buffer[decoderData->pos++];

                    *amqp_value = amqpvalue_create_list(count);
                    if (*amqp_value == NULL)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        for (i = 0; i < count; i++)
                        {
							AMQP_VALUE child_amqp_value;
							if (decoder_decode(handle, &child_amqp_value, NULL) != 0)
							{
								break;
							}
							else if(amqpvalue_set_list_item(*amqp_value, i, child_amqp_value) != 0)
							{
								break;
							}
                        }

						if (i < count)
						{
							result = __LINE__;
						}
						else
						{
							result = 0;
						}
                    }

                }
                break;
            }
		}
		else
		{
			result = __LINE__;
		}
	}
#endif

	return result;
}
