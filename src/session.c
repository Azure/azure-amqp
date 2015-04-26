#include <stdlib.h>
#include "session.h"
#include "connection.h"

typedef enum SESION_STATE_TAG
{
	SESSION_STATE_UNMAPPED,
	SESSION_STATE_BEGIN_SENT,
	SESSION_STATE_BEGIN_RCVD,
	SESSION_STATE_MAPPED,
	SESSION_STATE_END_SENT,
	SESSION_STATE_END_RCVD,
	SESSION_STATE_DISCARDING
} SESSION_STATE;

typedef struct SESSION_DATA_TAG
{
	CONNECTION_HANDLE connection;
	SESSION_STATE session_state;
} SESSION_DATA;

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_DATA* result = malloc(sizeof(SESSION_DATA));
	if (result != NULL)
	{
		result->session_state = SESSION_STATE_UNMAPPED;
		result->connection = connection;
	}

	return result;
}

void session_destroy(SESSION_HANDLE handle)
{
	free(handle);
}

static int send_begin(SESSION_DATA* session_data)
{
	return 0;
}

int session_dowork(SESSION_HANDLE handle)
{
	int result;
	SESSION_DATA* session_data = (SESSION_DATA*)handle;

	switch (session_data->session_state)
	{
		default:
			result = __LINE__;
			break;

		case SESSION_STATE_UNMAPPED:
			result = send_begin(session_data);
			break;
	}

	return result;
}
