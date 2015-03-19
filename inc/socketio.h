#ifndef SOCKETIO_H
#define SOCKETIO_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "io.h"

extern IO_HANDLE socketio_create(void* config);
extern void socketio_destroy(IO_HANDLE handle);
extern void socketio_send(IO_HANDLE handle, const void* buffer, size_t size);
extern void socketio_startreceive(IO_HANDLE handle, IO_RECEIVE_CALLBACK callback);
extern void socketio_dowork(IO_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SOCKETIO_H */
