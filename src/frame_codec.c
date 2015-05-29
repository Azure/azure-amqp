#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "frame_codec.h"
#include "amqpvalue.h"
#include "encoder.h"
#include "logger.h"
#include "io.h"
#include "amqpalloc.h"
#include "list.h"

#define FRAME_HEADER_SIZE 8

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_DOFF,
	RECEIVE_FRAME_STATE_FRAME_TYPE,
	RECEIVE_FRAME_STATE_TYPE_SPECIFIC,
	RECEIVE_FRAME_STATE_FRAME_BODY,
	RECEIVE_FRAME_STATE_ERROR
} RECEIVE_FRAME_STATE;

typedef struct SUBSCRIPTION_TAG
{
	uint8_t frame_type;
	FRAME_BEGIN_CALLBACK frame_begin_callback;
	FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback;
	void* callback_context;
} SUBSCRIPTION;

typedef struct FRAME_CODEC_DATA_TAG
{
	IO_HANDLE io;
	LOGGER_LOG logger_log;
	LIST_HANDLE subscription_list;
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_pos;
	uint32_t receive_frame_size;
	uint8_t receive_frame_doff;
	uint8_t receive_frame_type;
	uint32_t max_frame_size;
	unsigned char* receive_frame_type_specific;
	SUBSCRIPTION* receive_frame_subscription;
} FRAME_CODEC_DATA;

