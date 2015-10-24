#include <stdlib.h>
#ifdef _CRT_DBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <stdbool.h>
#include "amqplib.h"
#include "message_sender.h"
#include "message.h"
#include "messaging.h"
#include "amqpalloc.h"
#include "saslio.h"
#include "sasl_plain.h"
#include "tlsio.h"
#include "consolelogger.h"

static bool sent = false;

void on_message_send_complete(const void* context, MESSAGE_SEND_RESULT send_result)
{
	(void)send_result;
	(void)context;

	printf("Sent.\r\n");
	sent = true;
}

int main(int argc, char** argv)
{
	int result;

	amqpalloc_set_memory_tracing_enabled(true);

	if (amqplib_init() != 0)
	{
		result = -1;
	}
	else
	{
		IO_HANDLE sasl_io;
		CONNECTION_HANDLE connection;
		SESSION_HANDLE session;
		LINK_HANDLE link;
		MESSAGE_SENDER_HANDLE message_sender;
		MESSAGE_HANDLE message;

		size_t last_memory_used = 0;
		unsigned char muie[4] = { 'm', 'u', 'i', 'e' };
		BINARY_DATA binary_data = { muie, sizeof(muie) };

        /* create SASL plain handler */
		SASL_PLAIN_CONFIG sasl_plain_config = { "RootManageSharedAccessKey", "GZOKQjll7SoJuQcoArp26Zs3wxFj9FmA0Q7t3Gpv+90=" };
		SASL_MECHANISM_HANDLE sasl_mechanism_handle = saslmechanism_create(saslplain_get_interface(), &sasl_plain_config);

        /* create the TLS IO */
        TLSIO_CONFIG tls_io_config = { "pupupupu.servicebus.windows.net", 5671 };

        /* create the SASL IO using the TLS IO */
        SASLIO_CONFIG sasl_io_config = { tlsio_get_interface_description(), &tls_io_config, sasl_mechanism_handle };
		sasl_io = io_create(saslio_get_interface_description(), &sasl_io_config, NULL);

        /* create the connection, session and link */
        connection = connection_create(sasl_io, "pupupupu.servicebus.windows.net", "whatever");
		session = session_create(connection);
		AMQP_VALUE source = messaging_create_source("ingress");
		AMQP_VALUE target = messaging_create_target("amqps://pupupupu.servicebus.windows.net/ingress");
        link = link_create(session, "sender-link", role_sender, source, target);
		amqpvalue_destroy(source);
		amqpvalue_destroy(target);

		message = message_create();
		message_set_to(message, "pupupupu.servicebus.windows.net");
		message_set_body_amqp_data(message, binary_data);

        /* create a message sender */
        message_sender = messagesender_create(link);
		if (message_sender != NULL)
		{
			uint32_t i;

			for (i = 0; i < 1; i++)
			{
				(void)messagesender_send(message_sender, message, on_message_send_complete, message);
			}

			while (true)
			{
				size_t current_memory_used;
				size_t maximum_memory_used;
				connection_dowork(connection);

				current_memory_used = amqpalloc_get_current_memory_used();
				maximum_memory_used = amqpalloc_get_maximum_memory_used();

				if (current_memory_used != last_memory_used)
				{
					printf("Current memory usage:%lu (max:%lu)\r\n", (unsigned long)current_memory_used, (unsigned long)maximum_memory_used);
					last_memory_used = current_memory_used;
				}

				if (sent)
				{
					break;
				}
			}
		}
		
		message_destroy(message);
		messagesender_destroy(message_sender);
		link_destroy(link);
		session_destroy(session);
		connection_destroy(connection);
		io_destroy(sasl_io);
		saslmechanism_destroy(sasl_mechanism_handle);
		amqplib_deinit();

		printf("Max memory usage:%lu\r\n", (unsigned long)amqpalloc_get_maximum_memory_used());
		printf("Current memory usage:%lu\r\n", (unsigned long)amqpalloc_get_current_memory_used());
	}

#ifdef _CRT_DBG_MAP_ALLOC
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
