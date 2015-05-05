#include "amqplib.h"
#include "messaging.h"
#include "message.h"

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
		int sent = 0;

		messaging = messaging_create();
		message = message_create();
		message_set_to(message, "amqp://127.0.0.1");
		(void)messaging_send(messaging, message);

		amqplib_deinit();
	}

	return 0;
}
