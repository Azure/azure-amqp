#ifndef FRAME_CODEC_H
#define FRAME_CODEC_H

#include "io.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* FRAME_CODEC_HANDLE;

	extern FRAME_CODEC_HANDLE frame_codec_create(IO_HANDLE);
	extern void frame_codec_free(FRAME_CODEC_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FRAME_CODEC_H */
