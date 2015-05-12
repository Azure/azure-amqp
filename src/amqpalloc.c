#include <stdlib.h>
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
static size_t totalSize = 0;
static size_t maxSize = 0;

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

			totalSize += size;
			if (maxSize < totalSize)
			{
				maxSize = totalSize;
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

			totalSize += allocation->size;
			if (maxSize < totalSize)
			{
				maxSize = totalSize;
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
				totalSize -= allocation->size;
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

			totalSize += size;

			if (maxSize < totalSize)
			{
				maxSize = totalSize;
			}
		}
	}

	return result;
}

void gballoc_free(void* ptr)
{
	ALLOCATION* curr = head;
	ALLOCATION* prev = NULL;

	while (curr != NULL)
	{
		if (curr->ptr == ptr)
		{
			free(ptr);
			totalSize -= curr->size;
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

	if ((curr == NULL) && (ptr != NULL))
	{
		/* could not find the allocation */
	}
}

void* amqpalloc_malloc(size_t size)
{
	void* result;
	if (alloc_trace)
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
	free(ptr);
}

void* amqpalloc_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

void* amqpalloc_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}

#if 0

int gballoc_init(void)
{
    int result;

    if (gballocState != GBALLOC_STATE_NOT_INIT)
    {
        /* Codes_SRS_GBALLOC_01_025: [Init after Init shall fail and return a non-zero value.] */
        result = __LINE__;
    }
    /* Codes_SRS_GBALLOC_01_026: [gballoc_Init shall create a lock handle that will be used to make the other gballoc APIs thread-safe.] */
    else if ((gballocThreadSafeLock = Lock_Init()) == NULL)
    {
        /* Codes_SRS_GBALLOC_01_027: [If the Lock creation fails, gballoc_init shall return a non-zero value.]*/
        result = __LINE__;
    }
    else
    {
        gballocState = GBALLOC_STATE_INIT;

        /* Codes_ SRS_GBALLOC_01_002: [Upon initialization the total memory used and maximum total memory used tracked by the module shall be set to 0.] */
        totalSize = 0;
        maxSize = 0;

        /* Codes_SRS_GBALLOC_01_024: [gballoc_init shall initialize the gballoc module and return 0 upon success.] */
        result = 0;
    }

    return result;
}

void gballoc_deinit(void)
{
    if (gballocState == GBALLOC_STATE_INIT)
    {
        /* Codes_SRS_GBALLOC_01_028: [gballoc_deinit shall free all resources allocated by gballoc_init.] */
        (void)Lock_Deinit(gballocThreadSafeLock);
    }

    gballocState = GBALLOC_STATE_NOT_INIT;
}

size_t gballoc_getMaximumMemoryUsed(void)
{
    size_t result;

    /* Codes_SRS_GBALLOC_01_038: [If gballoc was not initialized gballoc_getMaximumMemoryUsed shall return MAX_INT_SIZE.] */
    if (gballocState != GBALLOC_STATE_INIT)
    {
        LogError("gballoc is not initialized.\r\n");
        result = SIZE_MAX;
    }
    /* Codes_SRS_GBALLOC_01_034: [gballoc_getMaximumMemoryUsed shall ensure thread safety by using the lock created by gballoc_Init.]  */
    else if (LOCK_OK != Lock(gballocThreadSafeLock))
    {
        /* Codes_SRS_GBALLOC_01_050: [If the lock cannot be acquired, gballoc_getMaximumMemoryUsed shall return SIZE_MAX.]  */
        LogError("Failed to get the Lock.\r\n");
        result = SIZE_MAX;
    }
    else
    {
    /* Codes_SRS_GBALLOC_01_010: [gballoc_getMaximumMemoryUsed shall return the maximum amount of total memory used recorded since the module initialization.] */
        result = maxSize;
        Unlock(gballocThreadSafeLock);
}

    return result;
}

size_t gballoc_getCurrentMemoryUsed(void)
{
    size_t result;

    /* Codes_SRS_GBALLOC_01_044: [If gballoc was not initialized gballoc_getCurrentMemoryUsed shall return SIZE_MAX.] */
    if (gballocState != GBALLOC_STATE_INIT)
    {
        LogError("gballoc is not initialized.\r\n");
        result = SIZE_MAX;
    }
    /* Codes_SRS_GBALLOC_01_036: [gballoc_getCurrentMemoryUsed shall ensure thread safety by using the lock created by gballoc_Init.]*/
    else if (LOCK_OK != Lock(gballocThreadSafeLock))
    {
        /* Codes_SRS_GBALLOC_01_051: [If the lock cannot be acquired, gballoc_getCurrentMemoryUsed shall return SIZE_MAX.] */
        LogError("Failed to get the Lock.\r\n");
        result = SIZE_MAX;
    }
    else
    {
    /*Codes_SRS_GBALLOC_02_001: [gballoc_getCurrentMemoryUsed shall return the currently used memory size.] */
        result = totalSize;
        Unlock(gballocThreadSafeLock);
    }

    return result;
}

#endif
