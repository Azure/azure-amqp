#ifndef SOCKETIO_H
#define SOCKETIO_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "io.h"
#include "logger.h"

typedef struct SOCKETIO_CONFIG_TAG
{
	const char* hostname;
	int port;
} SOCKETIO_CONFIG;

extern IO_HANDLE socketio_create(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* context, LOGGER_LOG logger_log);
extern void socketio_destroy(IO_HANDLE socket_io);
extern int socketio_send(IO_HANDLE socket_io, const void* buffer, size_t size);
extern void socketio_dowork(IO_HANDLE socket_io);
extern IO_STATE socketio_get_state(IO_HANDLE socket_io);
extern const IO_INTERFACE_DESCRIPTION* socketio_get_interface_description(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SOCKETIO_H */
