#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "logger.h"

	extern void consolelogger_log(unsigned int options, char* format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONSOLELOGGER_H */
