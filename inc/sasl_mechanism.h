#ifndef SASL_MECHANISM_H
#define SASL_MECHANISM_H

#include "link.h"
#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* SASL_MECHANISM_HANDLE;
	typedef void* SASL_MECHANISM_CONCRETE_HANDLE;
	typedef void(*ON_MESSAGE_RECEIVED)(const void* context, MESSAGE_HANDLE message);

	typedef struct INIT_BYTES_TAG
	{
		const void* bytes;
		size_t length;
	} INIT_BYTES;

	typedef SASL_MECHANISM_CONCRETE_HANDLE(*SASL_MECHANISM_CREATE)(void* config);
	typedef void(*SASL_MECHANISM_DESTROY)(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle);
	typedef int(*SASL_MECHANISM_GET_INIT_BYTES)(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle, INIT_BYTES* init_bytes);

	typedef struct SASL_MECHANISM_INTERFACE_TAG
	{
		SASL_MECHANISM_CREATE sasl_mechanism_concrete_create;
		SASL_MECHANISM_DESTROY sasl_mechanism_concrete_destroy;
		SASL_MECHANISM_GET_INIT_BYTES sasl_mechanism_concrete_get_init_bytes;
	} SASL_MECHANISM_INTERFACE_DESCRIPTION;

	extern SASL_MECHANISM_HANDLE saslmechanism_create(const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface_description, void* sasl_mechanism_create_parameters);
	extern void saslmechanism_destroy(SASL_MECHANISM_HANDLE sasl_mechanism);
	extern int saslmechanism_get_init_bytes(SASL_MECHANISM_HANDLE sasl_mechanism, INIT_BYTES* init_bytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SASL_MECHANISM_H */
