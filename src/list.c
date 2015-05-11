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

	/* Codes_SRS_LIST_01_001: [list_create shall create a new list and return a non-NULL handle on success.] */
	result = (LIST_DATA*)amqp_malloc(sizeof(LIST_DATA));
	if (result != NULL)
	{
		/* Codes_SRS_LIST_01_002: [If any error occurs during the list creation, list_create shall return NULL.] */
		result->head = NULL;
	}

	return result;
}

void list_destroy(LIST_HANDLE handle)
{
	/* Codes_SRS_LIST_01_004: [If the handle argument is NULL, no freeing of resources shall occur.] */
	if (handle != NULL)
	{
		/* Codes_SRS_LIST_01_003: [list_destroy shall free all resources associated with the list identified by the handle argument.] */
		amqp_free(handle);
	}
}

void* list_find(LIST_HANDLE handle, LIST_MATCH_FUNCTION match_function)
{
	return NULL;
}
