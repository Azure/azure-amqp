#define SECURITY_WIN32

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "tlsio.h"
#include "socketio.h"
#include "windows.h"
#include "amqpalloc.h"
#include "sspi.h"
#include "schannel.h"
#include "logger.h"

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
	ON_BYTES_RECEIVED on_bytes_received;
	ON_IO_STATE_CHANGED on_io_state_changed;
	void* callback_context;
	LOGGER_LOG logger_log;
	CtxtHandle security_context;
	TLS_STATE tls_state;
	SEC_CHAR* host_name;
	CredHandle credential_handle;
	bool credential_handle_allocated;
	unsigned char* received_bytes;
	size_t received_byte_count;
	size_t buffer_size;
	size_t needed_bytes;
	size_t consumed_bytes;
	IO_STATE io_state;
} TLS_IO_INSTANCE;

static const IO_INTERFACE_DESCRIPTION tls_io_interface_description =
{
	tlsio_create,
	tlsio_destroy,
	tlsio_open,
	tlsio_close,
	tlsio_send,
	tlsio_dowork
};

static void set_io_state(TLS_IO_INSTANCE* tls_io_instance, IO_STATE io_state)
{
	IO_STATE previous_state = tls_io_instance->io_state;
	tls_io_instance->io_state = io_state;
	if (tls_io_instance->on_io_state_changed != NULL)
	{
		tls_io_instance->on_io_state_changed(tls_io_instance->callback_context, io_state, previous_state);
	}
}

static int resize_receive_buffer(TLS_IO_INSTANCE* tls_io_instance, size_t needed_buffer_size)
{
	int result;

	if (needed_buffer_size > tls_io_instance->buffer_size)
	{
		unsigned char* new_buffer = amqpalloc_realloc(tls_io_instance->received_bytes, needed_buffer_size);
		if (new_buffer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			tls_io_instance->received_bytes = new_buffer;
			tls_io_instance->buffer_size = needed_buffer_size;
			result = 0;
		}
	}
	else
	{
		result = 0;
	}

	return result;
}

static int set_receive_buffer(TLS_IO_INSTANCE* tls_io_instance, size_t buffer_size)
{
	int result;

	unsigned char* new_buffer = amqpalloc_realloc(tls_io_instance->received_bytes, buffer_size);
	if (new_buffer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		tls_io_instance->received_bytes = new_buffer;
		tls_io_instance->buffer_size = buffer_size;
		result = 0;
	}

	return result;
}