int frame_codec_encode_frame_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, size_t length)
{
	int result;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	if (frame_codec_data->io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		if (io_send(frame_codec_data->io, bytes, length) != 0)
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

int frame_codec_encode_bytes(void* context, const void* bytes, size_t length)
{
	IO_HANDLE io_handle = (IO_HANDLE)context;
	return io_send(io_handle, bytes, length);
}

static int decode_received_sasl_frame(FRAME_CODEC_DATA* frame_codec)
{
	/* not implemented */
	return __LINE__;
}

/* Codes_SRS_FRAME_CODEC_01_001: [Frames are divided into three distinct areas: a fixed width frame header, a variable width extended header, and a variable width frame body.] */
/* Codes_SRS_FRAME_CODEC_01_002: [frame header The frame header is a fixed size (8 byte) structure that precedes each frame.] */
/* Codes_SRS_FRAME_CODEC_01_003: [The frame header includes mandatory information necessary to parse the rest of the frame including size and type information.] */
/* Codes_SRS_FRAME_CODEC_01_004: [extended header The extended header is a variable width area preceding the frame body.] */
/* Codes_SRS_FRAME_CODEC_01_007: [frame body The frame body is a variable width sequence of bytes the format of which depends on the frame type.] */
/* Codes_SRS_FRAME_CODEC_01_028: [The sequence of bytes shall be decoded according to the AMQP ISO.] */
/* Codes_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
static int receive_frame_byte(FRAME_CODEC_DATA* frame_codec, unsigned char b)
{
	int result;

	return result;
}

static bool find_subscription_by_frame_type(LIST_ITEM_HANDLE list_item, const void* match_context)
{
	SUBSCRIPTION* subscription = (SUBSCRIPTION*)list_item;
	return subscription->frame_type == *((uint8_t*)match_context) ? true : false;
}

FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, LOGGER_LOG logger_log)
{
	FRAME_CODEC_DATA* result;

	/* Codes_SRS_FRAME_CODEC_01_020: [If the io argument is NULL, frame_codec_create shall return NULL.] */
	if (io == NULL)
	{
		result = NULL;
	}
	else
	{
		result = amqpalloc_malloc(sizeof(FRAME_CODEC_DATA));
		/* Codes_SRS_FRAME_CODEC_01_022: [If allocating memory for the frame_codec instance fails, frame_codec_create shall return NULL.] */
		if (result != NULL)
		{
			/* Codes_SRS_FRAME_CODEC_01_021: [frame_codec_create shall create a new instance of frame_codec and return a non-NULL handle to it on success.] */
			result->io = io;
			result->logger_log = logger_log;
			result->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			result->receive_frame_pos = 0;
			result->receive_frame_size = 0;
			result->receive_frame_type_specific = NULL;
			result->subscription_list = list_create();
		}
	}

	return result;
}

void frame_codec_destroy(FRAME_CODEC_HANDLE frame_codec)
{
	/* Codes_SRS_FRAME_CODEC_01_024: [If frame_codec is NULL, frame_codec_destroy shall do nothing.] */
	if (frame_codec != NULL)
	{
		FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

		list_destroy(frame_codec_data->subscription_list);
		if (frame_codec_data->receive_frame_type_specific != NULL)
		{
			amqpalloc_free(frame_codec_data->receive_frame_type_specific);
		}

		/* Codes_SRS_FRAME_CODEC_01_023: [frame_codec_destroy shall free all resources associated with a frame_codec instance.] */
		amqpalloc_free(frame_codec);
	}
}

int frame_codec_set_max_frame_size(FRAME_CODEC_HANDLE frame_codec, uint32_t max_frame_size)
{
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	frame_codec_data->max_frame_size = max_frame_size;
	return 0;
}

int frame_codec_receive_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* buffer, size_t size)
{
	int result;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	/* Codes_SRS_FRAME_CODEC_01_026: [If frame_codec or buffer are NULL, frame_codec_receive_bytes shall return a non-zero value.] */
	if ((frame_codec == NULL) ||
		(buffer == NULL) ||
		/* Codes_SRS_FRAME_CODEC_01_027: [If size is zero, frame_codec_receive_bytes shall return a non-zero value.] */
		(size == 0))
	{
		result = __LINE__;
	}
	else
	{
		while (size > 0)
		{
			switch (frame_codec_data->receive_frame_state)
			{
			default:
			case RECEIVE_FRAME_STATE_ERROR:
				/* Codes_SRS_frame_codec_data_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
				result = __LINE__;
				size = 0;
				break;

				/* Codes_SRS_frame_codec_data_01_008: [SIZE Bytes 0-3 of the frame header contain the frame size.] */
			case RECEIVE_FRAME_STATE_FRAME_SIZE:
				/* Codes_SRS_frame_codec_data_01_009: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
				frame_codec_data->receive_frame_size += buffer[0] << (24 - frame_codec_data->receive_frame_pos * 8);
				buffer++;
				size--;
				frame_codec_data->receive_frame_pos++;

				if (frame_codec_data->receive_frame_pos == 4)
				{
					/* Codes_SRS_frame_codec_data_01_010: [The frame is malformed if the size is less than the size of the frame header (8 bytes).] */
					if (frame_codec_data->receive_frame_size < FRAME_HEADER_SIZE)
					{
						/* Codes_SRS_frame_codec_data_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
						result = __LINE__;
					}
					else
					{
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_DOFF;
						result = 0;
					}
				}
				else
				{
					result = 0;
				}

				break;

			case RECEIVE_FRAME_STATE_DOFF:
				/* Codes_SRS_frame_codec_data_01_011: [DOFF Byte 4 of the frame header is the data offset.] */
				/* Codes_SRS_frame_codec_data_01_013: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
				/* Codes_SRS_frame_codec_data_01_012: [This gives the position of the body within the frame.] */
				frame_codec_data->receive_frame_doff = buffer[0];
				buffer++;
				size--;

				/* Codes_SRS_frame_codec_data_01_014: [Due to the mandatory 8-byte frame header, the frame is malformed if the value is less than 2.] */
				if (frame_codec_data->receive_frame_doff < 2)
				{
					/* Codes_SRS_frame_codec_data_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
					frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
					result = __LINE__;
				}
				else
				{
					frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_TYPE;
					result = 0;
				}

				break;

			case RECEIVE_FRAME_STATE_FRAME_TYPE:
			{
				uint32_t type_specific_size = (frame_codec_data->receive_frame_doff * 4) - 6;

				/* Codes_SRS_frame_codec_data_01_015: [TYPE Byte 5 of the frame header is a type code.] */
				frame_codec_data->receive_frame_type = buffer[0];
				buffer++;
				size--;

				/* Codes_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value.] */
				frame_codec_data->receive_frame_subscription = (SUBSCRIPTION*)list_find(frame_codec_data->subscription_list, find_subscription_by_frame_type, &frame_codec_data->receive_frame_type);

				if (frame_codec_data->receive_frame_subscription != NULL)
				{
					frame_codec_data->receive_frame_pos = 0;
					frame_codec_data->receive_frame_type_specific = (unsigned char*)amqpalloc_malloc(type_specific_size);
					/* Codes_SRS_frame_codec_data_01_030: [If a decoding error occurs, frame_codec_data_receive_bytes shall return a non-zero value.] */
					if (frame_codec_data->receive_frame_type_specific == NULL)
					{
						/* Codes_SRS_frame_codec_data_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
						result = __LINE__;
						break;
					}
					else
					{
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_TYPE_SPECIFIC;
						result = 0;
						break;
					}
				}
				else
				{
					frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_TYPE_SPECIFIC;
					result = 0;
					break;
				}
			}

			case RECEIVE_FRAME_STATE_TYPE_SPECIFIC:
			{
				uint32_t type_specific_size = (frame_codec_data->receive_frame_doff * 4) - 6;
				uint32_t to_copy = type_specific_size - frame_codec_data->receive_frame_pos;
				if (to_copy > size)
				{
					to_copy = size;
				}

				if (frame_codec_data->receive_frame_subscription != NULL)
				{
					if (memcpy(&frame_codec_data->receive_frame_type_specific[frame_codec_data->receive_frame_pos], buffer, to_copy) == NULL)
					{
						/* Codes_SRS_frame_codec_data_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_ERROR;
						result = __LINE__;
						break;
					}
					else
					{
						frame_codec_data->receive_frame_pos += to_copy;
						buffer += to_copy;
						size -= to_copy;
					}
				}
				else
				{
					frame_codec_data->receive_frame_pos += to_copy;
					buffer += to_copy;
					size -= to_copy;
				}

				if (frame_codec_data->receive_frame_pos == type_specific_size)
				{
					if (frame_codec_data->receive_frame_subscription != NULL)
					{
						/* Codes_SRS_frame_codec_data_01_031: [When a frame header is successfully decoded it shall be indicated to the upper layer by invoking the frame_begin_callback passed to frame_codec_data_subscribe.] */
						/* Codes_SRS_frame_codec_data_01_032: [Besides passing the frame information, the callback_context value passed to frame_codec_data_subscribe shall be passed to the frame_begin_callback function.] */
						frame_codec_data->receive_frame_subscription->frame_begin_callback(frame_codec_data->receive_frame_subscription->callback_context, frame_codec_data->receive_frame_size - frame_codec_data->receive_frame_doff * 4, frame_codec_data->receive_frame_type_specific, type_specific_size);
						amqpalloc_free(frame_codec_data->receive_frame_type_specific);
						frame_codec_data->receive_frame_type_specific = NULL;
					}

					/* Codes_SRS_frame_codec_data_01_085: [If the frame body is empty, no call to frame_body_bytes_received_callback shall be made.] */
					if (frame_codec_data->receive_frame_size == FRAME_HEADER_SIZE)
					{
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
						frame_codec_data->receive_frame_size = 0;
					}
					else
					{
						frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_BODY;
					}

					frame_codec_data->receive_frame_pos = 0;
				}

				result = 0;
				break;
			}

			case RECEIVE_FRAME_STATE_FRAME_BODY:
			{
				uint32_t frame_body_size = frame_codec_data->receive_frame_size - (frame_codec_data->receive_frame_doff * 4);
				uint32_t to_notify = frame_body_size;
				FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback = NULL;

				if (to_notify > size)
				{
					to_notify = size;
				}

				if (frame_codec_data->receive_frame_subscription != NULL)
				{
					/* Codes_SRS_frame_codec_data_01_083: [The frame body bytes shall be passed to the frame_body_bytes_received_callback function that was given to frame_codec_data_subscribe.] */
					/* Codes_SRS_frame_codec_data_01_084: [The bytes shall be passed to frame_body_bytes_received_callback as they arrive, not waiting for all frame body bytes to be received.] */
					/* Codes_SRS_frame_codec_data_01_086: [Besides passing the frame information, the callback_context value passed to frame_codec_data_subscribe shall be passed to the frame_body_bytes_received_callback function.] */
					frame_codec_data->receive_frame_subscription->frame_body_bytes_received_callback(frame_codec_data->receive_frame_subscription->callback_context, buffer, to_notify);
				}

				buffer += to_notify;
				size -= to_notify;
				frame_codec_data->receive_frame_pos += to_notify;

				if (frame_codec_data->receive_frame_pos == frame_body_size)
				{
					frame_codec_data->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
					frame_codec_data->receive_frame_pos = 0;
					frame_codec_data->receive_frame_size = 0;
				}
				result = 0;

				break;
			}
			}
		}
	}

	return result;
}

int frame_codec_begin_encode_frame(FRAME_CODEC_HANDLE frame_codec, size_t frame_payload_size)
{
	int result;
	ENCODER_HANDLE encoder_handle;
	size_t frame_size = frame_payload_size + FRAME_HEADER_SIZE;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
	uint8_t doff = 2;
	uint8_t type = 0;
	uint16_t channel = 0;

	/* Codes_SRS_FRAME_CODEC_01_042: [frame_codec_begin_encode_frame encodes the header of a frame that has frame_payload_size bytes.] */
	encoder_handle = encoder_create(frame_codec_encode_bytes, frame_codec_data->io);
	if (encoder_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		unsigned char b;

		b = (frame_size >> 24) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (frame_size >> 16) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (frame_size >> 8) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (frame_size)& 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		(void)io_send(frame_codec_data->io, &doff, sizeof(doff));
		(void)io_send(frame_codec_data->io, &type, sizeof(type));
		b = (channel >> 8) & 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);
		b = (channel)& 0xFF;
		(void)io_send(frame_codec_data->io, &b, 1);

		result = 0;
	}

	return result;
}

