#include "sasl_mechanism.h"

SASL_MECHANISM_HANDLE saslmechanism_create(const SASL_MECHANISM_INTERFACE* sasl_mechanism_interface, void* sasl_mechanism_config)
{
	return NULL;
}

void saslmechanism_destroy(SASL_MECHANISM_HANDLE sasl_mechanism)
{

}

int saslmechanism_get_init_bytes(SASL_MECHANISM_HANDLE sasl_mechanism, INIT_BYTES* init_bytes)
{
	return 0;
}
