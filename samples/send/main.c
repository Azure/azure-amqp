#include "amqplib.h"

int main(int argc, char** argv)
{
	AMQPLIB_HANDLE amqplib_handle;

	if (amqplib_init() != 0)
	{
		/* init failed */
	}
	else
	{
		amqplib_handle = amqplib_create("10.91.97.188", 5672);

		while (1)
		{
			(void)amqplib_dowork(amqplib_handle);
		}

		amqplib_destroy(amqplib_handle);
		amqplib_deinit();
	}

	return 0;
}
