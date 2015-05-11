#include <stdlib.h>
#include "amqp_alloc.h"

void* amqp_malloc(size_t size)
{
	return malloc(size);
}

void amqp_free(void* ptr)
{
	free(ptr);
}

void* amqp_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

void* amqp_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}
