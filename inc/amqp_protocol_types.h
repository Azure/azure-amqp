#ifndef AMQP_PROTOCOL_TYPES
#define AMQP_PROTOCOL_TYPES

#include <stdint.h>
#include <stdbool.h>
#include "amqpvalue.h"

typedef uint32_t sequence_no;
typedef sequence_no transfer_number;
typedef sequence_no delivery_number;
typedef uint32_t handle;
typedef uint32_t milliseconds;

typedef enum role_tag
{
	sender = false,
	receiver = true
} role;

typedef enum sender_settle_mode_tag
{
	unsettled = 0,
	settled = 1,
	mixed = 2
} sender_settle_mode;

typedef enum receiver_settle_mode_tag
{
	first = 0,
	second = 1
} receiver_settle_mode;

#define amqpvalue_create_sequence_no amqpvalue_create_uint
#define amqpvalue_create_transfer_number amqpvalue_create_sequence_no
#define amqpvalue_create_handle amqpvalue_create_uint
#define amqpvalue_create_milliseconds amqpvalue_create_uint
#define amqpvalue_create_delivery_number amqpvalue_create_sequence_no
#define amqpvalue_create_role amqpvalue_create_boolean
#define amqpvalue_create_sender_settle_mode amqpvalue_create_ubyte
#define amqpvalue_create_receiver_settle_mode amqpvalue_create_ubyte
#define amqpvalue_create_delivery_tag amqpvalue_create_binary

#endif /* AMQP_PROTOCOL_TYPES */