static void tlsio_on_bytes_received(void* context, const void* buffer, size_t size)
{
	TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)context;

	if (resize_receive_buffer(tls_io_instance, tls_io_instance->received_byte_count + size) == 0)
	{
		memcpy(tls_io_instance->received_bytes + tls_io_instance->received_byte_count, buffer, size);
		tls_io_instance->received_byte_count += size;

		if (size > tls_io_instance->needed_bytes)
		{
			tls_io_instance->needed_bytes = 0;
		}
		else
		{
			tls_io_instance->needed_bytes -= size;
		}

		switch (tls_io_instance->tls_state)
		{
		default:
			break;

		case TLS_STATE_ERROR:
			break;

		case TLS_STATE_HANDSHAKE_CLIENT_HELLO_SENT:
		{
			if (tls_io_instance->needed_bytes == 0)
			{
				SecBuffer input_buffers[2];
				SecBuffer output_buffers[2];
				ULONG context_attributes;

				/* we need to try and perform the second (next) step of the init */
				input_buffers[0].cbBuffer = tls_io_instance->received_byte_count;
				input_buffers[0].BufferType = SECBUFFER_TOKEN;
				input_buffers[0].pvBuffer = (void*)tls_io_instance->received_bytes;
				input_buffers[1].cbBuffer = 0;
				input_buffers[1].BufferType = SECBUFFER_EMPTY;
				input_buffers[1].pvBuffer = 0;

				SecBufferDesc input_buffers_desc;
				input_buffers_desc.cBuffers = 2;
				input_buffers_desc.pBuffers = input_buffers;
				input_buffers_desc.ulVersion = SECBUFFER_VERSION;

				output_buffers[0].cbBuffer = 0;
				output_buffers[0].BufferType = SECBUFFER_TOKEN;
				output_buffers[0].pvBuffer = NULL;
				output_buffers[1].cbBuffer = 0;
				output_buffers[1].BufferType = SECBUFFER_EMPTY;
				output_buffers[1].pvBuffer = 0;

				SecBufferDesc output_buffers_desc;
				output_buffers_desc.cBuffers = 2;
				output_buffers_desc.pBuffers = output_buffers;
				output_buffers_desc.ulVersion = SECBUFFER_VERSION;

				SECURITY_STATUS status = InitializeSecurityContext(&tls_io_instance->credential_handle,
					&tls_io_instance->security_context, (SEC_CHAR*)tls_io_instance->host_name, ISC_REQ_EXTENDED_ERROR | ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, &input_buffers_desc, 0,
					&tls_io_instance->security_context, &output_buffers_desc,
					&context_attributes, NULL);

				switch (status)
				{
				case SEC_E_INCOMPLETE_MESSAGE:
					if (input_buffers[1].BufferType != SECBUFFER_MISSING)
					{
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						tls_io_instance->needed_bytes = input_buffers[1].cbBuffer;
						tls_io_instance->consumed_bytes += tls_io_instance->needed_bytes;
						if (resize_receive_buffer(tls_io_instance, tls_io_instance->received_byte_count + tls_io_instance->needed_bytes) != 0)
						{
							tls_io_instance->tls_state = TLS_STATE_ERROR;
						}
					}
					break;
				case SEC_E_OK:
					memmove(tls_io_instance->received_bytes, tls_io_instance->received_bytes + tls_io_instance->consumed_bytes, tls_io_instance->received_byte_count - tls_io_instance->consumed_bytes);
					tls_io_instance->received_byte_count -= tls_io_instance->consumed_bytes;

					tls_io_instance->needed_bytes = 1;
					tls_io_instance->consumed_bytes = tls_io_instance->needed_bytes;

					if (set_receive_buffer(tls_io_instance, tls_io_instance->needed_bytes + tls_io_instance->received_byte_count) != 0)
					{
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						tls_io_instance->tls_state = TLS_STATE_HANDSHAKE_DONE;
						set_io_state(tls_io_instance, IO_STATE_OPEN);
					}

					break;

				case SEC_I_COMPLETE_NEEDED:
				case SEC_I_CONTINUE_NEEDED:
				case SEC_I_COMPLETE_AND_CONTINUE:
					if ((output_buffers[0].cbBuffer > 0) && io_send(tls_io_instance->socket_io, output_buffers[0].pvBuffer, output_buffers[0].cbBuffer) != 0)
					{
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						memmove(tls_io_instance->received_bytes, tls_io_instance->received_bytes + tls_io_instance->consumed_bytes, tls_io_instance->received_byte_count - tls_io_instance->consumed_bytes);
						tls_io_instance->received_byte_count -= tls_io_instance->consumed_bytes;

						/* set the needed bytes to 1, to get on the next byte how many we actually need */
						tls_io_instance->needed_bytes = 1;
						tls_io_instance->consumed_bytes = tls_io_instance->needed_bytes;
						if (set_receive_buffer(tls_io_instance, tls_io_instance->needed_bytes + tls_io_instance->received_byte_count) != 0)
						{
							FreeCredentialHandle(&tls_io_instance->credential_handle);
							tls_io_instance->tls_state = TLS_STATE_ERROR;
						}
						else
						{
							tls_io_instance->tls_state = TLS_STATE_HANDSHAKE_CLIENT_HELLO_SENT;
						}
					}
					break;
				}
			}

			break;

		case TLS_STATE_HANDSHAKE_DONE:
		{
			if (tls_io_instance->needed_bytes == 0)
			{
				SecBuffer security_buffers[4];
				SecBufferDesc security_buffers_desc;

				security_buffers[0].BufferType = SECBUFFER_DATA;
				security_buffers[0].pvBuffer = tls_io_instance->received_bytes;
				security_buffers[0].cbBuffer = tls_io_instance->received_byte_count;
				security_buffers[1].BufferType = SECBUFFER_EMPTY;
				security_buffers[2].BufferType = SECBUFFER_EMPTY;
				security_buffers[3].BufferType = SECBUFFER_EMPTY;

				security_buffers_desc.cBuffers = sizeof(security_buffers) / sizeof(security_buffers[0]);
				security_buffers_desc.pBuffers = security_buffers;
				security_buffers_desc.ulVersion = SECBUFFER_VERSION;

				SECURITY_STATUS status = DecryptMessage(&tls_io_instance->security_context, &security_buffers_desc, 0, NULL);
				switch (status)
				{
				case SEC_E_INCOMPLETE_MESSAGE:
					if (security_buffers[1].BufferType != SECBUFFER_MISSING)
					{
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						tls_io_instance->needed_bytes = security_buffers[1].cbBuffer;
						tls_io_instance->consumed_bytes += tls_io_instance->needed_bytes;
						if (resize_receive_buffer(tls_io_instance, tls_io_instance->received_byte_count + tls_io_instance->needed_bytes) != 0)
						{
							tls_io_instance->tls_state = TLS_STATE_ERROR;
						}
					}
					break;

				case SEC_E_OK:
					if (security_buffers[1].BufferType != SECBUFFER_DATA)
					{
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						size_t i;
						for (i = 0; i < security_buffers[1].cbBuffer; i++)
						{
							LOG(tls_io_instance->logger_log, 0, "<-%02x ", ((unsigned char*)security_buffers[1].pvBuffer)[i]);
						}

						/* notify of the received data */
						if (tls_io_instance->on_bytes_received != NULL)
						{
							tls_io_instance->on_bytes_received(tls_io_instance->callback_context, security_buffers[1].pvBuffer, security_buffers[1].cbBuffer);
						}

						memmove(tls_io_instance->received_bytes, tls_io_instance->received_bytes + tls_io_instance->consumed_bytes, tls_io_instance->received_byte_count - tls_io_instance->consumed_bytes);
						tls_io_instance->received_byte_count -= tls_io_instance->consumed_bytes;

						tls_io_instance->needed_bytes = 1;
						tls_io_instance->consumed_bytes = tls_io_instance->needed_bytes;

						if (set_receive_buffer(tls_io_instance, tls_io_instance->needed_bytes + tls_io_instance->received_byte_count) != 0)
						{
							tls_io_instance->tls_state = TLS_STATE_ERROR;
						}
						else
						{
							tls_io_instance->tls_state = TLS_STATE_HANDSHAKE_DONE;
							set_io_state(tls_io_instance, IO_STATE_OPEN);
						}
					}
					break;
				}
			}

			break;
		}
		}
		}
	}
}

