#include "open_frame.h"
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
			if ((amqpvalue_set_list_item(open_frame_list, 0, container_id_value) != 0) ||
				(frame_codec_encode(frame_codec, 0x10, &open_frame_list, 1) != 0))
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(container_id_value);
		}

		amqpvalue_destroy(open_frame_list);
	}

	return result;
}
