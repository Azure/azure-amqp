#ifndef IO_H
#define IO_H

#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* IO_HANDLE;
	
	typedef void(*IO_RECEIVE_CALLBACK)(IO_HANDLE handle, const void* buffer, size_t size);

	typedef IO_HANDLE(*IO_CREATE)(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, LOGGER_LOG logger_log);
	typedef int(*IO_SEND)(IO_HANDLE handle, const void* buffer, size_t size);
	typedef int(*IO_DOWORK)(IO_HANDLE handle);

	typedef struct IO_INTERFACE_DESCRIPTION_TAG
	{
		IO_CREATE io_create;
		IO_SEND io_send;
		IO_DOWORK io_dowork;
	} IO_INTERFACE_DESCRIPTION;

	extern IO_HANDLE io_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, void* io_create_parameters, LOGGER_LOG logger_log);
	extern int io_send(IO_HANDLE handle, const void* buffer, size_t size);
	const IO_INTERFACE_DESCRIPTION* socketio_get_interface_description(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IO_H */
