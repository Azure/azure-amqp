#ifndef AMQP_FRAME_CODEC_H
#define AMQP_FRAME_CODEC_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */
#include "frame_codec.h"

extern int amqp_frame_codec_encode(FRAME_CODEC_HANDLE frame_codec_handle, uint64_t performative, const AMQP_VALUE* frame_content_chunks, size_t frame_content_chunk_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_FRAME_CODEC_H */
