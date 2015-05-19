#ifndef ANQP_TYPES_H
#define ANQP_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef enum AMQP_TYPE_TAG
	{
		AMQP_TYPE_NULL,
		AMQP_TYPE_BOOL,
		AMQP_TYPE_UBYTE,
		AMQP_TYPE_USHORT,
		AMQP_TYPE_UINT,
		AMQP_TYPE_ULONG,
		AMQP_TYPE_BYTE,
		AMQP_TYPE_SHORT,
		AMQP_TYPE_INT,
		AMQP_TYPE_LONG,
		AMQP_TYPE_FLOAT,
		AMQP_TYPE_DOUBLE,
		AMQP_TYPE_DESCRIPTOR,
		AMQP_TYPE_LIST,
		AMQP_TYPE_STRING,
		AMQP_TYPE_COMPOSITE,
		AMQP_TYPE_BINARY
	} AMQP_TYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ANQP_TYPES_H */
