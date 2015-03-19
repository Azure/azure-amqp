#ifndef SOCKETIO_H
#define SOCKETIO_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "io.h"

typedef struct SOCKETIO_CONFIG_TAG
{
	const char* hostname;
	int port;
} SOCKETIO_CONFIG;

extern IO_HANDLE socketio_create(void* config);
extern void socketio_destroy(IO_HANDLE handle);
extern int socketio_send(IO_HANDLE handle, const void* buffer, size_t size);
extern int socketio_startreceive(IO_HANDLE handle, IO_RECEIVE_CALLBACK callback);
extern int socketio_dowork(IO_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SOCKETIO_H */