static void tlsio_on_io_state_changed(void* context, IO_STATE new_io_state, IO_STATE previous_io_state)
{
	TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)context;

	if (tls_io_instance->io_state == IO_STATE_OPENING)
	{
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
				&auth_data, NULL, NULL, &tls_io_instance->credential_handle, NULL);
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

				status = InitializeSecurityContext(&tls_io_instance->credential_handle,
					NULL, (SEC_CHAR*)tls_io_instance->host_name, ISC_REQ_EXTENDED_ERROR | ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, NULL, 0,
					&tls_io_instance->security_context, &security_buffers_desc,
					&context_attributes, NULL);

				if ((status == SEC_I_COMPLETE_NEEDED) || (status == SEC_I_CONTINUE_NEEDED) || (status == SEC_I_COMPLETE_AND_CONTINUE))
				{
					if (io_send(tls_io_instance->socket_io, init_security_buffers[0].pvBuffer, init_security_buffers[0].cbBuffer) != 0)
					{
						FreeCredentialHandle(&tls_io_instance->credential_handle);
						tls_io_instance->tls_state = TLS_STATE_ERROR;
					}
					else
					{
						/* set the needed bytes to 1, to get on the next byte how many we actually need */
						tls_io_instance->needed_bytes = 1;
						tls_io_instance->consumed_bytes = tls_io_instance->needed_bytes;
						if (resize_receive_buffer(tls_io_instance, tls_io_instance->needed_bytes + tls_io_instance->received_byte_count) != 0)
						{
							FreeCredentialHandle(&tls_io_instance->credential_handle);
							tls_io_instance->tls_state = TLS_STATE_ERROR;
						}
						else
						{
							tls_io_instance->tls_state = TLS_STATE_HANDSHAKE_CLIENT_HELLO_SENT;
						}
					}
				}
			}

			break;
		}
		}
	}
}

