#include <stdlib.h>
#include "message.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

typedef struct MESSAGE_DATA_TAG
{
	AMQP_VALUE to;
	unsigned char* body_data_section_bytes;
	size_t body_data_section_length;
} MESSAGE_DATA;

MESSAGE_HANDLE message_create(void)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)amqpalloc_malloc(sizeof(MESSAGE_DATA));
	if (result != NULL)
	{
		result->to = NULL;
		result->body_data_section_bytes = NULL;
		result->body_data_section_length = 0;
	}

	return result;

}

void message_destroy(MESSAGE_HANDLE handle)
{
	if (handle != NULL)
	{
		MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
		amqpvalue_destroy(message->to);
		amqpalloc_free(message->body_data_section_bytes);
		amqpalloc_free(handle);
	}
}

int message_set_to(MESSAGE_HANDLE handle, const char* to)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = __LINE__;
	}
	else
	{
		message->to = amqpvalue_create_string(to);
		if (message->to == NULL)
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

const char* message_get_to(MESSAGE_HANDLE handle)
{
	const char* result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = NULL;
	}
	else
	{
		if (amqpvalue_get_string(message->to, &result) != 0)
		{
			result = NULL;
		}
	}

	return result;
}

int message_set_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA binary_data)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = __LINE__;
	}
	else
	{
		message->body_data_section_bytes = (unsigned char*)malloc(binary_data.length);
		message->body_data_section_length = binary_data.length;
		result = 0;
	}

	return result;
}

int message_get_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA* binary_data)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if ((message == NULL) ||
		(binary_data == NULL))
	{
		result = __LINE__;
	}
	else
	{
		binary_data->bytes = message->body_data_section_bytes;
		binary_data->length = message->body_data_section_length;

		result = 0;
	}

	return result;
}
