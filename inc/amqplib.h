#ifndef AMQPLIB_H
#define AMQPLIB_H

#include "link.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* AMQPLIB_HANDLE;

extern int amqplib_init(void);
extern void amqplib_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQPLIB_H */
