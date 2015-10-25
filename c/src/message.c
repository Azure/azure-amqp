#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "message.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

typedef struct MESSAGE_DATA_TAG
{
	AMQP_VALUE to;
	unsigned char* body_data_section_bytes;
	size_t body_data_section_length;
	HEADER_HANDLE header;
} MESSAGE_DATA;

MESSAGE_HANDLE message_create(void)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)amqpalloc_malloc(sizeof(MESSAGE_DATA));
	if (result != NULL)
	{
		result->header = NULL;
		result->to = NULL;
		result->body_data_section_bytes = NULL;
		result->body_data_section_length = 0;
	}

	return result;
}

MESSAGE_HANDLE message_clone(MESSAGE_HANDLE source_message)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)amqpalloc_malloc(sizeof(MESSAGE_DATA));
	MESSAGE_DATA* source_message_instance = (MESSAGE_DATA*)source_message;

	if (result != NULL)
	{
		result->to = amqpvalue_clone(source_message_instance->to);
		result->body_data_section_length = source_message_instance->body_data_section_length;

		if (source_message_instance->body_data_section_length > 0)
		{
			result->body_data_section_bytes = amqpalloc_malloc(source_message_instance->body_data_section_length);
			if (result->body_data_section_bytes == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
		}
	}

	return result;
}

void message_destroy(MESSAGE_HANDLE handle)
{
	if (handle != NULL)
	{
		MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
		if (message->header != NULL)
		{
			header_destroy(message->header);
		}
		amqpvalue_destroy(message->to);
		amqpalloc_free(message->body_data_section_bytes);
		amqpalloc_free(handle);
	}
}

int message_set_to(MESSAGE_HANDLE handle, const char* to)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = __LINE__;
	}
	else
	{
		message->to = amqpvalue_create_string(to);
		if (message->to == NULL)
		{
			result = __LINE__;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

const char* message_get_to(MESSAGE_HANDLE handle)
{
	const char* result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = NULL;
	}
	else
	{
		if (amqpvalue_get_string(message->to, &result) != 0)
		{
			result = NULL;
		}
	}

	return result;
}

int message_set_header(MESSAGE_HANDLE handle, HEADER_HANDLE header)
{
	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)handle;

	if (message_instance->header != NULL)
	{
		header_destroy(message_instance->header);
	}

	message_instance->header = header_clone(header);

	return 0;
}

int message_get_header(MESSAGE_HANDLE handle, HEADER_HANDLE* header)
{
	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)handle;

	if (message_instance->header == NULL)
	{
		*header = NULL;
	}
	else
	{
		*header = header_clone(message_instance->header);
	}

	return 0;
}

int message_set_delivery_annotations(MESSAGE_HANDLE handle, annotations delivery_annotations)
{
	return 0;
}

int message_get_delivery_annotations(MESSAGE_HANDLE handle, annotations* delivery_annotations)
{
	return 0;
}

int message_set_message_annotations(MESSAGE_HANDLE handle, annotations message_annotations)
{
	return 0;
}

int message_get_message_annotations(MESSAGE_HANDLE handle, annotations* message_annotations)
{
	return 0;
}

int message_set_properties(MESSAGE_HANDLE handle, PROPERTIES_HANDLE properties)
{
	return 0;
}

int message_get_properties(MESSAGE_HANDLE handle, PROPERTIES_HANDLE* properties)
{
	return 0;
}

int message_set_application_properties(MESSAGE_HANDLE handle, AMQP_VALUE application_properties)
{
	return 0;
}

int message_get_application_properties(MESSAGE_HANDLE handle, AMQP_VALUE* application_properties)
{
	return 0;
}

int message_set_footer(MESSAGE_HANDLE handle, annotations footer)
{
	return 0;
}

int message_get_footer(MESSAGE_HANDLE handle, annotations* footer)
{
	return 0;
}

int message_set_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA binary_data)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if (message == NULL)
	{
		result = __LINE__;
	}
	else
	{
		message->body_data_section_bytes = (unsigned char*)amqpalloc_malloc(binary_data.length);
		message->body_data_section_length = binary_data.length;
		result = 0;
	}

	return result;
}

int message_get_body_amqp_data(MESSAGE_HANDLE handle, BINARY_DATA* binary_data)
{
	int result;

	MESSAGE_DATA* message = (MESSAGE_DATA*)handle;
	if ((message == NULL) ||
		(binary_data == NULL))
	{
		result = __LINE__;
	}
	else
	{
		binary_data->bytes = message->body_data_section_bytes;
		binary_data->length = message->body_data_section_length;

		result = 0;
	}

	return result;
}
