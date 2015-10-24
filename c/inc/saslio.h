#ifndef SASLIO_H
#define SASLIO_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "io.h"
#include "sasl_mechanism.h"
#include "logger.h"

typedef struct SASLIO_CONFIG_TAG
{
	const IO_INTERFACE_DESCRIPTION* socket_io_interface;
	const void* socket_io_parameters;
	SASL_MECHANISM_HANDLE sasl_mechanism;
} SASLIO_CONFIG;

extern IO_HANDLE saslio_create(void* io_create_parameters, LOGGER_LOG logger_log);
extern void saslio_destroy(IO_HANDLE sasl_io);
extern int saslio_open(IO_HANDLE sasl_io, ON_BYTES_RECEIVED on_bytes_received, ON_IO_STATE_CHANGED on_io_state_changed, void* callback_context);
extern int saslio_close(IO_HANDLE sasl_io);
extern int saslio_send(IO_HANDLE sasl_io, const void* buffer, size_t size);
extern void saslio_dowork(IO_HANDLE sasl_io);
extern IO_STATE saslio_get_state(IO_HANDLE sasl_io);
extern const IO_INTERFACE_DESCRIPTION* saslio_get_interface_description(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SASLIO_H */
