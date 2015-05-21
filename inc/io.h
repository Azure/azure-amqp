#ifndef IO_H
#define IO_H

#include "logger.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

	typedef void* IO_HANDLE;
	typedef void* CONCRETE_IO_HANDLE;
	
	typedef void(*IO_RECEIVE_CALLBACK)(void* context, const void* buffer, size_t size);

	typedef CONCRETE_IO_HANDLE(*IO_CREATE)(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* receive_callback_context, LOGGER_LOG logger_log);
	typedef void(*IO_DESTROY)(CONCRETE_IO_HANDLE handle);
	typedef int(*IO_SEND)(CONCRETE_IO_HANDLE handle, const void* buffer, size_t size);
	typedef int(*IO_DOWORK)(CONCRETE_IO_HANDLE handle);

	typedef struct IO_INTERFACE_DESCRIPTION_TAG
	{
		IO_CREATE concrete_io_create;
		IO_DESTROY concrete_io_destroy;
		IO_SEND concrete_io_send;
		IO_DOWORK concrete_io_dowork;
	} IO_INTERFACE_DESCRIPTION;

	extern IO_HANDLE io_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* receive_callback_context, LOGGER_LOG logger_log);
	extern void io_destroy(IO_HANDLE handle);
	extern int io_send(IO_HANDLE handle, const void* buffer, size_t size);
	extern int io_dowork(IO_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IO_H */
