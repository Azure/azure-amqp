#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void(*LOGGER_LOG)(char* format, ...);

#define LOG(logger, ...) if (logger != NULL) logger(__VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LOGGER_H */
