#ifndef AMQP_ALLOC_H
#define AMQP_ALLOC_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdbool>
#include <cstdlib>
#else
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#endif /* __cplusplus */

#ifndef DISABLE_MEMORY_TRACE
extern void* amqpalloc_malloc(size_t size);
extern void amqpalloc_free(void* ptr);
extern void* amqpalloc_calloc(size_t nmemb, size_t size);
extern void* amqpalloc_realloc(void* ptr, size_t size);
#else
#define amqpalloc_malloc malloc
#define amqpalloc_free free
#define amqpalloc_calloc calloc
#define amqpalloc_realloc realloc
#endif

extern size_t amqpalloc_get_maximum_memory_used(void);
extern size_t amqpalloc_get_current_memory_used(void);
extern void amqpalloc_set_memory_tracing_enabled(bool memory_tracing_enabled);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_ALLOC_H */
