#include <string.h>
#include "sasl_plain.h"
#include "amqpalloc.h"

typedef struct SASL_PLAIN_INSTANCE_TAG
{
	char* authcid;
	char* passwd;
} SASL_PLAIN_INSTANCE;

static const SASL_MECHANISM_INTERFACE_DESCRIPTION saslplain_interface =
{
	saslplain_create,
	saslplain_destroy,
	saslplain_get_init_bytes
};

SASL_MECHANISM_CONCRETE_HANDLE saslplain_create(void* config)
{
	SASL_PLAIN_INSTANCE* result = amqpalloc_malloc(sizeof(SASL_PLAIN_INSTANCE));
	if (result != NULL)
	{
		SASL_PLAIN_CONFIG* sasl_plain_config = (SASL_PLAIN_CONFIG*)config;
		size_t authcid_length = strlen(sasl_plain_config->authcid);
		size_t passwd_length = strlen(sasl_plain_config->passwd);
		result->authcid = (char*)amqpalloc_malloc(authcid_length + 1);
		if (result->authcid == NULL)
		{
			amqpalloc_free(result);
			result = NULL;
		}
		else
		{
			result->passwd = (char*)amqpalloc_malloc(passwd_length + 1);
			if (result->passwd == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				strcpy(result->authcid, sasl_plain_config->authcid);
				strcpy(result->passwd, sasl_plain_config->passwd);
			}
		}
	}

	return result;
}

void saslplain_destroy(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle)
{
	if (sasl_mechanism_concrete_handle != NULL)
	{
		SASL_PLAIN_INSTANCE* sasl_plain_instance = (SASL_PLAIN_INSTANCE*)sasl_mechanism_concrete_handle;
		if (sasl_plain_instance->authcid != NULL)
		{
			amqpalloc_free(sasl_plain_instance->authcid);
		}

		if (sasl_plain_instance->passwd != NULL)
		{
			amqpalloc_free(sasl_plain_instance->passwd);
		}
	}
}

int saslplain_get_init_bytes(SASL_MECHANISM_CONCRETE_HANDLE sasl_mechanism_concrete_handle, INIT_BYTES* init_bytes)
{
	return 0;
}

const SASL_MECHANISM_INTERFACE_DESCRIPTION* saslplain_get_interface(void)
{
	return &saslplain_interface;
}
