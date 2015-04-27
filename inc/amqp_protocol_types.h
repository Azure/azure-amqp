#ifndef AMQP_PROTOCOL_TYPES
#define AMQP_PROTOCOL_TYPES

#include <stdint.h>
#include "amqpvalue.h"

typedef uint32_t sequence_no;
typedef sequence_no transfer_number;

#define amqpvalue_create_sequence_no amqpvalue_create_uint
#define amqpvalue_create_transfer_number amqpvalue_create_sequence_no

#endif /* AMQP_PROTOCOL_TYPES */
