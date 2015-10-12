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

/* Codes_SRS_FRAME_CODEC_01_016: [The type code indicates the format and purpose of the frame.] */
/* Codes_SRS_FRAME_CODEC_01_017: [The subsequent bytes in the frame header MAY be interpreted differently depending on the type of the frame.] */
/* Codes_SRS_FRAME_CODEC_01_070: [The type code indicates the format and purpose of the frame.] */
/* Codes_SRS_FRAME_CODEC_01_071: [The subsequent bytes in the frame header MAY be interpreted differently depending on the type of the frame.] */
/* Codes_SRS_FRAME_CODEC_01_018: [A type code of 0x00 indicates that the frame is an AMQP frame.] */
/* Codes_SRS_FRAME_CODEC_01_072: [A type code of 0x00 indicates that the frame is an AMQP frame.] */
#define FRAME_TYPE_AMQP	(uint8_t)0x00

/* Codes_SRS_FRAME_CODEC_01_073: [A type code of 0x01 indicates that the frame is a SASL frame] */
/* Codes_SRS_FRAME_CODEC_01_019: [A type code of 0x01 indicates that the frame is a SASL frame] */
#define FRAME_TYPE_SASL	(uint8_t)0x01

	typedef void* FRAME_CODEC_HANDLE;
	typedef void(*FRAME_RECEIVED_CALLBACK)(void* context, const unsigned char* type_specific, uint32_t type_specific_size, const unsigned char* frame_body, uint32_t frame_body_size);
	typedef void(*FRAME_CODEC_ERROR_CALLBACK)(void* context);

	extern FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE io, FRAME_CODEC_ERROR_CALLBACK frame_codec_error_callback, LOGGER_LOG logger_log);
	extern void frame_codec_destroy(FRAME_CODEC_HANDLE frame_codec);
	extern int frame_codec_set_max_frame_size(FRAME_CODEC_HANDLE frame_codec, uint32_t max_frame_size);
	extern int frame_codec_subscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type, FRAME_RECEIVED_CALLBACK frame_received_callback, void* callback_context);
	extern int frame_codec_unsubscribe(FRAME_CODEC_HANDLE frame_codec, uint8_t type);
	extern int frame_codec_receive_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* buffer, size_t size);
	extern int frame_codec_begin_encode_frame(FRAME_CODEC_HANDLE frame_codec, uint8_t type, uint32_t frame_body_size, const unsigned char* type_specific_bytes, uint32_t type_specific_size);
	extern int frame_codec_encode_frame_bytes(FRAME_CODEC_HANDLE frame_codec, const unsigned char* bytes, size_t length);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FRAME_CODEC_H */
