#ifndef DECODER_H
#define DECODER_H

#include <stddef.h>
#include <stdbool.h>
#include "amqp_types.h"
#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* DECODER_HANDLE;
	typedef void(*VALUE_DECODED_CALLBACK)(void* context, AMQP_VALUE decoded_value);

	extern DECODER_HANDLE decoder_create(VALUE_DECODED_CALLBACK value_decoded_callback, void* value_decoded_callback_context);
	extern void decoder_destroy(DECODER_HANDLE handle);
	extern int decoder_decode_bytes(DECODER_HANDLE handle, const unsigned char* buffer, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DECODER_H */
