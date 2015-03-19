#ifndef IO_H
#define IO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* IO_HANDLE;
	typedef void(*IO_RECEIVE_CALLBACK)(IO_HANDLE handle, const void* buffer, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IO_H */
