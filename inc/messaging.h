#ifndef MESSAGING_H
#define MESSAGING_H

#include "amqpvalue.h"
#include "message_sender.h"
#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* MESSAGING_HANDLE;
	typedef void(*ON_MESSAGE_RECEIVE)(MESSAGE_SEND_RESULT send_result, const void* context);

	extern MESSAGING_HANDLE messaging_create(void);
	extern void messaging_destroy(MESSAGING_HANDLE handle);
	extern void messaging_dowork(MESSAGING_HANDLE handle);
	extern int messaging_send(MESSAGING_HANDLE handle, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, const void* callback_context);
	extern int messaging_receive(MESSAGING_HANDLE handle, const char* source, ON_MESSAGE_RECEIVE on_message_receive, const void* callback_context);
	extern AMQP_VALUE messaging_create_source(AMQP_VALUE address);
	extern AMQP_VALUE messaging_create_target(AMQP_VALUE address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGING_H */
