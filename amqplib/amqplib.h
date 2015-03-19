#ifndef AMQPLIB_H
#define AMQPLIB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* AMQPLIB_HANDLE;

extern AMQPLIB_HANDLE amqplib_create(void);
extern void amqplib_destroy(AMQPLIB_HANDLE handle);
extern int amqplib_dowork(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AMQPLIB_H */
