#ifndef ENCODER_H
#define ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* ENCODER_HANDLE;

	extern ENCODER_HANDLE encoder_create(void);
	extern void encoder_destroy(ENCODER_HANDLE handle);
	extern int encoder_encode_string(ENCODER_HANDLE handle, const char* value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENCODER_H */
