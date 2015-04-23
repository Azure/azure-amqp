#ifndef ENCODER_H
#define ENCODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* ENCODER_HANDLE;
	typedef int (*ENCODER_OUTPUT)(void* context, const void* bytes, size_t length);

	extern ENCODER_HANDLE encoder_create(ENCODER_OUTPUT encoderOutput, void* context);
	extern void encoder_destroy(ENCODER_HANDLE handle);
	extern int encoder_encode_string(ENCODER_HANDLE handle, const char* value);
	extern int encoder_encode_ulong(ENCODER_HANDLE handle, uint64_t value);
	extern int encoder_get_encoded_size(ENCODER_HANDLE handle, size_t* size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENCODER_H */
