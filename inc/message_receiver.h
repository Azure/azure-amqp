#ifndef MESSAGE_RECEIVER_H
#define MESSAGE_RECEIVER_H

#include "link.h"
#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef void* MESSAGE_RECEIVER_HANDLE;
	typedef void(*ON_MESSAGE_RECEIVED)(const void* context, MESSAGE_HANDLE message);

	extern MESSAGE_RECEIVER_HANDLE messagesender_receiver(LINK_HANDLE link);
	extern void messagesender_destroy(MESSAGE_RECEIVER_HANDLE message_receiver);
	extern int messagesender_subscribe(MESSAGE_RECEIVER_HANDLE message_receiver, ON_MESSAGE_RECEIVED on_message_received, const void* callback_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGE_RECEIVER_H */
