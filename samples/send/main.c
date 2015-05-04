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
		int sent = 0;

		amqplib_handle = amqplib_create("127.0.0.1", 5672);
		link = amqplib_get_link(amqplib_handle);
		payload = amqpvalue_create_null();

		while (1)
		{
			LINK_STATE link_state;

			(void)amqplib_dowork(amqplib_handle);
			if ((link_get_state(link, &link_state) == 0) &&
				(link_state == LINK_STATE_ATTACHED) &&
				(sent < 1000))
			{
				link_transfer(link, &payload, 1);
				sent++;
			}
		}

		amqplib_destroy(amqplib_handle);
		amqplib_deinit();
	}

	return 0;
}
