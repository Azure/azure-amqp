#include "open_frame.h"
#include "amqp_frame_codec.h"
#include "amqpvalue.h"

AMQP_OPEN_FRAME_HANDLE open_frame_create(const char* container_id)
{
	AMQP_OPEN_FRAME_HANDLE result;
	AMQP_VALUE ulong_descriptor_value = amqpvalue_create_ulong(AMQP_OPEN);
	AMQP_VALUE open_frame_list = amqpvalue_create_list(1);
	AMQP_VALUE performative;

	if ((ulong_descriptor_value == NULL) ||
		(open_frame_list == NULL))
	{
		amqpvalue_destroy(ulong_descriptor_value);
		amqpvalue_destroy(open_frame_list);
		result = NULL;
	}
	else
	{
		performative = amqpvalue_create_described_value(ulong_descriptor_value, open_frame_list);
		if (performative == NULL)
		{
			amqpvalue_destroy(ulong_descriptor_value);
			amqpvalue_destroy(open_frame_list);
			result = NULL;
		}
		else
		{
			result = (AMQP_OPEN_FRAME_HANDLE)performative;
		}
	}

	return result;
}

void open_frame_destroy(AMQP_OPEN_FRAME_HANDLE amqp_open_frame)
{
	if (amqp_open_frame != NULL)
	{
		amqpvalue_destroy(amqp_open_frame);
	}
}

int open_frame_encode(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, AMQP_FRAME_CODEC_HANDLE frame_codec)
{
	int result;

	if (amqp_frame_codec_begin_encode_frame(frame_codec, 0, (AMQP_VALUE)amqp_open_frame, 0) != 0)
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

int open_set_max_frame_size(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint32_t max_frame_size)
{
	return 0;
}

int open_get_max_frame_size(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint32_t* max_frame_size)
{
	return 0;
}

int open_set_channel_max(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint16_t channel_max)
{
	return 0;
}

int open_get_channel_max(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint16_t* channel_max)
{
	return 0;
}

int open_set_idle_timeout(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, milliseconds channel_max)
{
	return 0;
}

int open_get_idle_timeout(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint16_t* channel_max)
{
	return 0;
}
