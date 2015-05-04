#include "amqplib.h"
#include "messaging.h"

int main(int argc, char** argv)
{
	AMQPLIB_HANDLE amqplib_handle;

	if (amqplib_init() != 0)
	{
		/* init failed */
	}
	else
	{
		LINK_HANDLE link;
		AMQP_VALUE payload;
		MESSAGING_HANDLE messaging;
		MESSAGE_HANDLE message;
		int sent = 0;

		amqplib_handle = amqplib_create();

		messaging = messaging_create();
		message = message_create();
		message_set_to("amqp://127.0.0.1");
		(void)messaging_send(message);

		amqplib_destroy(amqplib_handle);
		amqplib_deinit();
	}

	return 0;
}
