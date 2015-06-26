#ifndef MESSAGING_H
#define MESSAGING_H

#include "amqpvalue.h"
#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef enum MESSAGING_RESULT_TAG
	{
		MESSAGING_OK,
		MESSAGING_ERROR
	} MESSAGING_RESULT;

	typedef void* MESSAGING_HANDLE;
	typedef void(*MESSAGE_SEND_COMPLETE_CALLBACK)(MESSAGING_RESULT send_result, const void* context);

	extern MESSAGING_HANDLE messaging_create(void);
	extern void messaging_destroy(MESSAGING_HANDLE handle);
	extern void messaging_dowork(MESSAGING_HANDLE handle);
	extern int messaging_send(MESSAGING_HANDLE handle, MESSAGE_HANDLE message, MESSAGE_SEND_COMPLETE_CALLBACK callback, const void* callback_context);
	extern AMQP_VALUE messaging_create_source(AMQP_VALUE address);
	extern AMQP_VALUE messaging_create_target(AMQP_VALUE address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGING_H */
