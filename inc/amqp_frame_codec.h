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
typedef void(*AMQP_EMPTY_FRAME_RECEIVED_CALLBACK)(void* context, uint16_t channel);
typedef void(*AMQP_FRAME_RECEIVED_CALLBACK)(void* context, uint16_t channel, uint64_t performative, AMQP_VALUE performative_type_value, const unsigned char* frame_payload, uint32_t frame_payload_size);

extern AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec_handle, AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback, 
	AMQP_EMPTY_FRAME_RECEIVED_CALLBACK empty_frame_received_callback, void* frame_received_callback_context);
extern void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_handle);
extern int amqp_frame_codec_encode(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_handle, uint64_t performative, const AMQP_VALUE* frame_content_chunks, size_t frame_content_chunk_count);
extern int amqp_frame_codec_decode_received_frame(void* context, uint8_t type, void* frame_body, uint32_t frame_body_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_FRAME_CODEC_H */
