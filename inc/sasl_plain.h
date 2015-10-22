#ifndef SASL_PLAIN_H
#define SASL_PLAIN_H

#include "sasl_mechanism.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef struct SASL_PLAIN_CONFIG_TAG
	{
		const char* authcid;
		const char* passwd;
	} SASL_PLAIN_CONFIG;

	extern SASL_MECHANISM_CONCRETE_HANDLE saslplain_create(void* config);
	extern void saslplain_destroy(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle);
	extern int saslplain_get_init_bytes(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle, INIT_BYTES* init_bytes);
	extern const SASL_MECHANISM_INTERFACE* saslplain_get_interface(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SASL_PLAIN_H */
