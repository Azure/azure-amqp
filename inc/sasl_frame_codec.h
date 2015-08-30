#ifndef SASL_FRAME_CODEC_H
#define SASL_FRAME_CODEC_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */
#include "frame_codec.h"

#define SASL_MECHANISMS		(uint64_t)0x40
#define SASL_INIT			(uint64_t)0x41
#define SASL_CHALLENGE		(uint64_t)0x42
#define SASL_RESPONSE		(uint64_t)0x43
#define SASL_OUTCOME		(uint64_t)0x44

typedef void* SASL_FRAME_CODEC_HANDLE;
typedef void(*SASL_FRAME_RECEIVED_CALLBACK)(void* context, AMQP_VALUE sasl_frame);

extern SASL_FRAME_CODEC_HANDLE sasl_frame_codec_create(FRAME_CODEC_HANDLE frame_codec, SASL_FRAME_RECEIVED_CALLBACK frame_received_callback,
	void* frame_received_callback_context);
extern void sasl_frame_codec_destroy(SASL_FRAME_CODEC_HANDLE sasl_frame_codec);
extern int sasl_frame_codec_encode_frame(SASL_FRAME_CODEC_HANDLE sasl_frame_codec, const AMQP_VALUE sasl_frame, uint32_t payload_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SASL_FRAME_CODEC_H */
