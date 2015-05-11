#include <stdbool.h>
#include "list.h"
#include "amqp_alloc.h"

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
	result = (LIST_DATA*)amqp_malloc(sizeof(LIST_DATA));
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
		amqp_free(handle);
	}
}

void* list_find(LIST_HANDLE handle, LIST_MATCH_FUNCTION match_function)
{
	return NULL;
}
