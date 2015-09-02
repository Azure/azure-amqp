#define SECURITY_WIN32

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tlsio.h"
#include "socketio.h"
#include "windows.h"
#include "amqpalloc.h"
#include "sspi.h"
#include "schannel.h"

typedef enum TLS_STATE_TAG
{
	TLS_STATE_HANDSHAKE_NOT_STARTED,
	TLS_STATE_HANDSHAKE_CLIENT_HELLO_SENT,
	TLS_STATE_HANDSHAKE_SERVER_HELLO_RECEIVED,
	TLS_STATE_HANDSHAKE_DONE,
	TLS_STATE_ERROR
} TLS_STATE;

typedef struct TLS_IO_INSTANCE_TAG
{
	IO_HANDLE socket_io;
	IO_RECEIVE_CALLBACK receive_callback;
	void* context;
	LOGGER_LOG logger_log;
	CtxtHandle security_context;
	TLS_STATE tls_state;
	SEC_CHAR* host_name;
} TLS_IO_INSTANCE;

static const IO_INTERFACE_DESCRIPTION tls_io_interface_description =
{
	tlsio_create,
	tlsio_destroy,
	tlsio_send,
	tlsio_dowork,
	tlsio_get_state
};

static void tlsio_receive_bytes(void* context, const void* buffer, size_t size)
{
	TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)context;
	SecBuffer security_buffers[4];
	SecBufferDesc security_buffers_desc;

	security_buffers[0].BufferType = SECBUFFER_DATA;
	security_buffers[1].BufferType = SECBUFFER_EMPTY;
	security_buffers[2].BufferType = SECBUFFER_EMPTY;
	security_buffers[3].BufferType = SECBUFFER_EMPTY;

	security_buffers_desc.cBuffers = sizeof(security_buffers) / sizeof(security_buffers[0]);
	security_buffers_desc.pBuffers = security_buffers;
	security_buffers_desc.ulVersion = SECBUFFER_VERSION;

	SECURITY_STATUS result = DecryptMessage(&tls_io_instance->security_context, &security_buffers_desc, 0, NULL);
	if (result == SEC_E_INCOMPLETE_MESSAGE)
	{
		/* more bytes needed */
	}
	else
	{

	}
}

IO_HANDLE tlsio_create(void* io_create_parameters, IO_RECEIVE_CALLBACK receive_callback, void* context, LOGGER_LOG logger_log)
{
	TLSIO_CONFIG* tls_io_config = io_create_parameters;
	TLS_IO_INSTANCE* result;

	if (tls_io_config == NULL)
	{
		result = NULL;
	}
	else
	{
		result = amqpalloc_malloc(sizeof(TLS_IO_INSTANCE));
		if (result != NULL)
		{
			SOCKETIO_CONFIG socketio_config;

			socketio_config.hostname = tls_io_config->hostname;
			socketio_config.port = tls_io_config->port;

			result->receive_callback = NULL;
			result->logger_log = logger_log;
			result->receive_callback = receive_callback;
			result->context = context;

			result->host_name = (SEC_CHAR*)malloc(sizeof(SEC_CHAR) * (1 + strlen(tls_io_config->hostname)));
			if (result->host_name == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				const IO_INTERFACE_DESCRIPTION* socket_io_interface = socketio_get_interface_description();
				if (socket_io_interface == NULL)
				{
					amqpalloc_free(result->host_name);
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					result->socket_io = io_create(socket_io_interface, &socketio_config, tlsio_receive_bytes, result, logger_log);
					if (result->socket_io == NULL)
					{
						amqpalloc_free(result->host_name);
						amqpalloc_free(result);
						result = NULL;
					}
					else
					{
						result->tls_state = TLS_STATE_HANDSHAKE_NOT_STARTED;
					}
				}
			}
		}
	}

	return result;
}

