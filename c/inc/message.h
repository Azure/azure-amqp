#ifndef MESSAGE_H
#define MESSAGE_H

#include "amqpvalue.h"
#include "amqp_definitions.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

	typedef void* MESSAGE_HANDLE;
	typedef struct BINARY_DATA_TAG
	{
		const unsigned char* bytes;
		size_t length;
	} BINARY_DATA;

	extern MESSAGE_HANDLE message_create(void);
	extern MESSAGE_HANDLE message_clone(MESSAGE_HANDLE source_message);
	extern void message_destroy(MESSAGE_HANDLE handle);
	extern int message_set_header(MESSAGE_HANDLE handle, HEADER_HANDLE message_header);
	extern int message_get_header(MESSAGE_HANDLE handle, HEADER_HANDLE* message_header);
	extern int message_set_delivery_annotations(MESSAGE_HANDLE handle, annotations delivery_annotations);
	extern int message_get_delivery_annotations(MESSAGE_HANDLE handle, annotations* delivery_annotations);
	extern int message_set_message_annotations(MESSAGE_HANDLE handle, annotations delivery_annotations);
	extern int message_get_message_annotations(MESSAGE_HANDLE handle, annotations* delivery_annotations);
	extern int message_set_properties(MESSAGE_HANDLE handle, PROPERTIES_HANDLE properties);
	extern int message_get_properties(MESSAGE_HANDLE handle, PROPERTIES_HANDLE* properties);
	extern int message_set_application_properties(MESSAGE_HANDLE handle, AMQP_VALUE application_properties);
	extern int message_get_application_properties(MESSAGE_HANDLE handle, AMQP_VALUE* application_properties);
	extern int message_set_footer(MESSAGE_HANDLE handle, annotations footer);
	extern int message_get_footer(MESSAGE_HANDLE handle, annotations* footer);
	extern int message_set_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA binary_data);
	extern int message_get_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA* binary_data);
	extern int message_set_format(MESSAGE_HANDLE handle, message_format message_format);
	extern int message_get_format(MESSAGE_HANDLE handle, message_format* message_format);
	extern int message_set_raw_payload(MESSAGE_HANDLE handle, BINARY_DATA raw_payload);
	extern int message_get_raw_payload(MESSAGE_HANDLE handle, BINARY_DATA* raw_payload);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MESSAGE_H */
