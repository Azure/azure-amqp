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

typedef void* AMQP_FRAME_CODEC_HANDLE;
typedef void(*AMQP_FRAME_RECEIVED_CALLBACK)(void* context, uint64_t performative, AMQP_VALUE frame_list_value);

extern AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec_handle, AMQP_FRAME_RECEIVED_CALLBACK frame_receive_callback, void* frame_receive_callback_context);
extern void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_handle);
extern int amqp_frame_codec_encode(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_handle, uint64_t performative, const AMQP_VALUE* frame_content_chunks, size_t frame_content_chunk_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_FRAME_CODEC_H */
