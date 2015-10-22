#include <stdlib.h>
#ifdef _CRT_DBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <stdbool.h>
#include "amqplib.h"
#include "message_receiver.h"
#include "message.h"
#include "messaging.h"
#include "amqpalloc.h"
#include "saslio.h"
#include "tlsio.h"

void on_message_received(const void* context, MESSAGE_HANDLE message)
{
	(void)message;
	(void)context;

	printf("Message received.\r\n");
}

int main(int argc, char** argv)
{
	int result;
	IO_HANDLE sasl_io = NULL;
	CONNECTION_HANDLE connection = NULL;
	SESSION_HANDLE session = NULL;
	LINK_HANDLE link = NULL;
	MESSAGE_RECEIVER_HANDLE message_receiver = NULL;

	amqpalloc_set_memory_tracing_enabled(true);

	if (amqplib_init() != 0)
	{
		result = -1;
		goto error;
	}

	size_t last_memory_used = 0;
	unsigned char muie[4] = { 'm', 'u', 'i', 'e' };
	BINARY_DATA binary_data = { muie, sizeof(muie) };

	TLSIO_CONFIG tls_io_config = { "pupupupu.servicebus.windows.net", 5671 };
	SASLIO_CONFIG sasl_io_config = { tlsio_get_interface_description(), &tls_io_config };

	sasl_io = io_create(saslio_get_interface_description(), &sasl_io_config, NULL);
	connection = connection_create(sasl_io, "pupupupu.servicebus.windows.net", "11222");
	session = session_create(connection);
	link = link_create(session, "sender-link", messaging_create_source("ingress"), messaging_create_target("amqps://pupupupu.servicebus.windows.net/ingress"));
	message_receiver = messagereceiver_create(link);
	if ((message_receiver == NULL) ||
		(messagereceiver_subscribe(message_receiver, on_message_received, message_receiver) != 0))
	{
		result = -1;
		goto error;
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
	}

	result = 0;

error:
	messagereceiver_destroy(message_receiver);
	link_destroy(link);
	session_destroy(session);
	connection_destroy(connection);
	amqplib_deinit();

	printf("Max memory usage:%lu\r\n", (unsigned long)amqpalloc_get_maximum_memory_used());
	printf("Current memory usage:%lu\r\n", (unsigned long)amqpalloc_get_current_memory_used());

#ifdef _CRT_DBG_MAP_ALLOC
	_CrtDumpMemoryLeaks();
#endif

	return result;
}
