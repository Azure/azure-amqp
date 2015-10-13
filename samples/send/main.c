#include <stdlib.h>
#ifdef _CRT_DBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <stdbool.h>
#include "amqplib.h"
#include "messaging.h"
#include "message.h"
#include "amqpalloc.h"

static bool sent = false;

void message_send_callback(MESSAGING_RESULT send_result, const void* context)
{
	(void)send_result;
	(void)context;

	printf("Sent.\r\n");
	sent = true;
}

int main(int argc, char** argv)
{
	amqpalloc_set_memory_tracing_enabled(true);

	if (amqplib_init() != 0)
	{
		/* init failed */
	}
	else
	{
		MESSAGING_HANDLE messaging;
		MESSAGE_HANDLE message;
		size_t last_memory_used = 0;
		amqp_binary body = { "muie", 4 };

		messaging = messaging_create();
		uint32_t i;

		for (i = 0; i < 1; i++)
		{
			message = message_create();
			message_set_to(message, "pupupupu.servicebus.windows.net");
			message_set_body(message, amqpvalue_create_binary(body));

			(void)messaging_send(messaging, message, message_send_callback, NULL);
		}

		while (true)
		{
			size_t current_memory_used;
			messaging_dowork(messaging);

			current_memory_used = amqpalloc_get_maximum_memory_used();
			
			if (current_memory_used != last_memory_used)
			{
				printf("Current memory usage:%lu\r\n", (unsigned long)current_memory_used);
				last_memory_used = current_memory_used;
			}
		}
		
		message_destroy(message);
		messaging_destroy(messaging);
		amqplib_deinit();

		printf("Max memory usage:%lu\r\n", (unsigned long)amqpalloc_get_maximum_memory_used());
		printf("Current memory usage:%lu\r\n", (unsigned long)amqpalloc_get_current_memory_used());
	}

#ifdef _CRT_DBG_MAP_ALLOC
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
