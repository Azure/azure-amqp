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
	typedef struct BINARY_DATA_TAG
	{
		const unsigned char* bytes;
		size_t length;
	} BINARY_DATA;

	extern MESSAGE_HANDLE message_create(void);
	extern void message_destroy(MESSAGE_HANDLE handle);
	extern int message_set_to(MESSAGE_HANDLE handle, const char* to);
	extern const char* message_get_to(MESSAGE_HANDLE handle);
	extern int message_set_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA binary_data);
	extern int message_get_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA* binary_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGE_H */
