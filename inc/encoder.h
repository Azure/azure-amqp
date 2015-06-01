#ifndef ENCODER_H
#define ENCODER_H

#include "amqpvalue.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#include <cstdbool>
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif /* __cplusplus */

	typedef void* ENCODER_HANDLE;
	typedef int (*ENCODER_OUTPUT)(void* context, const void* bytes, size_t length);

	extern ENCODER_HANDLE encoder_create(ENCODER_OUTPUT encoderOutput, void* context);
	extern void encoder_destroy(ENCODER_HANDLE handle);
	extern int encoder_get_encoded_size(ENCODER_HANDLE handle, size_t* size);
	extern int encoder_encode_amqp_value(ENCODER_HANDLE handle, AMQP_VALUE value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENCODER_H */
