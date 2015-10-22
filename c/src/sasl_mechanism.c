#include "sasl_mechanism.h"
#include "amqpalloc.h"

typedef struct SASL_MECHANISM_INSTANCE_TAG
{
	const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface_description;
	SASL_MECHANISM_CONCRETE_HANDLE concrete_sasl_mechanism_handle;
} SASL_MECHANISM_INSTANCE;

SASL_MECHANISM_HANDLE saslmechanism_create(const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface_description, void* sasl_mechanism_create_parameters)
{
	SASL_MECHANISM_INSTANCE* sasl_mechanism_instance;

	if ((sasl_mechanism_interface_description == NULL) ||
		(sasl_mechanism_interface_description->sasl_mechanism_concrete_create == NULL) ||
		(sasl_mechanism_interface_description->sasl_mechanism_concrete_destroy == NULL) ||
		(sasl_mechanism_interface_description->sasl_mechanism_concrete_get_init_bytes == NULL))
	{
		sasl_mechanism_instance = NULL;
	}
	else
	{
		sasl_mechanism_instance = (SASL_MECHANISM_INSTANCE*)amqpalloc_malloc(sizeof(SASL_MECHANISM_INSTANCE));

		if (sasl_mechanism_instance != NULL)
		{
			sasl_mechanism_instance->sasl_mechanism_interface_description = sasl_mechanism_interface_description;
			sasl_mechanism_instance->concrete_sasl_mechanism_handle = sasl_mechanism_instance->sasl_mechanism_interface_description->sasl_mechanism_concrete_create((void*)sasl_mechanism_create_parameters);

			if (sasl_mechanism_instance->concrete_sasl_mechanism_handle == NULL)
			{
				amqpalloc_free(sasl_mechanism_instance);
				sasl_mechanism_instance = NULL;
			}
		}
	}

	return (SASL_MECHANISM_HANDLE)sasl_mechanism_instance;
}

void saslmechanism_destroy(SASL_MECHANISM_HANDLE sasl_mechanism)
{
	if (sasl_mechanism != NULL)
	{
		SASL_MECHANISM_INSTANCE* sasl_mechanism_instance = (SASL_MECHANISM_INSTANCE*)sasl_mechanism;
		sasl_mechanism_instance->sasl_mechanism_interface_description->sasl_mechanism_concrete_destroy(sasl_mechanism_instance->concrete_sasl_mechanism_handle);
		amqpalloc_free(sasl_mechanism_instance);
	}
}

int saslmechanism_get_init_bytes(SASL_MECHANISM_HANDLE sasl_mechanism, INIT_BYTES* init_bytes)
{
	int result;

	if (sasl_mechanism == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_MECHANISM_INSTANCE* sasl_mechanism_instance = (SASL_MECHANISM_INSTANCE*)sasl_mechanism;

		if (sasl_mechanism_instance->sasl_mechanism_interface_description->sasl_mechanism_concrete_get_init_bytes(sasl_mechanism_instance->concrete_sasl_mechanism_handle, init_bytes) != 0)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}
