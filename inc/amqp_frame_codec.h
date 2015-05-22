#ifndef AMQP_FRAME_CODEC_H
#define AMQP_FRAME_CODEC_H

#include "io.h"
#include "logger.h"
#include "frame_codec.h"

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#else
#include <stdint.h>
#endif /* __cplusplus */

extern int amqp_frame_codec_encode_open(FRAME_CODEC_HANDLE frame_codec, const char* container_id);
extern int amqp_frame_codec_encode_close(FRAME_CODEC_HANDLE frame_codec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_FRAME_CODEC_H */
