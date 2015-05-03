#include <stdlib.h>
#include "message.h"
#include "amqpvalue.h"

typedef struct MESSAGE_DATA_TAG
{
	AMQP_VALUE application_data;
} MESSAGE_DATA;

MESSAGE_HANDLE message_create(void)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)malloc(sizeof(MESSAGE_DATA));
	if (result != NULL)
	{
		result->application_data = NULL;
	}

	return result;

}

void message_destroy(MESSAGE_HANDLE handle)
{
	free(handle);
}