void tlsio_destroy(IO_HANDLE tls_io)
{
	if (tls_io != NULL)
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;
		socketio_destroy(tls_io_instance->socket_io);
		amqpalloc_free(tls_io_instance->host_name);
		amqpalloc_free(tls_io);
	}
}

int tlsio_send(IO_HANDLE tls_io, const void* buffer, size_t size)
{
	int result;

	if ((tls_io == NULL) ||
		(buffer == NULL) ||
		(size == 0))
	{
		/* Invalid arguments */
		result = __LINE__;
	}
	else
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;
	}

	return result;
}

void tlsio_dowork(IO_HANDLE tls_io)
{
	if (tls_io != NULL)
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

		switch (tls_io_instance->tls_state)
		{
		default:
			break;
		case TLS_STATE_HANDSHAKE_NOT_STARTED:
		{
			SecBuffer init_security_buffers[2];
			ULONG context_attributes;
			SECURITY_STATUS status;
			SCHANNEL_CRED auth_data;
			CredHandle credential_handle;

			auth_data.dwVersion = SCHANNEL_CRED_VERSION;
			auth_data.cCreds = 0;
			auth_data.paCred = NULL;
			auth_data.hRootStore = NULL;
			auth_data.cSupportedAlgs = 0;
			auth_data.palgSupportedAlgs = NULL;
			auth_data.grbitEnabledProtocols = 0;
			auth_data.dwMinimumCipherStrength = 0;
			auth_data.dwMaximumCipherStrength = 0;
			auth_data.dwSessionLifespan = 0;
			auth_data.dwFlags = SCH_USE_STRONG_CRYPTO;
			auth_data.dwCredFormat = 0;

			status = AcquireCredentialsHandle(NULL, UNISP_NAME, SECPKG_CRED_OUTBOUND, NULL,
				&auth_data, NULL, NULL, &credential_handle, NULL);
			if (status != SEC_E_OK)
			{
				tls_io_instance->tls_state = TLS_STATE_ERROR;
			}
			else
			{
				init_security_buffers[0].cbBuffer = 0;
				init_security_buffers[0].BufferType = SECBUFFER_TOKEN;
				init_security_buffers[0].pvBuffer = NULL;
				init_security_buffers[1].cbBuffer = 0;
				init_security_buffers[1].BufferType = SECBUFFER_EMPTY;
				init_security_buffers[1].pvBuffer = 0;

				SecBufferDesc security_buffers_desc;
				security_buffers_desc.cBuffers = 2;
				security_buffers_desc.pBuffers = init_security_buffers;
				security_buffers_desc.ulVersion = SECBUFFER_VERSION;

				status = InitializeSecurityContext(&credential_handle,
					NULL, (SEC_CHAR*)tls_io_instance->host_name, ISC_REQ_EXTENDED_ERROR | ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, NULL, 0,
					&tls_io_instance->security_context, &security_buffers_desc,
					&context_attributes, NULL);

				if ((status == SEC_I_COMPLETE_NEEDED) || (status == SEC_I_CONTINUE_NEEDED) || (status == SEC_I_COMPLETE_AND_CONTINUE))
				{
					if (io_send(tls_io_instance->socket_io, init_security_buffers[0].pvBuffer, init_security_buffers[0].cbBuffer) != 0)
					{
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						tls_io_instance->tls_state = TLS_STATE_HANDSHAKE_CLIENT_HELLO_SENT;
					}
				}
			}

			break;
		}
		}

		io_dowork(tls_io_instance->socket_io);
	}
}

IO_STATE tlsio_get_state(IO_HANDLE tls_io)
{
	IO_STATE result;

	if (tls_io == NULL)
	{
		result = IO_STATE_ERROR;
	}
	else
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;
		result = (tls_io_instance->tls_state == TLS_STATE_HANDSHAKE_DONE) ? IO_STATE_READY : IO_STATE_NOT_READY;
	}

	return result;
}

const IO_INTERFACE_DESCRIPTION* tlsio_get_interface_description(void)
{
	return &tls_io_interface_description;
}
