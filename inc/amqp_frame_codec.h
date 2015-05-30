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
typedef void(*AMQP_FRAME_RECEIVED_CALLBACK)(void* context, uint16_t channel, uint64_t performative, AMQP_VALUE performative_fields, uint32_t frame_payload_size);
typedef void(*AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK)(void* context, const unsigned char* payload_bytes, uint32_t byte_count);

extern AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback, 
	AMQP_EMPTY_FRAME_RECEIVED_CALLBACK empty_frame_received_callback, AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK payload_bytes_received_callback,
	void* frame_received_callback_context);
extern void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec);
extern int amqp_frame_codec_begin_encode_frame(FRAME_CODEC_HANDLE frame_codec, uint16_t channel, uint64_t performative, const AMQP_VALUE performative_fields, uint32_t payload_size);
extern int amqp_frame_codec_encode_payload_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, uint32_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_FRAME_CODEC_H */
