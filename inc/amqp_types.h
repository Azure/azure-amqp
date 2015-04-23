#ifndef ANQP_TYPES_H
#define ANQP_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef enum AMQP_TYPE_TAG
	{
		AMQP_TYPE_STRING,
		AMQP_TYPE_ULONG
	} AMQP_TYPE;

	typedef void* AMQP_VALUE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ANQP_TYPES_H */
