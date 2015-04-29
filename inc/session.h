#ifndef SESSION_H
#define SESSION_H

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

	typedef void* SESSION_HANDLE;

	extern SESSION_HANDLE session_create(CONNECTION_HANDLE connection);
	extern void session_destroy(SESSION_HANDLE handle);
	extern int session_dowork(SESSION_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SESSION_H */
