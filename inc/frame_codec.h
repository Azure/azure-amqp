#ifndef FRAME_CODEC_H
#define FRAME_CODEC_H

#include "io.h"
#include "logger.h"
#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#else
#include <stdint.h>
#endif /* __cplusplus */

	typedef void* FRAME_CODEC_HANDLE;

	extern FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, LOGGER_LOG logger_log);
	extern void frame_codec_free(FRAME_CODEC_HANDLE handle);
	extern int frame_codec_encode(FRAME_CODEC_HANDLE frame_codec, uint64_t performative, AMQP_VALUE frame_content);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FRAME_CODEC_H */
