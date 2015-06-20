

#ifndef AMQP_DEFINITIONS_H
#define AMQP_DEFINITIONS_H

#ifdef __cplusplus
#include <cstdint>
#include <cstdbool>
extern "C" {
#else
#include <stdint.h>
#include <stdbool.h>
#endif

/* role */

	typedef bool role;

	#define sender false
	#define receiver true


#ifdef __cplusplus
}
#endif

#endif /* AMQP_DEFINITIONS_H */
