#include <stdlib.h>
#include "session.h"
#include "connection.h"

typedef struct SESSION_DATA_TAG
{
	CONNECTION_HANDLE connection;
} SESSION_DATA;

SESSION_HANDLE session_create(CONNECTION_HANDLE connection)
{
	SESSION_DATA* result = malloc(sizeof(SESSION_DATA));
	if (result != NULL)
	{
		result->connection = connection;
	}

	return result;
}

void session_destroy(SESSION_HANDLE handle)
{
	free(handle);
}

int session_dowork(SESSION_HANDLE handle)
{
	return 0;
}
