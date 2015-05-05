#ifndef MESSAGE_H
#define MESSAGE_H

#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

	typedef void* MESSAGE_HANDLE;

	extern MESSAGE_HANDLE message_create(void);
	extern void message_destroy(MESSAGE_HANDLE handle);
	extern int message_set_to(MESSAGE_HANDLE handle, const char* to);
	extern const char* message_get_to(MESSAGE_HANDLE handle);
	extern int message_set_body(MESSAGE_HANDLE handle, AMQP_VALUE body);
	extern AMQP_VALUE message_get_body(MESSAGE_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGE_H */
