#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "amqplib.h"
#include "platform.h"

int amqplib_init(void)
{
	int result;

	if (platform_init())
	{
		result = __LINE__;
	}
	else
	{
		result = 0;
	}

	return result;
}

void amqplib_deinit(void)
{
	platform_deinit();
}
