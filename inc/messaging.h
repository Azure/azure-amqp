#ifndef MESSAGING_H
#define MESSAGING_H

#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern AMQP_VALUE messaging_create_source(AMQP_VALUE address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGING_H */
