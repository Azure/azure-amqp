

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

	typedef  sender_settle_mode;

	#define unsettled 0
	#define settled 1
	#define mixed 2

/* receiver-settle-mode */

	typedef  receiver_settle_mode;

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

	extern int open_get_container_id(OPEN_HANDLE open, uint32_t* container_id);
	extern int open_set_container_id(OPEN_HANDLE open, uint32_t container_id);
	extern int open_get_hostname(OPEN_HANDLE open, uint32_t* hostname);
	extern int open_set_hostname(OPEN_HANDLE open, uint32_t hostname);
	extern int open_get_max_frame_size(OPEN_HANDLE open, uint32_t* max_frame_size);
	extern int open_set_max_frame_size(OPEN_HANDLE open, uint32_t max_frame_size);
	extern int open_get_channel_max(OPEN_HANDLE open, uint32_t* channel_max);
	extern int open_set_channel_max(OPEN_HANDLE open, uint32_t channel_max);
	extern int open_get_idle_time_out(OPEN_HANDLE open, uint32_t* idle_time_out);
	extern int open_set_idle_time_out(OPEN_HANDLE open, uint32_t idle_time_out);
	extern int open_get_outgoing_locales(OPEN_HANDLE open, uint32_t* outgoing_locales);
	extern int open_set_outgoing_locales(OPEN_HANDLE open, uint32_t outgoing_locales);
	extern int open_get_incoming_locales(OPEN_HANDLE open, uint32_t* incoming_locales);
	extern int open_set_incoming_locales(OPEN_HANDLE open, uint32_t incoming_locales);
	extern int open_get_offered_capabilities(OPEN_HANDLE open, uint32_t* offered_capabilities);
	extern int open_set_offered_capabilities(OPEN_HANDLE open, uint32_t offered_capabilities);
	extern int open_get_desired_capabilities(OPEN_HANDLE open, uint32_t* desired_capabilities);
	extern int open_set_desired_capabilities(OPEN_HANDLE open, uint32_t desired_capabilities);
	extern int open_get_properties(OPEN_HANDLE open, uint32_t* properties);
	extern int open_set_properties(OPEN_HANDLE open, uint32_t properties);


#ifdef __cplusplus
}
#endif

#endif /* AMQP_DEFINITIONS_H */
