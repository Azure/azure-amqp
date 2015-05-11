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

int list_add(LIST_HANDLE handle, void* item)
{
	int result;

	/* Codes_SRS_LIST_01_006: [If any of the arguments is NULL, list_add shall not add the item to the list and return a non-zero value.] */
	if ((handle == NULL) ||
		(item == NULL))
	{
		result = __LINE__;
	}
	else
	{
		LIST_DATA* list = (LIST_DATA*)handle;
		LIST_ITEM* new_item = amqp_malloc(sizeof(LIST_ITEM));

		if (new_item == NULL)
		{
			/* Codes_SRS_LIST_01_007: [If allocating the new list node fails, list_add shall return a non-zero value.] */
			result = __LINE__;
		}
		else
		{
			/* Codes_SRS_LIST_01_005: [list_add shall add one item to the tail of the list and on success it shall return 0.] */
			new_item->next = NULL;
			new_item->item = item;

			if (list->head == NULL)
			{
				list->head = new_item;
			}
			else
			{
				LIST_ITEM* current = list->head;
				while (current->next != NULL)
				{
					current = current->next;
				}

				current->next = new_item;
			}

			result = 0;
		}
	}

	return result;
}

void* list_get_head(LIST_HANDLE handle)
{
	LIST_DATA* list = (LIST_DATA*)handle;
	void* result;
	
	if (list->head == NULL)
	{
		result = NULL;
	}
	else
	{
		LIST_ITEM* head = list->head;
		result = head->item;
		list->head = list->head->next;
		amqp_free(head);
	}

	return result;
}

void* list_find(LIST_HANDLE handle, void* look_for, LIST_MATCH_FUNCTION match_function)
{
	return NULL;
}
