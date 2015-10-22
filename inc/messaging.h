#ifndef MESSAGING_H
#define MESSAGING_H

#include "amqpvalue.h"
#include "amqp_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	extern AMQP_VALUE messaging_create_source(SOURCE_HANDLE address);
	extern AMQP_VALUE messaging_create_target(TARGET_HANDLE address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGING_H */
