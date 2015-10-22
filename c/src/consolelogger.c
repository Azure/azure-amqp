#include <stdarg.h>
#include <stdio.h>
#include "logger.h"

void consolelogger_log(unsigned int options, char* format, ...)
{
	va_list args;
	va_start(args, format);
	(void)vprintf(format, args);
	va_end(args);

	if (options & LOG_LINE)
	{
		(void)printf("\r\n");
	}
}
