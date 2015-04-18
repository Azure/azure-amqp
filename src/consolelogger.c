#include <stdarg.h>
#include <stdio.h>
#include "logger.h"

void consolelogger_log(char* format, ...)
{
	va_list args;
	va_start(args, format);
	(void)printf(format, args);
	va_end(args);
}
