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
	if (amqplib_init() != 0)
	{
		/* init failed */
	}
	else
	{
		MESSAGING_HANDLE messaging;
		MESSAGE_HANDLE message;
		size_t max_memory_used = 0;
		size_t last_memory_used = 0;

		amqpalloc_set_memory_tracing_enabled(true);

		messaging = messaging_create();
		message = message_create();
		message_set_to(message, "127.0.0.1");
		message_set_body(message, amqpvalue_create_binary(NULL, 0));
		(void)messaging_send(messaging, message, message_send_callback, NULL);

		while (!sent)
		{
			size_t memory_used;
			size_t current_memory_used;
			messaging_dowork(messaging);

			memory_used = amqpalloc_get_maximum_memory_used();
			current_memory_used = amqpalloc_get_current_memory_used();
			
			if (current_memory_used != last_memory_used)
			{
				printf("Current memory usage:%lu\r\n", (unsigned long)current_memory_used);
				last_memory_used = current_memory_used;
			}

			if (memory_used > max_memory_used)
			{
				max_memory_used = memory_used;
				printf("Max memory usage:%lu\r\n", (unsigned long)max_memory_used);
			}
		}

		message_destroy(message);
		messaging_destroy(messaging);
		amqplib_deinit();

		printf("Current memory usage:%lu\r\n", (unsigned long)amqpalloc_get_current_memory_used());
	}

	return 0;
}
