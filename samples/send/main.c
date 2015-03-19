#include "amqplib.h"

int main(int argc, char** argv)
{
	AMQPLIB_HANDLE amqplib_handle = amqplib_create("localhost", 5671);

	while (1)
	{
		(void)amqplib_dowork(amqplib_handle);
	}

	amqplib_destroy(amqplib_handle);
	return 0;
}
