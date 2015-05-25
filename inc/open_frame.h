#ifndef OPEN_FRAME_H
#define OPEN_FRAME_H

#include "io.h"
#include "logger.h"
#include "frame_codec.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	extern int open_frame_encode(FRAME_CODEC_HANDLE frame_codec, const char* container_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OPEN_FRAME_H */
