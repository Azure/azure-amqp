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
	typedef void(*FRAME_RECEIVED_CALLBACK)(void* context, uint64_t performative, AMQP_VALUE frame_list_value);

	extern FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, FRAME_RECEIVED_CALLBACK frame_received_callback, void* frame_received_callback_context, LOGGER_LOG logger_log);
	extern void frame_codec_destroy(FRAME_CODEC_HANDLE handle);
	extern int frame_codec_receive_bytes(FRAME_CODEC_HANDLE handle, const void* buffer, size_t size);
	extern int frame_codec_encode_frame_bytes(FRAME_CODEC_HANDLE frame_codec_handle, const void* bytes, size_t length);
	extern int frame_codec_start_encode_frame(FRAME_CODEC_HANDLE frame_codec_handle, size_t frame_payload_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FRAME_CODEC_H */
