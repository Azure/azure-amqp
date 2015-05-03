#ifndef MESSAGING_H
#define MESSAGING_H

#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* MESSAGING_HANDLE;

	extern MESSAGING_HANDLE messaging_create(void);
	extern void messaging_destroy(MESSAGING_HANDLE handle);
	extern AMQP_VALUE messaging_create_source(AMQP_VALUE address);
	extern AMQP_VALUE messaging_create_target(AMQP_VALUE address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGING_H */