IO_HANDLE tlsio_create(void* io_create_parameters, LOGGER_LOG logger_log)
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

			result->on_bytes_received = NULL;
			result->on_io_state_changed = NULL;
			result->logger_log = logger_log;
			result->callback_context = NULL;

			result->host_name = (SEC_CHAR*)amqpalloc_malloc(sizeof(SEC_CHAR) * (1 + strlen(tls_io_config->hostname)));
			if (result->host_name == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
			else
			{
				(void)strcpy(result->host_name, tls_io_config->hostname);

				const IO_INTERFACE_DESCRIPTION* socket_io_interface = socketio_get_interface_description();
				if (socket_io_interface == NULL)
				{
					amqpalloc_free(result->host_name);
					amqpalloc_free(result);
					result = NULL;
				}
				else
				{
					result->socket_io = io_create(socket_io_interface, &socketio_config, logger_log);
					if (result->socket_io == NULL)
					{
						amqpalloc_free(result->host_name);
						amqpalloc_free(result);
						result = NULL;
					}
					else
					{
						result->received_bytes = NULL;
						result->received_byte_count = 0;
						result->buffer_size = 0;
						result->consumed_bytes = 0;
						result->tls_state = TLS_STATE_HANDSHAKE_NOT_STARTED;
						set_io_state(result, IO_STATE_NOT_OPEN);
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
		if (tls_io_instance->credential_handle_allocated)
		{
			(void)FreeCredentialHandle(&tls_io_instance->credential_handle);
		}

		if (tls_io_instance->received_bytes != NULL)
		{
			amqpalloc_free(tls_io_instance->received_bytes);
		}

		io_destroy(tls_io_instance->socket_io);
		amqpalloc_free(tls_io_instance->host_name);
		amqpalloc_free(tls_io);
	}
}

int tlsio_open(IO_HANDLE tls_io, ON_BYTES_RECEIVED on_bytes_received, ON_IO_STATE_CHANGED on_io_state_changed, void* callback_context)
{
	int result;

	if (tls_io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

		if (tls_io_instance->io_state != IO_STATE_NOT_OPEN)
		{
			result = __LINE__;
		}
		else
		{
			tls_io_instance->on_bytes_received = on_bytes_received;
			tls_io_instance->on_io_state_changed = on_io_state_changed;
			tls_io_instance->callback_context = callback_context;

			set_io_state(tls_io_instance, IO_STATE_OPENING);

			if (io_open(tls_io_instance->socket_io, tlsio_on_bytes_received, tlsio_on_io_state_changed, tls_io_instance) != 0)
			{
				set_io_state(tls_io_instance, IO_STATE_ERROR);
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int tlsio_close(IO_HANDLE tls_io)
{
	int result = 0;

	if (tls_io == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;
		(void)io_close(tls_io_instance->socket_io);
		set_io_state(tls_io_instance, IO_STATE_NOT_OPEN);
	}

	return result;
}

int send_chunk(IO_HANDLE tls_io, const void* buffer, size_t size)
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
		if (tls_io_instance->io_state != IO_STATE_OPEN)
		{
			result = __LINE__;
		}
		else
		{
			SecBuffer security_buffers[4];
			unsigned char out_buffer[65536];
			SecBufferDesc security_buffers_desc;
			SecPkgContext_StreamSizes  sizes;

			SECURITY_STATUS status = QueryContextAttributes(&tls_io_instance->security_context, SECPKG_ATTR_STREAM_SIZES, &sizes);

			memcpy(out_buffer + sizes.cbHeader, buffer, size);

			security_buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
			security_buffers[0].cbBuffer = sizes.cbHeader;
			security_buffers[0].pvBuffer = out_buffer;
			security_buffers[1].BufferType = SECBUFFER_DATA;
			security_buffers[1].cbBuffer = size;
			security_buffers[1].pvBuffer = out_buffer + sizes.cbHeader;
			security_buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
			security_buffers[2].cbBuffer = sizes.cbTrailer;
			security_buffers[2].pvBuffer = out_buffer + sizes.cbHeader + size;
			security_buffers[3].cbBuffer = 0;
			security_buffers[3].BufferType = SECBUFFER_EMPTY;
			security_buffers[3].pvBuffer = 0;

			security_buffers_desc.cBuffers = sizeof(security_buffers) / sizeof(security_buffers[0]);
			security_buffers_desc.pBuffers = security_buffers;
			security_buffers_desc.ulVersion = SECBUFFER_VERSION;

			status = EncryptMessage(&tls_io_instance->security_context, 0, &security_buffers_desc, 0);
			if (FAILED(status))
			{
				result = __LINE__;
			}
			else
			{
				if (io_send(tls_io_instance->socket_io, out_buffer, security_buffers[0].cbBuffer + security_buffers[1].cbBuffer + security_buffers[2].cbBuffer) != 0)
				{
					result = __LINE__;
				}
				else
				{
					size_t i;
					for (i = 0; i < size; i++)
					{
						LOG(tls_io_instance->logger_log, 0, "%02x-> ", ((unsigned char*)buffer)[i]);
					}

					result = 0;
				}
			}
		}
	}

	return result;
}

int tlsio_send(IO_HANDLE tls_io, const void* buffer, size_t size)
{
	int result;

	while (size > 0)
	{
		size_t to_send = 16 * 1024;
		if (to_send > size)
		{
			to_send = size;
		}

		if (send_chunk(tls_io, buffer, to_send) != 0)
		{
			break;
		}

		size -= to_send;
		buffer = ((const unsigned char*)buffer) + to_send;
	}

	if (size > 0)
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

void tlsio_dowork(IO_HANDLE tls_io)
{
	if (tls_io != NULL)
	{
		TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;
		io_dowork(tls_io_instance->socket_io);
	}
}

const IO_INTERFACE_DESCRIPTION* tlsio_get_interface_description(void)
{
	return &tls_io_interface_description;
}
