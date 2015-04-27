#ifndef ANQPVALUE_H
#define ANQPVALUE_H

#include <stddef.h>
#include <stdint.h>
#include "amqp_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* AMQP_VALUE;

	extern AMQP_VALUE amqpvalue_create_descriptor(AMQP_VALUE value);
	extern AMQP_VALUE amqpvalue_create_ulong(uint64_t value);
	extern AMQP_VALUE amqpvalue_create_string(const char* value);
	extern AMQP_VALUE amqpvalue_create_uint(uint32_t value);
	extern AMQP_VALUE amqpvalue_create_string_with_length(const char* value, size_t length);
	extern AMQP_VALUE amqpvalue_create_list(size_t size);
	extern int amqpvalue_set_list_item(AMQP_VALUE value, size_t index, AMQP_VALUE list_item_value);
	extern int amqpvalue_get_type(AMQP_VALUE value, AMQP_TYPE* type);
	extern int amqpvalue_get_list_item_count(AMQP_VALUE value, size_t* count);
	extern AMQP_VALUE amqpvalue_get_list_item(AMQP_VALUE value, size_t index);
	extern const char* amqpvalue_get_string(AMQP_VALUE value);
	extern void amqpvalue_destroy(AMQP_VALUE value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ANQPVALUE_H */
