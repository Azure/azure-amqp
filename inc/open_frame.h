#ifndef OPEN_FRAME_H
#define OPEN_FRAME_H

#include "io.h"
#include "logger.h"
#include "amqp_frame_codec.h"
#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* AMQP_OPEN_FRAME_HANDLE;

	extern AMQP_OPEN_FRAME_HANDLE open_frame_create(const char* container_id);
	extern AMQP_OPEN_FRAME_HANDLE open_frame_create_from_value(AMQP_VALUE value);
	extern void open_frame_destroy(AMQP_OPEN_FRAME_HANDLE amqp_open_frame);
	extern int open_frame_encode(AMQP_OPEN_FRAME_HANDLE amqp_open_frame, AMQP_FRAME_CODEC_HANDLE frame_codec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OPEN_FRAME_H */
