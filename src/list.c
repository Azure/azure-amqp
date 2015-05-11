#include <stdbool.h>
#include "list.h"
#include "amqp_alloc.h"

typedef struct LIST_ITEM_TAG
{
	const void* item;
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

const void* list_get_head(LIST_HANDLE handle)
{
	LIST_DATA* list = (LIST_DATA*)handle;
	const void* result;
	
	if (list == NULL)
	{
		/* Codes_SRS_LIST_01_009: [If the handle argument is NULL, list_get_head shall return NULL.] */
		result = NULL;
	}
	else
	{
		/* Codes_SRS_LIST_01_008: [list_get_head shall return the head of the list and remove the retrieved item from the list.] */
		LIST_ITEM* head = list->head;

		if (head == NULL)
		{
			/* Codes_SRS_LIST_01_010: [If the list is empty, list_get_head_shall_return NULL.] */
			result = NULL;
		}
		else
		{
			result = head->item;
			list->head = list->head->next;
			amqp_free(head);
		}
	}

	return result;
}

const void* list_find(LIST_HANDLE handle, LIST_MATCH_FUNCTION match_function, const void* match_context)
{
	const void* result;

	if ((handle == NULL) ||
		(match_function == NULL))
	{
		/* Codes_SRS_LIST_01_012: [If the handle or the match_function argument is NULL, list_find shall return NULL.] */
		result = NULL;
	}
	else
	{
		LIST_DATA* list = (LIST_DATA*)handle;
		LIST_ITEM* current = list->head;

		/* Codes_SRS_LIST_01_011: [list_find shall iterate through all items in a list and return the one that satisfies a certain match function.] */
		while (current != NULL)
		{
			/* Codes_SRS_LIST_01_014: [list find shall determine whether an item satisfies the match criteria by invoking the match function for each item in the list until a matching item is found.] */
			/* Codes_SRS_LIST_01_013: [The match_function shall get as arguments the list item being attempted to be matched and the match_context as is.] */
			if (match_function(current->item, match_context) == true)
			{
				/* Codes_SRS_LIST_01_017: [If the match function returns true, list_find shall consider that item as matching.] */
				break;
			}

			/* Codes_SRS_LIST_01_016: [If the match function returns false, list_find shall consider that item as not matching.] */
			current = current->next;
		}

		if (current == NULL)
		{
			/* Codes_SRS_LIST_01_015: [If the list is empty, list_find shall return NULL.] */
			result = NULL;
		}
		else
		{
			result = current->item;
		}
	}

	return result;
}
