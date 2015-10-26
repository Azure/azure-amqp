#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "message.h"
#include "amqpvalue.h"
#include "amqpalloc.h"

typedef struct MESSAGE_DATA_TAG
{
	unsigned char* body_data_section_bytes;
	size_t body_data_section_length;
	HEADER_HANDLE header;
	PROPERTIES_HANDLE properties;
} MESSAGE_DATA;

MESSAGE_HANDLE message_create(void)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)amqpalloc_malloc(sizeof(MESSAGE_DATA));
	/* Codes_SRS_MESSAGE_01_002: [If allocating memory for the message fails, message_create shall fail and return NULL.] */
	if (result != NULL)
	{
		result->header = NULL;
		result->properties = NULL;
		result->body_data_section_bytes = NULL;
		result->body_data_section_length = 0;
	}

	/* Codes_SRS_MESSAGE_01_001: [message_create shall create a new AMQP message instance and on success it shall return a non-NULL handle for the newly created message instance.] */
	return result;
}

MESSAGE_HANDLE message_clone(MESSAGE_HANDLE source_message)
{
	MESSAGE_DATA* result = (MESSAGE_DATA*)amqpalloc_malloc(sizeof(MESSAGE_DATA));
	MESSAGE_DATA* source_message_instance = (MESSAGE_DATA*)source_message;

	/* Codes_SRS_MESSAGE_01_003: [message_clone shall clone a message entirely and on success return a non-NULL handle to the cloned message.] */
	if (result != NULL)
	{
		result->body_data_section_length = source_message_instance->body_data_section_length;

		if (source_message_instance->header != NULL)
		{
			/* Codes_SRS_MESSAGE_01_005: [If a header exists on the source message it shall be cloned by using header_clone.] */
			result->header = header_clone(source_message_instance->header);
		}
		else
		{
			result->header = NULL;
		}

		if (source_message_instance->properties != NULL)
		{
			result->properties = properties_clone(source_message_instance->properties);
		}
		else
		{
			result->properties = NULL;
		}

		if (source_message_instance->body_data_section_length > 0)
		{
			result->body_data_section_bytes = amqpalloc_malloc(source_message_instance->body_data_section_length);
			if (result->body_data_section_bytes == NULL)
			{
				amqpalloc_free(result);
				result = NULL;
			}
		}
		else
		{
			result->body_data_section_bytes = NULL;
			result->body_data_section_length = 0;
		}
	}

	return result;
}

void message_destroy(MESSAGE_HANDLE message)
{
	if (message != NULL)
	{
		MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;
		if (message_instance->header != NULL)
		{
			header_destroy(message_instance->header);
		}
		if (message_instance->properties != NULL)
		{
			properties_destroy(message_instance->properties);
		}
		amqpalloc_free(message_instance->body_data_section_bytes);
		amqpalloc_free(message_instance);
	}
}

int message_set_header(MESSAGE_HANDLE message, HEADER_HANDLE header)
{
	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;

	if (message_instance->header != NULL)
	{
		header_destroy(message_instance->header);
	}

	message_instance->header = header_clone(header);

	return 0;
}

int message_get_header(MESSAGE_HANDLE message, HEADER_HANDLE* header)
{
	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;

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

int message_set_delivery_annotations(MESSAGE_HANDLE message, annotations delivery_annotations)
{
	return 0;
}

int message_get_delivery_annotations(MESSAGE_HANDLE message, annotations* delivery_annotations)
{
	return 0;
}

int message_set_message_annotations(MESSAGE_HANDLE message, annotations message_annotations)
{
	return 0;
}

int message_get_message_annotations(MESSAGE_HANDLE message, annotations* message_annotations)
{
	return 0;
}

int message_set_properties(MESSAGE_HANDLE message, PROPERTIES_HANDLE properties)
{
	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;

	if (message_instance->properties != NULL)
	{
		properties_destroy(message_instance->properties);
	}

	message_instance->properties = properties_clone(properties);

	return 0;
}

int message_get_properties(MESSAGE_HANDLE message, PROPERTIES_HANDLE* properties)
{
	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;

	if (message_instance->properties == NULL)
	{
		*properties = NULL;
	}
	else
	{
		*properties = properties_clone(message_instance->properties);
	}

	return 0;
}

int message_set_application_properties(MESSAGE_HANDLE message, AMQP_VALUE application_properties)
{
	return 0;
}

int message_get_application_properties(MESSAGE_HANDLE message, AMQP_VALUE* application_properties)
{
	return 0;
}

int message_set_footer(MESSAGE_HANDLE message, annotations footer)
{
	return 0;
}

int message_get_footer(MESSAGE_HANDLE message, annotations* footer)
{
	return 0;
}

int message_set_body_amqp_data(MESSAGE_HANDLE message, BINARY_DATA binary_data)
{
	int result;

	MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;
	if (message == NULL)
	{
		result = __LINE__;
	}
	else
	{
		message_instance->body_data_section_bytes = (unsigned char*)amqpalloc_malloc(binary_data.length);
		message_instance->body_data_section_length = binary_data.length;
		result = 0;
	}

	return result;
}

int message_get_body_amqp_data(MESSAGE_HANDLE message, BINARY_DATA* binary_data)
{
	int result;

	if ((message == NULL) ||
		(binary_data == NULL))
	{
		result = __LINE__;
	}
	else
	{
		MESSAGE_DATA* message_instance = (MESSAGE_DATA*)message;

		binary_data->bytes = message_instance->body_data_section_bytes;
		binary_data->length = message_instance->body_data_section_length;

		result = 0;
	}

	return result;
}
