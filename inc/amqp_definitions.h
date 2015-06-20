

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

/* sender-settle-mode */

	typedef  sender-settle-mode;

	#define unsettled 0
	#define settled 1
	#define mixed 2

/* receiver-settle-mode */

	typedef  receiver-settle-mode;

	#define first 0
	#define second 1

/* handle */

	typedef uint32_t handle;


/* seconds */

	typedef uint32_t seconds;


/* milliseconds */

	typedef uint32_t milliseconds;


/* open */

	typedef void* OPEN_HANDLE;

	extern OPEN_HANDLE open_create(void);
	extern void open_destroy(OPEN_HANDLE open);

	extern int open_get_container-id(OPEN_HANDLE open, uint32_t* container-id);
	extern int open_set_container-id(OPEN_HANDLE open, uint32_t container-id);
	extern int open_get_hostname(OPEN_HANDLE open, uint32_t* hostname);
	extern int open_set_hostname(OPEN_HANDLE open, uint32_t hostname);
	extern int open_get_max-frame-size(OPEN_HANDLE open, uint32_t* max-frame-size);
	extern int open_set_max-frame-size(OPEN_HANDLE open, uint32_t max-frame-size);
	extern int open_get_channel-max(OPEN_HANDLE open, uint32_t* channel-max);
	extern int open_set_channel-max(OPEN_HANDLE open, uint32_t channel-max);
	extern int open_get_idle-time-out(OPEN_HANDLE open, uint32_t* idle-time-out);
	extern int open_set_idle-time-out(OPEN_HANDLE open, uint32_t idle-time-out);
	extern int open_get_outgoing-locales(OPEN_HANDLE open, uint32_t* outgoing-locales);
	extern int open_set_outgoing-locales(OPEN_HANDLE open, uint32_t outgoing-locales);
	extern int open_get_incoming-locales(OPEN_HANDLE open, uint32_t* incoming-locales);
	extern int open_set_incoming-locales(OPEN_HANDLE open, uint32_t incoming-locales);
	extern int open_get_offered-capabilities(OPEN_HANDLE open, uint32_t* offered-capabilities);
	extern int open_set_offered-capabilities(OPEN_HANDLE open, uint32_t offered-capabilities);
	extern int open_get_desired-capabilities(OPEN_HANDLE open, uint32_t* desired-capabilities);
	extern int open_set_desired-capabilities(OPEN_HANDLE open, uint32_t desired-capabilities);
	extern int open_get_properties(OPEN_HANDLE open, uint32_t* properties);
	extern int open_set_properties(OPEN_HANDLE open, uint32_t properties);


#ifdef __cplusplus
}
#endif

#endif /* AMQP_DEFINITIONS_H */
