#include <stddef.h>
#include <stdlib.h>
#include "io.h"
#include "amqpalloc.h"

typedef struct IO_INSTANCE_TAG
{
	const IO_INTERFACE_DESCRIPTION* io_interface_description;
	IO_HANDLE concrete_io_handle;
} IO_INSTANCE;

IO_HANDLE io_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, const void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* receive_callback_context, LOGGER_LOG logger_log)
{
	IO_INSTANCE* io_instance;
	/* Codes_SRS_IO_01_003: [If the argument io_interface_description is NULL, io_create shall return NULL.] */
	if ((io_interface_description == NULL) ||
		/* Codes_SRS_IO_01_004: [If any io_interface_description member is NULL, io_create shall return NULL.] */
		(io_interface_description->concrete_io_create == NULL) ||
		(io_interface_description->concrete_io_destroy == NULL) ||
		(io_interface_description->concrete_io_open == NULL) ||
		(io_interface_description->concrete_io_close == NULL) ||
		(io_interface_description->concrete_io_send == NULL) ||
		(io_interface_description->concrete_io_dowork == NULL))
	{
		io_instance = NULL;
	}
	else
	{
		io_instance = (IO_INSTANCE*)amqpalloc_malloc(sizeof(IO_INSTANCE));

		/* Codes_SRS_IO_01_017: [If allocating the memory needed for the IO interface fails then io_create shall return NULL.] */
		if (io_instance != NULL)
		{
			/* Codes_SRS_IO_01_001: [io_create shall return on success a non-NULL handle to a new IO interface.] */
			io_instance->io_interface_description = io_interface_description;

			/* Codes_SRS_IO_01_002: [In order to instantiate the concrete IO implementation the function concrete_io_create from the io_interface_description shall be called, passing the io_create_parameters, receive_callback, receive_callback_context and logger_log arguments.] */
			io_instance->concrete_io_handle = io_instance->io_interface_description->concrete_io_create((void*)io_create_parameters, receive_callback, receive_callback_context, logger_log);

			/* Codes_SRS_IO_01_016: [If the underlying concrete_io_create call fails, io_create shall return NULL.] */
			if (io_instance->concrete_io_handle == NULL)
			{
				amqpalloc_free(io_instance);
				io_instance = NULL;
			}
		}
	}
	return (IO_HANDLE)io_instance;
}

void io_destroy(IO_HANDLE io)
{
	if (io != NULL)
	{
		IO_INSTANCE* io_instance = (IO_INSTANCE*)io;

		/* Codes_SRS_IO_01_006: [io_destroy shall also call the concrete_io_destroy function that is member of the io_interface_description argument passed to io_create, while passing as argument to concrete_io_destroy the result of the underlying concrete_io_create handle that was called as part of the io_create call.] */
		io_instance->io_interface_description->concrete_io_destroy(io_instance->concrete_io_handle);

		/* Codes_SRS_IO_01_005: [io_destroy shall free all resources associated with the IO handle.] */
		amqpalloc_free(io_instance);
	}
}

int io_send(IO_HANDLE io, const void* buffer, size_t size)
{
	int result;

	/* Codes_SRS_IO_01_011: [No error check shall be performed on buffer and size.] */
	/* Codes_SRS_IO_01_010: [If handle is NULL, io_send shall return a non-zero value.] */
	if (io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		IO_INSTANCE* io_instance = (IO_INSTANCE*)io;

		/* Codes_SRS_IO_01_008: [io_send shall pass the sequence of bytes pointed to by buffer to the concrete IO implementation specified in io_create, by calling the concrete_io_send function while passing down the buffer and size arguments to it.] */
		/* Codes_SRS_IO_01_009: [On success, io_send shall return 0.] */
		/* Codes_SRS_IO_01_015: [If the underlying concrete_io_send fails, io_send shall return a non-zero value.] */
		result = io_instance->io_interface_description->concrete_io_send(io_instance->concrete_io_handle, buffer, size);
	}

	return result;
}

void io_dowork(IO_HANDLE io)
{
	/* Codes_SRS_IO_01_018: [When the handle argument is NULL, io_dowork shall do nothing.] */
	if (io != NULL)
	{
		IO_INSTANCE* io_instance = (IO_INSTANCE*)io;

		/* Codes_SRS_IO_01_012: [io_dowork shall call the concrete IO implementation specified in io_create, by calling the concrete_io_dowork function.] */
		io_instance->io_interface_description->concrete_io_dowork(io_instance->concrete_io_handle);
	}
}

IO_STATE io_get_state(IO_HANDLE io)
{
	IO_STATE result;

	if (io == NULL)
	{
		result = IO_STATE_ERROR;
	}
	else
	{
		IO_INSTANCE* io_instance = (IO_INSTANCE*)io;
		result = io_instance->io_interface_description->concrete_io_get_state(io_instance->concrete_io_handle);
	}

	return result;
}
