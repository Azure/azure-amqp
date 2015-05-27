#include <stdlib.h>
#ifdef _CRT_DBG_MAP_ALLOC
#include <crtdbg.h>
#endif /* _CRT_DBG_MAP_ALLOC */
#include <stdbool.h>
#include <stdint.h>
#include "amqpalloc.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)~(size_t)0)
#endif

static bool alloc_trace = false;

typedef struct ALLOCATION_TAG
{
	size_t size;
	void* ptr;
	void* next;
} ALLOCATION;

static ALLOCATION* head = NULL;
static size_t total_size = 0;
static size_t max_size = 0;

#ifndef DISABLE_MEMORY_TRACE

void* trace_malloc(size_t size)
{
	void* result;

	ALLOCATION* allocation = (ALLOCATION*)malloc(sizeof(ALLOCATION));
	if (allocation == NULL)
	{
		result = NULL;
	}
	else
	{
		result = malloc(size);
		if (result == NULL)
		{
			free(allocation);
		}
		else
		{
			allocation->ptr = result;
			allocation->size = size;
			allocation->next = head;
			head = allocation;

			total_size += size;
			if (max_size < total_size)
			{
				max_size = total_size;
			}
		}
	}

	return result;
}

void* trace_calloc(size_t nmemb, size_t size)
{
	void* result;

	ALLOCATION* allocation = (ALLOCATION*)malloc(sizeof(ALLOCATION));
	if (allocation == NULL)
	{
		result = NULL;
	}
	else
	{
		result = calloc(nmemb, size);
		if (result == NULL)
		{
			free(allocation);
		}
		else
		{
			allocation->ptr = result;
			allocation->size = nmemb * size;
			allocation->next = head;
			head = allocation;

			total_size += allocation->size;
			if (max_size < total_size)
			{
				max_size = total_size;
			}
		}
	}

	return result;
}

void* trace_realloc(void* ptr, size_t size)
{
	ALLOCATION* curr;
	void* result;
	ALLOCATION* allocation = NULL;

	if (ptr == NULL)
	{
		allocation = (ALLOCATION*)malloc(sizeof(ALLOCATION));
	}
	else
	{
		curr = head;
		while (curr != NULL)
		{
			if (curr->ptr == ptr)
			{
				allocation = curr;
				break;
			}
			else
			{
				curr = (ALLOCATION*)curr->next;
			}
		}
	}

	if (allocation == NULL)
	{
		result = NULL;
	}
	else
	{
		result = realloc(ptr, size);
		if (result == NULL)
		{
			if (ptr == NULL)
			{
				free(allocation);
			}
		}
		else
		{
			if (ptr != NULL)
			{
				allocation->ptr = result;
				total_size -= allocation->size;
				allocation->size = size;
			}
			else
			{
				/* add block */
				allocation->ptr = result;
				allocation->size = size;
				allocation->next = head;
				head = allocation;
			}

			total_size += size;

			if (max_size < total_size)
			{
				max_size = total_size;
			}
		}
	}

	return result;
}

void trace_free(void* ptr)
{
	ALLOCATION* curr = head;
	ALLOCATION* prev = NULL;

	while (curr != NULL)
	{
		if (curr->ptr == ptr)
		{
			free(ptr);
			total_size -= curr->size;
			if (prev != NULL)
			{
				prev->next = curr->next;
			}
			else
			{
				head = (ALLOCATION*)curr->next;
			}

			free(curr);
			break;
		}

		prev = curr;
		curr = (ALLOCATION*)curr->next;
	}
}

void* amqpalloc_malloc(size_t size)
{
	void* result;
	if (!alloc_trace)
	{
		result = malloc(size);
	}
	else
	{
		result = trace_malloc(size);
	}

	return result;
}

void amqpalloc_free(void* ptr)
{
	if (!alloc_trace)
	{
		free(ptr);
	}
	else
	{
		trace_free(ptr);
	}
}

void* amqpalloc_calloc(size_t nmemb, size_t size)
{
	void* result;

	if (!alloc_trace)
	{
		result = calloc(nmemb, size);
	}
	else
	{
		result = trace_calloc(nmemb, size);
	}

	return result;
}

void* amqpalloc_realloc(void* ptr, size_t size)
{
	void* result;

	if (!alloc_trace)
	{
		result = realloc(ptr, size);
	}
	else
	{
		result = trace_realloc(ptr, size);
	}

	return result;
}

#endif

size_t amqpalloc_get_maximum_memory_used(void)
{
    return max_size;
}

size_t amqpalloc_get_current_memory_used(void)
{
	return total_size;
}

void amqpalloc_set_memory_tracing_enabled(bool memory_tracing_enabled)
{
	alloc_trace = memory_tracing_enabled;
}
