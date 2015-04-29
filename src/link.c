#include <stdlib.h>
#include "link.h"
#include "session.h"

typedef struct LINK_DATA_TAG
{
	SESSION_HANDLE session;
} LINK_DATA;

LINK_HANDLE link_create(SESSION_HANDLE session)
{
	LINK_DATA* result = malloc(sizeof(LINK_DATA));
	if (result != NULL)
	{
		result->session = session;
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
