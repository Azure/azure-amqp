#include <stdlib.h>
#include "amqp_types.h"
#include "amqpvalue.h"

typedef union AMQP_VALUE_UNION_TAG
{
	AMQP_VALUE descriptor;
} AMQP_VALUE_UNION;

typedef struct AMQP_VALUE_DATA_TAG
{
	AMQP_TYPE type;
	AMQP_VALUE_UNION value;
} AMQP_VALUE_DATA;

AMQP_VALUE amqpvalue_create_descriptor(AMQP_VALUE value)
{
	AMQP_VALUE_DATA* result = (AMQP_VALUE_DATA*)malloc(sizeof(AMQP_VALUE_DATA));
	if (result != NULL)
	{
		result->type = AMQP_TYPE_DESCRIPTOR;
		result->value.descriptor = value;
	}
	return result;
}

void amqpvalue_destroy(AMQP_VALUE value)
{
	if (value != NULL)
	{
		free(value);
	}
}
