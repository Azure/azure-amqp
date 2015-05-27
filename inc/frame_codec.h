#ifndef FRAME_CODEC_H
#define FRAME_CODEC_H

#include "io.h"
#include "logger.h"
#include "amqpvalue.h"

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

	typedef void* FRAME_CODEC_HANDLE;
	typedef void(*FRAME_BEGIN_CALLBACK)(void* context, uint32_t frame_body_size, const unsigned char* type_specific, uint32_t type_specific_size);
	typedef void(*FRAME_BODY_BYTES_RECEIVED_CALLBACK)(void* context, const unsigned char* frame_body_bytes, uint32_t frame_body_bytes_size);

	extern FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, LOGGER_LOG logger_log);
	extern void frame_codec_destroy(FRAME_CODEC_HANDLE frame_codec);
	extern int frame_codec_set_max_frame_size(FRAME_CODEC_HANDLE frame_codec, uint32_t max_frame_size);
	extern int frame_codec_subscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type, FRAME_BEGIN_CALLBACK frame_begin_callback, FRAME_BODY_BYTES_RECEIVED_CALLBACK frame_body_bytes_received_callback, void* callback_context);
	extern int frame_codec_unsubscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type);
	extern int frame_codec_receive_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* buffer, size_t size);
	extern int frame_codec_start_encode_frame(FRAME_CODEC_HANDLE frame_codec, size_t frame_payload_size);
	extern int frame_codec_encode_frame_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, size_t length);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FRAME_CODEC_H */
