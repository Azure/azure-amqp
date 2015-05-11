#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#include "cstdbool"
#else
#include "stdbool.h"
#endif /* __cplusplus */

	typedef void* LIST_HANDLE;
	typedef bool LIST_MATCH_FUNCTION(const void* list_item, const void* match_context);

	extern LIST_HANDLE list_create(void);
	extern void list_destroy(LIST_HANDLE handle);
	extern int list_add(LIST_HANDLE handle, const void* item);
	extern const void* list_get_head(LIST_HANDLE handle);
	extern const void* list_find(LIST_HANDLE handle, LIST_MATCH_FUNCTION match_function, const void* match_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIST_H */
