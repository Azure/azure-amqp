#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <stdint.h>
#include "io.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* CONNECTION_HANDLE;

	extern CONNECTION_HANDLE connection_create(const char* host, int port);
	extern void connection_destroy(CONNECTION_HANDLE handle);
	extern int connection_dowork(CONNECTION_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONNECTION_H */
