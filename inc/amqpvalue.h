#ifndef ANQPVALUE_H
#define ANQPVALUE_H

#include "amqp_types.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#include <cstdbool>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif /* __cplusplus */

	typedef void* AMQP_VALUE;

	extern AMQP_VALUE amqpvalue_create_null(void);
	extern AMQP_VALUE amqpvalue_create_boolean(bool value);
	extern int amqpvalue_get_boolean(AMQP_VALUE value, bool* bool_value);
	extern AMQP_VALUE amqpvalue_create_ubyte(unsigned char value);
	extern int amqpvalue_get_ubyte(AMQP_VALUE value, unsigned char* ubyte_value);
	extern AMQP_VALUE amqpvalue_create_ushort(uint16_t value);
	extern int amqpvalue_get_ushort(AMQP_VALUE value, uint16_t* ushort_value);
	extern AMQP_VALUE amqpvalue_create_uint(uint32_t value);
	extern int amqpvalue_get_uint(AMQP_VALUE value, uint32_t* uint_value);
	extern AMQP_VALUE amqpvalue_create_ulong(uint64_t value);
	extern int amqpvalue_get_ulong(AMQP_VALUE value, uint64_t* ulong_value);
	extern AMQP_VALUE amqpvalue_create_byte(char value);
	extern int amqpvalue_get_byte(AMQP_VALUE value, char* byte_value);
	extern AMQP_VALUE amqpvalue_create_short(int16_t value);
	extern int amqpvalue_get_short(AMQP_VALUE value, int16_t* short_value);
	extern AMQP_VALUE amqpvalue_create_int(int32_t value);
	extern int amqpvalue_get_int(AMQP_VALUE value, int32_t* int_value);
	extern AMQP_VALUE amqpvalue_create_long(int64_t value);
	extern int amqpvalue_get_long(AMQP_VALUE value, int64_t* long_value);
	extern AMQP_VALUE amqpvalue_create_float(float value);
	extern int amqpvalue_get_float(AMQP_VALUE value, float* float_value);

	extern AMQP_VALUE amqpvalue_create_descriptor(AMQP_VALUE value);
	extern AMQP_VALUE amqpvalue_create_string(const char* value);
	extern AMQP_VALUE amqpvalue_create_string_with_length(const char* value, uint32_t length);
	extern AMQP_VALUE amqpvalue_create_list(size_t size);
	extern AMQP_VALUE amqpvalue_create_composite_with_ulong_descriptor(uint64_t descriptor, size_t size);
	extern AMQP_VALUE amqpvalue_create_binary(const void* value, uint32_t length);
	extern AMQP_VALUE amqpvalue_clone(AMQP_VALUE value);
	extern int amqpvalue_set_list_item(AMQP_VALUE value, size_t index, AMQP_VALUE list_item_value);
	extern int amqpvalue_get_type(AMQP_VALUE value, AMQP_TYPE* type);
	extern int amqpvalue_get_list_item_count(AMQP_VALUE value, size_t* count);
	extern AMQP_VALUE amqpvalue_get_composite_descriptor(AMQP_VALUE value);
	extern AMQP_VALUE amqpvalue_get_composite_list(AMQP_VALUE value);
	extern AMQP_VALUE amqpvalue_get_list_item(AMQP_VALUE value, size_t index);
	extern const char* amqpvalue_get_string(AMQP_VALUE value);
	extern const unsigned char* amqpvalue_get_binary_content(AMQP_VALUE value, uint32_t* length);
	extern AMQP_VALUE amqpvalue_get_descriptor(AMQP_VALUE value);
	extern void amqpvalue_destroy(AMQP_VALUE value);
	extern AMQP_VALUE amqpvalue_clone(AMQP_VALUE value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ANQPVALUE_H */
