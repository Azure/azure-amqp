#ifndef DECODER_H
#define DECODER_H

#include <stddef.h>
#include "amqp_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* DECODER_HANDLE;

	extern DECODER_HANDLE decoder_create(void* buffer, size_t length);
	extern void decoder_destroy(DECODER_HANDLE handle);
	extern int decoder_decode(DECODER_HANDLE handle, AMQP_VALUE* amqp_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DECODER_H */
