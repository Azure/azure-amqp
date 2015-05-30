#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "frame_codec.h"
#include "amqpvalue.h"
#include "logger.h"
#include "io.h"
#include "amqpalloc.h"
#include "list.h"

#define FRAME_HEADER_SIZE 8
#define MAX_TYPE_SPECIFIC_SIZE	((255 * 4) - 6)

typedef enum RECEIVE_FRAME_STATE_TAG
{
	RECEIVE_FRAME_STATE_FRAME_SIZE,
	RECEIVE_FRAME_STATE_DOFF,
	RECEIVE_FRAME_STATE_FRAME_TYPE,
	RECEIVE_FRAME_STATE_TYPE_SPECIFIC,
	RECEIVE_FRAME_STATE_FRAME_BODY,
	RECEIVE_FRAME_STATE_ERROR
} RECEIVE_FRAME_STATE;

typedef enum ENCODE_FRAME_STATE_TAG
{
	ENCODE_FRAME_STATE_FRAME_HEADER,
	ENCODE_FRAME_STATE_FRAME_BODY,
	ENCODE_FRAME_STATE_ERROR
} ENCODE_FRAME_STATE;

typedef struct SUBSCRIPTION_TAG
{
	uint8_t frame_type;
	FRAME_BEGIN_CALLBACK frame_begin_callback;
	FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback;
	void* callback_context;
} SUBSCRIPTION;

typedef struct FRAME_CODEC_DATA_TAG
{
	/* dependencies */
	IO_HANDLE io;
	LOGGER_LOG logger_log;

	/* subscriptions */
	LIST_HANDLE subscription_list;

	/* decode frame */
	RECEIVE_FRAME_STATE receive_frame_state;
	size_t receive_frame_pos;
	uint32_t receive_frame_size;
	uint8_t receive_frame_doff;
	uint8_t receive_frame_type;
	SUBSCRIPTION* receive_frame_subscription;
	unsigned char* receive_frame_type_specific;

	/* encode frame */
	ENCODE_FRAME_STATE encode_frame_state;
	uint32_t encode_frame_body_bytes_left;

	/* configuration */
	uint32_t max_frame_size;
} FRAME_CODEC_DATA;

static bool find_subscription_by_frame_type(LIST_ITEM_HANDLE list_item, const void* match_context)
{
	SUBSCRIPTION* subscription = (SUBSCRIPTION*)list_item_get_value(list_item);
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
			result->encode_frame_state = ENCODE_FRAME_STATE_FRAME_HEADER;
			result->receive_frame_state = RECEIVE_FRAME_STATE_FRAME_SIZE;
			result->receive_frame_pos = 0;
			result->receive_frame_size = 0;
			result->receive_frame_type_specific = NULL;
			result->subscription_list = list_create();

			/* Codes_SRS_FRAME_CODEC_01_082: [The initial max_frame_size_shall be 512.] */
			result->max_frame_size = 512;
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
	int result;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	/* Codes_SRS_FRAME_CODEC_01_077: [If frame_codec is NULL, frame_codec_set_max_frame_size shall return a non-zero value.] */
	if ((frame_codec == NULL) ||
		/* Codes_SRS_FRAME_CODEC_01_078: [If max_frame_size is invalid according to the AMQP standard, frame_codec_set_max_frame_size shall return a non-zero value.] */
		(max_frame_size < FRAME_HEADER_SIZE) ||
		/* Codes_SRS_FRAME_CODEC_01_081: [If a frame being decoded already has a size bigger than the max_frame_size argument then frame_codec_set_max_frame_size shall return a non-zero value and the previous frame size shall be kept.] */
		((max_frame_size < frame_codec_data->receive_frame_size) && (frame_codec_data->receive_frame_state != RECEIVE_FRAME_STATE_FRAME_SIZE)) ||
		/* Codes_SRS_FRAME_CODEC_01_097: [Setting a frame size on a frame_codec that had a decode error shall fail.] */
		(frame_codec_data->receive_frame_state == RECEIVE_FRAME_STATE_ERROR) ||
		/* Codes_SRS_FRAME_CODEC_01_098: [Setting a frame size on a frame_codec that had an encode error shall fail.] */
		(frame_codec_data->encode_frame_state == ENCODE_FRAME_STATE_ERROR))
	{
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_FRAME_CODEC_01_075: [frame_codec_set_max_frame_size shall set the maximum frame size for a frame_codec.] */
		/* Codes_SRS_FRAME_CODEC_01_079: [The new frame size shall take effect immediately, even for a frame that is being decoded at the time of the call.] */
		frame_codec_data->max_frame_size = max_frame_size;

		/* Codes_SRS_FRAME_CODEC_01_076: [On success, frame_codec_set_max_frame_size shall return 0.] */
		result = 0;
	}
	return result;
}

