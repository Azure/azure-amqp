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

#define AMQP_OPEN			(uint64_t)0x10
#define AMQP_BEGIN			(uint64_t)0x11
#define AMQP_ATTACH			(uint64_t)0x12
#define AMQP_FLOW			(uint64_t)0x13
#define AMQP_TRANSFER		(uint64_t)0x14
#define AMQP_DISPOSITION	(uint64_t)0x15
#define AMQP_DETACH			(uint64_t)0x16
#define AMQP_END			(uint64_t)0x17
#define AMQP_CLOSE			(uint64_t)0x18

typedef void* AMQP_FRAME_CODEC_HANDLE;
typedef int(*AMQP_EMPTY_FRAME_RECEIVED_CALLBACK)(void* context, uint16_t channel);
typedef int(*AMQP_FRAME_RECEIVED_CALLBACK)(void* context, uint16_t channel, AMQP_VALUE performative, uint32_t frame_payload_size);
typedef int(*AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK)(void* context, const unsigned char* payload_bytes, uint32_t byte_count);

extern AMQP_FRAME_CODEC_HANDLE amqp_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, AMQP_FRAME_RECEIVED_CALLBACK frame_received_callback, 
	AMQP_EMPTY_FRAME_RECEIVED_CALLBACK empty_frame_received_callback, AMQP_FRAME_PAYLOAD_BYTES_RECEIVED_CALLBACK payload_bytes_received_callback,
	void* frame_received_callback_context);
extern void amqp_frame_codec_destroy(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec);
extern int amqp_frame_codec_begin_encode_frame(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, uint16_t channel, const AMQP_VALUE performative, uint32_t payload_size);
extern int amqp_frame_codec_encode_payload_bytes(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, const unsigned char* bytes, uint32_t count);
extern int amqp_frame_codec_encode_empty_frame(AMQP_FRAME_CODEC_HANDLE amqp_frame_codec, uint16_t channel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQP_FRAME_CODEC_H */
