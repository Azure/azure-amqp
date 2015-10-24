#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "amqpvalue.h"
#include "amqp_definitions.h"

AMQP_VALUE messaging_create_source(AMQP_VALUE address)
{
	AMQP_VALUE result;
	SOURCE_HANDLE source = source_create();
	source_set_address(source, amqpvalue_create_string(address));
	result = amqpvalue_create_source(source);
	source_destroy(source);
	return result;
}

AMQP_VALUE messaging_create_target(AMQP_VALUE address)
{
	AMQP_VALUE result;
	TARGET_HANDLE target = target_create();
	source_set_address(target, amqpvalue_create_string(address));
	result = amqpvalue_create_source(target);
	source_destroy(target);
	return result;
}
