#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include "link.h"
#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef enum MESSAGE_SEND_RESULT_TAG
	{
		MESSAGE_SEND_OK,
		MESSAGE_SEND_ERROR
	} MESSAGE_SEND_RESULT;

	typedef void* MESSAGE_SENDER_HANDLE;
	typedef void(*ON_MESSAGE_SEND_COMPLETE)(const void* context, MESSAGE_SEND_RESULT send_result);

	extern MESSAGE_SENDER_HANDLE messagesender_create(LINK_HANDLE link);
	extern void messagesender_destroy(MESSAGE_SENDER_HANDLE message_sender);
	extern int messagesender_send(MESSAGE_SENDER_HANDLE message_sender, MESSAGE_HANDLE message, ON_MESSAGE_SEND_COMPLETE on_message_send_complete, const void* callback_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGE_SENDER_H */
