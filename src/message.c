#include <stdlib.h>
#include "message.h"
#include "amqpvalue.h"

typedef struct MESSAGE_DATA_TAG
{
	AMQP_VALUE to;
} MESSAGE_DATA;

MESSAGE_HANDLE message_create(void)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)malloc(sizeof(MESSAGE_DATA));
	if (result != NULL)
	{
		result->to = NULL;
	}

	return result;

}

void message_destroy(MESSAGE_HANDLE handle)
{
	free(handle);
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
		result = message->to;
	}

	return result;
}