/* Codes_SRS_FRAME_CODEC_01_001: [Frames are divided into three distinct areas: a fixed width frame header, a variable width extended header, and a variable width frame body.] */
/* Codes_SRS_FRAME_CODEC_01_002: [frame header The frame header is a fixed size (8 byte) structure that precedes each frame.] */
/* Codes_SRS_FRAME_CODEC_01_003: [The frame header includes mandatory information necessary to parse the rest of the frame including size and type information.] */
/* Codes_SRS_FRAME_CODEC_01_004: [extended header The extended header is a variable width area preceding the frame body.] */
/* Codes_SRS_FRAME_CODEC_01_007: [frame body The frame body is a variable width sequence of bytes the format of which depends on the frame type.] */
/* Codes_SRS_FRAME_CODEC_01_028: [The sequence of bytes shall be decoded according to the AMQP ISO.] */
/* Codes_SRS_FRAME_CODEC_01_029: [The sequence of bytes does not have to be a complete frame, frame_codec shall be responsible for maintaining decoding state between frame_codec_receive_bytes calls.] */
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
				/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
				result = __LINE__;
				size = 0;
				break;

				/* Codes_SRS_FRAME_CODEC_01_008: [SIZE Bytes 0-3 of the frame header contain the frame size.] */
			case RECEIVE_FRAME_STATE_FRAME_SIZE:
				/* Codes_SRS_FRAME_CODEC_01_009: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
				frame_codec_data->receive_frame_size += buffer[0] << (24 - frame_codec_data->receive_frame_pos * 8);
				buffer++;
				size--;
				frame_codec_data->receive_frame_pos++;

				if (frame_codec_data->receive_frame_pos == 4)
				{
					/* Codes_SRS_FRAME_CODEC_01_010: [The frame is malformed if the size is less than the size of the frame header (8 bytes).] */
					if ((frame_codec_data->receive_frame_size < FRAME_HEADER_SIZE) ||
						/* Codes_SRS_FRAME_CODEC_01_096: [If a frame bigger than the current max frame size is received, frame_codec_receive_bytes shall fail and return a non-zero value.] */
						(frame_codec_data->receive_frame_size > frame_codec_data->max_frame_size))
					{
						/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
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
				/* Codes_SRS_FRAME_CODEC_01_011: [DOFF Byte 4 of the frame header is the data offset.] */
				/* Codes_SRS_FRAME_CODEC_01_013: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
				/* Codes_SRS_FRAME_CODEC_01_012: [This gives the position of the body within the frame.] */
				frame_codec_data->receive_frame_doff = buffer[0];
				buffer++;
				size--;

				/* Codes_SRS_FRAME_CODEC_01_014: [Due to the mandatory 8-byte frame header, the frame is malformed if the value is less than 2.] */
				if (frame_codec_data->receive_frame_doff < 2)
				{
					/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
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

				/* Codes_SRS_FRAME_CODEC_01_015: [TYPE Byte 5 of the frame header is a type code.] */
				frame_codec_data->receive_frame_type = buffer[0];
				buffer++;
				size--;

				/* Codes_SRS_FRAME_CODEC_01_035: [After successfully registering a callback for a certain frame type, when subsequently that frame type is received the callbacks shall be invoked, passing to it the received frame and the callback_context value.] */
				frame_codec_data->receive_frame_subscription = (SUBSCRIPTION*)list_find(frame_codec_data->subscription_list, find_subscription_by_frame_type, &frame_codec_data->receive_frame_type);

				if (frame_codec_data->receive_frame_subscription != NULL)
				{
					frame_codec_data->receive_frame_pos = 0;
					frame_codec_data->receive_frame_type_specific = (unsigned char*)amqpalloc_malloc(type_specific_size);
					/* Codes_SRS_FRAME_CODEC_01_030: [If a decoding error occurs, frame_codec_data_receive_bytes shall return a non-zero value.] */
					if (frame_codec_data->receive_frame_type_specific == NULL)
					{
						/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
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
						/* Codes_SRS_FRAME_CODEC_01_074: [If a decoding error is detected, any subsequent calls on frame_codec_data_receive_bytes shall fail.] */
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
						/* Codes_SRS_FRAME_CODEC_01_031: [When a frame header is successfully decoded it shall be indicated to the upper layer by invoking the frame_begin_callback passed to frame_codec_data_subscribe.] */
						/* Codes_SRS_FRAME_CODEC_01_032: [Besides passing the frame information, the callback_context value passed to frame_codec_data_subscribe shall be passed to the frame_begin_callback function.] */
						/* Codes_SRS_FRAME_CODEC_01_005: [This is an extension point defined for future expansion.] */
						/* Codes_SRS_FRAME_CODEC_01_006: [The treatment of this area depends on the frame type.] */
						frame_codec_data->receive_frame_subscription->frame_begin_callback(frame_codec_data->receive_frame_subscription->callback_context, frame_codec_data->receive_frame_size - frame_codec_data->receive_frame_doff * 4, frame_codec_data->receive_frame_type_specific, type_specific_size);
						amqpalloc_free(frame_codec_data->receive_frame_type_specific);
						frame_codec_data->receive_frame_type_specific = NULL;
					}

					/* Codes_SRS_FRAME_CODEC_01_085: [If the frame body is empty, no call to frame_body_bytes_received_callback shall be made.] */
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
					/* Codes_SRS_FRAME_CODEC_01_083: [The frame body bytes shall be passed to the frame_body_bytes_received_callback function that was given to frame_codec_data_subscribe.] */
					/* Codes_SRS_FRAME_CODEC_01_084: [The bytes shall be passed to frame_body_bytes_received_callback as they arrive, not waiting for all frame body bytes to be received.] */
					/* Codes_SRS_FRAME_CODEC_01_086: [Besides passing the frame information, the callback_context value passed to frame_codec_data_subscribe shall be passed to the frame_body_bytes_received_callback function.] */
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
					amqpalloc_free(subscription);
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

		if (list_remove_matching_item(frame_codec_data->subscription_list, find_subscription_by_frame_type, &type) != 0)
		{
			/* Codes_SRS_FRAME_CODEC_01_040: [If no subscription for the type frame type exists, frame_codec_unsubscribe shall return a non-zero value.] */
			/* Codes_SRS_FRAME_CODEC_01_041: [If any failure occurs while performing the unsubscribe operation, frame_codec_unsubscribe shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_FRAME_CODEC_01_038: [frame_codec_unsubscribe removes a previous subscription for frames of type type and on success it shall return 0.] */
			result = 0;
		}
	}

	return result;
}

int frame_codec_begin_encode_frame(FRAME_CODEC_HANDLE frame_codec, uint8_t type, uint32_t frame_body_size, const unsigned char* type_specific_bytes, uint32_t type_specific_size)
{
	int result;
	uint32_t frame_body_offset = type_specific_size + 6;

	/* round up to the 4 bytes for doff */
	/* Codes_SRS_FRAME_CODEC_01_067: [The value of the data offset is an unsigned, 8-bit integer specifying a count of 4-byte words.] */
	/* Codes_SRS_FRAME_CODEC_01_068: [Due to the mandatory 8-byte frame header, the frame is malformed if the value is less than 2.] */
	uint8_t doff = (frame_body_offset + 3) / 4;
	size_t frame_size;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	frame_body_offset = doff * 4;

	/* Codes_SRS_FRAME_CODEC_01_063: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
	frame_size = frame_body_size + frame_body_offset;

	/* Codes_SRS_FRAME_CODEC_01_044: [If the argument frame_codec is NULL, frame_codec_begin_encode_frame shall return a non-zero value.] */
	if ((frame_codec == NULL) ||
		/* Codes_SRS_FRAME_CODEC_01_091: [If the argument type_specific_size is greater than 0 and type_specific_bytes is NULL, frame_codec_begin_encode_frame shall return a non-zero value.] */
		((type_specific_size > 0) && (type_specific_bytes == NULL)) ||
		/* Codes_SRS_FRAME_CODEC_01_092: [If type_specific_size is too big to allow encoding the frame according to the AMQP ISO then frame_codec_begin_encode_frame shall return a non-zero value.] */
		(type_specific_size > MAX_TYPE_SPECIFIC_SIZE) ||
		/* Codes_SRS_FRAME_CODEC_01_046: [Once encoding succeeds, all subsequent frame_codec_begin_encode_frame calls shall fail, until all the bytes of the frame have been encoded by using frame_codec_encode_frame_bytes.] */
		(frame_codec_data->encode_frame_state != ENCODE_FRAME_STATE_FRAME_HEADER) ||
		/* Codes_SRS_FRAME_CODEC_01_095: [If the frame_size needed for the frame is bigger than the maximum frame size, frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
		(frame_size > frame_codec_data->max_frame_size))
	{
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_FRAME_CODEC_01_042: [frame_codec_begin_encode_frame encodes the header and type specific bytes of a frame that has frame_payload_size bytes.] */
		/* Codes_SRS_FRAME_CODEC_01_055: [Frames are divided into three distinct areas: a fixed width frame header, a variable width extended header, and a variable width frame body.] */
		/* Codes_SRS_FRAME_CODEC_01_056: [frame header The frame header is a fixed size (8 byte) structure that precedes each frame.] */
		/* Codes_SRS_FRAME_CODEC_01_057: [The frame header includes mandatory information necessary to parse the rest of the frame including size and type information.] */
		/* Codes_SRS_FRAME_CODEC_01_058: [extended header The extended header is a variable width area preceding the frame body.] */
		/* Codes_SRS_FRAME_CODEC_01_059: [This is an extension point defined for future expansion.] */
		/* Codes_SRS_FRAME_CODEC_01_060: [The treatment of this area depends on the frame type.]*/
		/* Codes_SRS_FRAME_CODEC_01_062: [SIZE Bytes 0-3 of the frame header contain the frame size.] */
		/* Codes_SRS_FRAME_CODEC_01_063: [This is an unsigned 32-bit integer that MUST contain the total frame size of the frame header, extended header, and frame body.] */
		/* Codes_SRS_FRAME_CODEC_01_064: [The frame is malformed if the size is less than the size of the frame header (8 bytes).] */
		unsigned char frame_header[] =
		{
			(frame_size >> 24) & 0xFF,
			(frame_size >> 16) & 0xFF,
			(frame_size >> 8) & 0xFF,
			frame_size & 0xFF,
			/* Codes_SRS_FRAME_CODEC_01_065: [DOFF Byte 4 of the frame header is the data offset.] */
			doff,
			/* Codes_SRS_FRAME_CODEC_01_069: [TYPE Byte 5 of the frame header is a type code.] */
			type
		};

		/* Codes_SRS_FRAME_CODEC_01_088: [Encoding the bytes shall happen by passing the bytes to the underlying IO interface.] */
		if (io_send(frame_codec_data->io, frame_header, sizeof(frame_header)) != 0)
		{
			/* Codes_SRS_FRAME_CODEC_01_093: [Once encoding has failed due to IO issues, all subsequent calls to frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
			frame_codec_data->encode_frame_state = ENCODE_FRAME_STATE_ERROR;

			/* Codes_SRS_FRAME_CODEC_01_045: [If encoding the header fails (cannot be sent through the IO interface), frame_codec_begin_encode_frame shall return a non-zero value.] */
			result = __LINE__;
		}
		/* Codes_SRS_FRAME_CODEC_01_088: [Encoding the bytes shall happen by passing the bytes to the underlying IO interface.] */
		else if ((type_specific_size > 0) &&
			(io_send(frame_codec_data->io, type_specific_bytes, type_specific_size) != 0))
		{
			/* Codes_SRS_FRAME_CODEC_01_093: [Once encoding has failed due to IO issues, all subsequent calls to frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
			frame_codec_data->encode_frame_state = ENCODE_FRAME_STATE_ERROR;

			/* Codes_SRS_FRAME_CODEC_01_045: [If encoding the header fails (cannot be sent through the IO interface), frame_codec_begin_encode_frame shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* send padding bytes */
			/* Codes_SRS_FRAME_CODEC_01_090: [If the type_specific_size – 2 does not divide by 4, frame_codec_begin_encode_frame shall pad the type_specific bytes with zeroes so that type specific data is according to the AMQP ISO.] */
			unsigned char padding_bytes[] = { 0x00, 0x00, 0x00 };
			uint8_t padding_byte_count = frame_body_offset - type_specific_size - 6;

			/* Codes_SRS_FRAME_CODEC_01_088: [Encoding the bytes shall happen by passing the bytes to the underlying IO interface.] */
			if (io_send(frame_codec_data->io, padding_bytes, padding_byte_count) != 0)
			{
				/* Codes_SRS_FRAME_CODEC_01_093: [Once encoding has failed due to IO issues, all subsequent calls to frame_codec_begin_encode_frame shall fail and return a non-zero value.] */
				frame_codec_data->encode_frame_state = ENCODE_FRAME_STATE_ERROR;

				/* Codes_SRS_FRAME_CODEC_01_045: [If encoding the header fails (cannot be sent through the IO interface), frame_codec_begin_encode_frame shall return a non-zero value.] */
				result = __LINE__;
			}
			else
			{
				frame_codec_data->encode_frame_body_bytes_left = frame_body_size;
				frame_codec_data->encode_frame_state = ENCODE_FRAME_STATE_FRAME_BODY;

				/* Codes_SRS_FRAME_CODEC_01_043: [On success it shall return 0.] */
				result = 0;
			}
		}
	}

	return result;
}

/* Codes_SRS_FRAME_CODEC_01_061: [frame body The frame body is a variable width sequence of bytes the format of which depends on the frame type.] */
int frame_codec_encode_frame_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, size_t length)
{
	int result;
	FRAME_CODEC_DATA* frame_codec_data = (FRAME_CODEC_DATA*)frame_codec;

	/* Codes_SRS_FRAME_CODEC_01_049: [If any of the frame_codec or bytes arguments is NULL, frame_codec_encode_frame_bytes shall return a non-zero value.] */
	if ((frame_codec == NULL) ||
		(bytes == NULL) ||
		/* Codes_SRS_FRAME_CODEC_01_050: [If the length argument is zero, frame_codec_encode_frame_bytes shall return a non-zero value.] */
		(length == 0) ||
		/* Codes_SRS_FRAME_CODEC_01_053: [If length is bigger than the expected amount of bytes for the frame currently being encoded, then frame_codec_encode_frame_bytes shall return a non-zero value.] */
		(length > frame_codec_data->encode_frame_body_bytes_left))
	{
		result = __LINE__;
	}
	else
	{
		/* Codes_SRS_FRAME_CODEC_01_047: [frame_codec_encode_frame_bytes encodes the frame bytes for a frame encoding started with a frame_codec_start_encode_frame call.] */
		if (io_send(frame_codec_data->io, bytes, length) != 0)
		{
			/* Codes_SRS_FRAME_CODEC_01_094: [Once encoding has failed due to IO issues, all subsequent encoding calls to shall fail and return a non-zero value.] */
			frame_codec_data->encode_frame_state = ENCODE_FRAME_STATE_ERROR;

			/* Codes_SRS_FRAME_CODEC_01_054: [If any encoding error (passing the data to the IO interface) occurs, frame_codec_encode_frame_bytes shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_FRAME_CODEC_01_051: [The total number of bytes to be encoded for a frame can be passed in multiple frame_codec_encode_frame_bytes calls.] */
			/* Codes_SRS_FRAME_CODEC_01_052: [At each call, the frame_codec instance shall decrease the amount of bytes still needed to be encoded for the frame.] */
			frame_codec_data->encode_frame_body_bytes_left -= length;
			if (frame_codec_data->encode_frame_body_bytes_left == 0)
			{
				frame_codec_data->encode_frame_state = ENCODE_FRAME_STATE_FRAME_HEADER;
			}

			/* Codes_SRS_FRAME_CODEC_01_048: [If all bytes are successfully encoded, frame_codec_encode_frame_bytes shall return 0.] */
			result = 0;
		}
	}

	return result;
}
