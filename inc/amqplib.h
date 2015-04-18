#ifndef AMQPLIB_H
#define AMQPLIB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* AMQPLIB_HANDLE;

extern int amqplib_init(void);
extern void amqplib_deinit(void);
extern AMQPLIB_HANDLE amqplib_create(const char* host, int port);
extern void amqplib_destroy(AMQPLIB_HANDLE handle);
extern int amqplib_dowork(AMQPLIB_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQPLIB_H */
