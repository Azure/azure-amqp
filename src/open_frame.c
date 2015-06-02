#include "open_frame.h"
#include "amqp_frame_codec.h"
#include "frame_codec.h"
#include "amqpvalue.h"

int open_frame_encode(FRAME_CODEC_HANDLE frame_codec, const char* container_id)
{
	int result;

	AMQP_VALUE open_frame_list;
	if ((open_frame_list = amqpvalue_create_list(1)) == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE container_id_value = amqpvalue_create_string(container_id);
		if (container_id_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE ulong_descriptor_value = amqpvalue_create_ulong(0x10);
			AMQP_VALUE performative = amqpvalue_create_described_value(ulong_descriptor_value, open_frame_list);

			if ((amqpvalue_set_list_item(open_frame_list, 0, container_id_value) != 0) ||
				(amqp_frame_codec_begin_encode_frame(frame_codec, 0, performative, 0) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(performative);
			amqpvalue_destroy(ulong_descriptor_value);
		}

		amqpvalue_destroy(container_id_value);
		amqpvalue_destroy(open_frame_list);
	}

	return result;
}
