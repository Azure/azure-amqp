#include <stdint.h>
#include <stdlib.h>
#include "link.h"
#include "session.h"
#include "amqpvalue.h"

typedef struct LINK_DATA_TAG
{
	SESSION_HANDLE session;
} LINK_DATA;

static void link_frame_received(void* context, uint64_t performative, AMQP_VALUE frame_list_value)
{

}

LINK_HANDLE link_create(SESSION_HANDLE session)
{
	LINK_DATA* result = malloc(sizeof(LINK_DATA));
	if (result != NULL)
	{
		if (session_set_frame_received_callback(session, link_frame_received, result) != 0)
		{
			free(result);
			result = NULL;
		}
		else
		{
			result->session = session;
		}
	}

	return result;
}

void link_destroy(LINK_HANDLE handle)
{
	free(handle);
}

int link_dowork(LINK_HANDLE handle)
{
	int result = 0;

	return result;
}
