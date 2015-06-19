#ifndef OPEN_FRAME_H
#define OPEN_FRAME_H

#include "io.h"
#include "logger.h"
#include "amqp_frame_codec.h"
#include "amqpvalue.h"
#include "amqp_protocol_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* AMQP_OPEN_FRAME_HANDLE;

	extern AMQP_OPEN_FRAME_HANDLE open_frame_create(const char* container_id);
	extern AMQP_OPEN_FRAME_HANDLE open_frame_create_from_value(AMQP_VALUE value);
	extern int open_frame_set_container_id(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, const char* container_id);
	extern const char* open_frame_get_container_id(AMQP_OPEN_FRAME_HANDLE amqp_open_frame);
	extern int open_frame_set_max_frame_size(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint32_t max_frame_size);
	extern int open_frame_get_max_frame_size(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint32_t* max_frame_size);
	extern int open_frame_set_channel_max(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint16_t channel_max);
	extern int open_frame_get_channel_max(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint16_t* channel_max);
	extern int open_frame_set_idle_timeout(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, milliseconds channel_max);
	extern int open_frame_get_idle_timeout(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, uint16_t* channel_max);
	extern void open_frame_destroy(AMQP_OPEN_FRAME_HANDLE amqp_open_frame);
	extern int open_frame_encode(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, AMQP_FRAME_CODEC_HANDLE frame_codec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OPEN_FRAME_H */
