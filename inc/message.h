#ifndef MESSAGE_H
#define MESSAGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* MESSAGE_HANDLE;

	extern MESSAGE_HANDLE message_create(void);
	extern void message_destroy(MESSAGE_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGE_H */
