#ifndef ANQPVALUE_H
#define ANQPVALUE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* AMQP_VALUE;

	extern AMQP_VALUE amqpvalue_create_descriptor(AMQP_VALUE value);
	extern AMQP_VALUE amqpvalue_create_ulong(uint64_t value);
	extern AMQP_VALUE amqpvalue_create_string(const char* string, uint32_t length);
	extern AMQP_VALUE amqpvalue_create_list(size_t size);
	extern int amqpvalue_set_list_item(AMQP_VALUE value, size_t index, AMQP_VALUE list_item_value);
	extern void amqpvalue_destroy(AMQP_VALUE value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ANQPVALUE_H */
