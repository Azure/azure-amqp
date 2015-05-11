#ifndef AMQP_ALLOC_H
#define AMQP_ALLOC_H

#ifdef __cplusplus
extern "C" {
#include "cstddef"
#else
#include "stddef.h"
#endif /* __cplusplus */

extern void* amqp_malloc(size_t size);
extern void amqp_free(void* ptr);
extern void* amqp_calloc(size_t nmemb, size_t size);
extern void* amqp_realloc(void* ptr, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_ALLOC_H */
