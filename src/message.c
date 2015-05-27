#include <stdlib.h>
#include "message.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

typedef struct MESSAGE_DATA_TAG
{
	AMQP_VALUE to;
	AMQP_VALUE body;
} MESSAGE_DATA;

MESSAGE_HANDLE message_create(void)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)amqpalloc_malloc(sizeof(MESSAGE_DATA));
	if (result != NULL)
	{
		result->to = NULL;
		result->body = NULL;
	}

	return result;

}

void message_destroy(MESSAGE_HANDLE handle)
{
	if (handle != NULL)
	{
		MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
		amqpvalue_destroy(message->to);
		amqpvalue_destroy(message->body);
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
		result = amqpvalue_get_string(message->to);
	}

	return result;
}

int message_set_body(MESSAGE_HANDLE handle, AMQP_VALUE body)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = __LINE__;
	}
	else
	{
		message->body = body;
		result = 0;
	}

	return result;
}

AMQP_VALUE message_get_body(MESSAGE_HANDLE handle)
{
	AMQP_VALUE result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = NULL;
	}
	else
	{
		result = message->body;
		message->body = NULL;
	}

	return result;
}
