#include <stdlib.h>
#include <stdbool.h>
#include "list.h"

typedef struct LIST_ITEM_TAG
{
	void* item;
	void* next;
} LIST_ITEM;

typedef struct LIST_DATA_TAG
{
	LIST_ITEM* head;
} LIST_DATA;

LIST_HANDLE list_create(void)
{
	LIST_DATA* result;
	result = (LIST_DATA*)malloc(sizeof(LIST_DATA));
	if (result != NULL)
	{
		result->head = NULL;
	}

	return result;
}

void list_destroy(LIST_HANDLE handle)
{
	if (handle != NULL)
	{
		free(handle);
	}
}

void* list_find(LIST_HANDLE handle, LIST_MATCH_FUNCTION match_function)
{
	if (handle != NULL)
	{
		free(handle);
	}

	return NULL;
}
