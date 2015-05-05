#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#include "cstdbool"
#else
#include "stdbool.h"
#endif /* __cplusplus */

	typedef void* LIST_HANDLE;
	typedef bool LIST_MATCH_FUNCTION(void* list_item, void* compare_value);

	extern LIST_HANDLE list_create(void);
	extern void list_destroy(LIST_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIST_H */