/* Codes_SRS_FRAME_CODEC_01_033: [frame_codec_subscribe subscribes for a certain type of frame received by the frame_codec instance identified by frame_codec.] */
int frame_codec_subscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type, FRAME_BEGIN_CALLBACK frame_begin_callback, FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback, void* callback_context)
{
	int result;

	/* Codes_SRS_FRAME_CODEC_01_034: [If any of the frame_codec, frame_begin_callback or frame_body_bytes_received_callback arguments is NULL, frame_codec_subscribe shall return a non-zero value.] */
	if ((frame_codec == NULL) ||
		(frame_begin_callback == NULL) ||
		(frame_body_bytes_received_callback == NULL))
	{
		result = __LINE__;
	}
	else
	{
		FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;
		SUBSCRIPTION* subscription;

		/* Codes_SRS_FRAME_CODEC_01_036: [Only one callback pair shall be allowed to be registered for a given frame type.] */
		/* find the subscription for this frame type */
		subscription = list_find(frame_codec_data->subscription_list, find_subscription_by_frame_type, &type);
		if (subscription != NULL)
		{
			/* a subscription was found */
			subscription->frame_begin_callback = frame_begin_callback;
			subscription->frame_body_bytes_received_callback = frame_body_bytes_received_callback;
			subscription->callback_context = callback_context;

			/* Codes_SRS_FRAME_CODEC_01_087: [On success, frame_codec_subscribe shall return zero.] */
			result = 0;
		}
		else
		{
			/* add a new subscription */
			subscription = amqpalloc_malloc(sizeof(SUBSCRIPTION));
			/* Codes_SRS_FRAME_CODEC_01_037: [If any failure occurs while performing the subscribe operation, frame_codec_subscribe shall return a non-zero value.] */
			if (subscription == NULL)
			{
				result = __LINE__;
			}
			else
			{
				subscription->frame_begin_callback = frame_begin_callback;
				subscription->frame_body_bytes_received_callback = frame_body_bytes_received_callback;
				subscription->callback_context = callback_context;
				subscription->frame_type = type;

				/* Codes_SRS_FRAME_CODEC_01_037: [If any failure occurs while performing the subscribe operation, frame_codec_subscribe shall return a non-zero value.] */
				if (list_add(frame_codec_data->subscription_list, subscription) != 0)
				{
					result = __LINE__;
				}
				else
				{
					/* Codes_SRS_FRAME_CODEC_01_087: [On success, frame_codec_subscribe shall return zero.] */
					result = 0;
				}
			}
		}
	}

	return result;
}

int frame_codec_unsubscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type)
{
	int result;

	/* Codes_SRS_FRAME_CODEC_01_039: [If frame_codec is NULL, frame_codec_unsubscribe shall return a non-zero value.] */
	if (frame_codec == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

		/* Codes_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
		if (list_remove_matching_item(frame_codec_data->subscription_list, find_subscription_by_frame_type, &type) != 0)
		{
			/* Codes_SRS_FRAME_CODEC_01_040: [If no subscription for the type frame type exists, frame_codec_unsubscribe shall return a non-zero value.] */
			/* Codes_SRS_FRAME_CODEC_01_041: [If any failure occurs while performing the unsubscribe operation, frame_codec_unsubscribe shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}
