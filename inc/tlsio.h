#ifndef TLSIO_H
#define TLSIO_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "io.h"
#include "logger.h"

typedef struct TLSIO_CONFIG_TAG
{
	const char* hostname;
	int port;
} TLSIO_CONFIG;

extern IO_HANDLE tlsio_create(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* context, LOGGER_LOG logger_log);
extern void tlsio_destroy(IO_HANDLE handle);
extern int tlsio_send(IO_HANDLE handle, const void* buffer, size_t size);
extern void tlsio_dowork(IO_HANDLE handle);
extern const IO_INTERFACE_DESCRIPTION* tlsio_get_interface_description(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TLSIO_H */
