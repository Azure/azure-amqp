#include "sasl_plain.h"

SASL_MECHANISM_CONCRETE_HANDLE saslplain_create(void* config)
{
	return NULL;
}

void saslplain_destroy(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle)
{

}

int saslplain_get_init_bytes(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle, INIT_BYTES* init_bytes)
{
	return 0;
}

const SASL_MECHANISM_INTERFACE* saslplain_get_interface(void)
{
	return NULL;
}
