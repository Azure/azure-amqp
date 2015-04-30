#ifndef LINK_H
#define LINK_H

#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* LINK_HANDLE;

extern LINK_HANDLE link_create(SESSION_HANDLE session, AMQP_VALUE source, AMQP_VALUE target);
extern void link_destroy(LINK_HANDLE handle);
extern int link_dowork(LINK_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINK_H */
