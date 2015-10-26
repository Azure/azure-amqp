

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "amqpvalue.h"
#include "amqp_definitions.h"
#include "amqpalloc.h"
#include <stdlib.h>
#include <stdbool.h>

/* role */

AMQP_VALUE amqpvalue_create_role(role value)
{
	return amqpvalue_create_boolean(value);
}

/* sender-settle-mode */

AMQP_VALUE amqpvalue_create_sender_settle_mode(sender_settle_mode value)
{
	return amqpvalue_create_ubyte(value);
}

/* receiver-settle-mode */

AMQP_VALUE amqpvalue_create_receiver_settle_mode(receiver_settle_mode value)
{
	return amqpvalue_create_ubyte(value);
}

/* handle */

AMQP_VALUE amqpvalue_create_handle(handle value)
{
	return amqpvalue_create_uint(value);
}

/* seconds */

AMQP_VALUE amqpvalue_create_seconds(seconds value)
{
	return amqpvalue_create_uint(value);
}

/* milliseconds */

AMQP_VALUE amqpvalue_create_milliseconds(milliseconds value)
{
	return amqpvalue_create_uint(value);
}

/* delivery-tag */

AMQP_VALUE amqpvalue_create_delivery_tag(delivery_tag value)
{
	return amqpvalue_create_binary(value);
}

/* sequence-no */

AMQP_VALUE amqpvalue_create_sequence_no(sequence_no value)
{
	return amqpvalue_create_uint(value);
}

/* delivery-number */

AMQP_VALUE amqpvalue_create_delivery_number(delivery_number value)
{
	return amqpvalue_create_sequence_no(value);
}

/* transfer-number */

AMQP_VALUE amqpvalue_create_transfer_number(transfer_number value)
{
	return amqpvalue_create_sequence_no(value);
}

/* message-format */

AMQP_VALUE amqpvalue_create_message_format(message_format value)
{
	return amqpvalue_create_uint(value);
}

/* ietf-language-tag */

AMQP_VALUE amqpvalue_create_ietf_language_tag(ietf_language_tag value)
{
	return amqpvalue_create_symbol(value);
}

/* fields */

AMQP_VALUE amqpvalue_create_fields(AMQP_VALUE value)
{
	return amqpvalue_clone(value);
}

/* error */

typedef struct ERROR_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} ERROR_INSTANCE;

static ERROR_HANDLE error_create_internal(void)
{
	ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)amqpalloc_malloc(sizeof(ERROR_INSTANCE));
	if (error_instance != NULL)
	{
		error_instance->composite_value = NULL;
	}

	return error_instance;
}

ERROR_HANDLE error_create(const char* condition_value)
{
	ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)amqpalloc_malloc(sizeof(ERROR_INSTANCE));
	if (error_instance != NULL)
	{
		error_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(29);
		if (error_instance->composite_value == NULL)
		{
			amqpalloc_free(error_instance);
			error_instance = NULL;
		}
		else
		{
			AMQP_VALUE condition_amqp_value;
			int result = 0;

			condition_amqp_value = amqpvalue_create_symbol(condition_value);
			if ((result == 0) && (amqpvalue_set_composite_item(error_instance->composite_value, 0, condition_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(condition_amqp_value);
		}
	}

	return error_instance;
}

ERROR_HANDLE error_clone(ERROR_HANDLE value)
{
	ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)amqpalloc_malloc(sizeof(ERROR_INSTANCE));
	if (error_instance != NULL)
	{
		error_instance->composite_value = amqpvalue_clone(((ERROR_INSTANCE*)value)->composite_value);
		if (error_instance->composite_value == NULL)
		{
			amqpalloc_free(error_instance);
			error_instance = NULL;
		}
	}

	return error_instance;
}

void error_destroy(ERROR_HANDLE error)
{
	if (error != NULL)
	{
		ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		amqpvalue_destroy(error_instance->composite_value);
		amqpalloc_free(error_instance);
	}
}

AMQP_VALUE amqpvalue_create_error(ERROR_HANDLE error)
{
	AMQP_VALUE result;

	if (error == NULL)
	{
		result = NULL;
	}
	else
	{
		ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		result = amqpvalue_clone(error_instance->composite_value);
	}

	return result;
}

bool is_error_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 29))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_error(AMQP_VALUE value, ERROR_HANDLE* error_handle)
{
	int result;
	ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error_create_internal();
	*error_handle = error_instance;
	if (*error_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			error_destroy(*error_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* condition */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						error_destroy(*error_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					const char* condition;
					if (amqpvalue_get_symbol(item_value, &condition) != 0)
					{
						error_destroy(*error_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* description */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* description;
					if (amqpvalue_get_string(item_value, &description) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							error_destroy(*error_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* info */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					fields info;
					if (amqpvalue_get_fields(item_value, &info) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							error_destroy(*error_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				error_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int error_get_condition(ERROR_HANDLE error, const char** condition_value)
{
	int result;

	if (error == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(error_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, condition_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int error_set_condition(ERROR_HANDLE error, const char* condition_value)
{
	int result;

	if (error == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		AMQP_VALUE condition_amqp_value = amqpvalue_create_symbol(condition_value);
		if (condition_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(error_instance->composite_value, 0, condition_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(condition_amqp_value);
		}
	}

	return result;
}

int error_get_description(ERROR_HANDLE error, const char** description_value)
{
	int result;

	if (error == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(error_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, description_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int error_set_description(ERROR_HANDLE error, const char* description_value)
{
	int result;

	if (error == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		AMQP_VALUE description_amqp_value = amqpvalue_create_string(description_value);
		if (description_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(error_instance->composite_value, 1, description_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(description_amqp_value);
		}
	}

	return result;
}

int error_get_info(ERROR_HANDLE error, fields* info_value)
{
	int result;

	if (error == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(error_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_fields(item_value, info_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int error_set_info(ERROR_HANDLE error, fields info_value)
{
	int result;

	if (error == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
		AMQP_VALUE info_amqp_value = amqpvalue_create_fields(info_value);
		if (info_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(error_instance->composite_value, 2, info_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(info_amqp_value);
		}
	}

	return result;
}


/* amqp-error */

AMQP_VALUE amqpvalue_create_amqp_error(amqp_error value)
{
	return amqpvalue_create_symbol(value);
}

/* connection-error */

AMQP_VALUE amqpvalue_create_connection_error(connection_error value)
{
	return amqpvalue_create_symbol(value);
}

/* session-error */

AMQP_VALUE amqpvalue_create_session_error(session_error value)
{
	return amqpvalue_create_symbol(value);
}

/* link-error */

AMQP_VALUE amqpvalue_create_link_error(link_error value)
{
	return amqpvalue_create_symbol(value);
}

/* open */

typedef struct OPEN_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} OPEN_INSTANCE;

static OPEN_HANDLE open_create_internal(void)
{
	OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)amqpalloc_malloc(sizeof(OPEN_INSTANCE));
	if (open_instance != NULL)
	{
		open_instance->composite_value = NULL;
	}

	return open_instance;
}

OPEN_HANDLE open_create(const char* container_id_value)
{
	OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)amqpalloc_malloc(sizeof(OPEN_INSTANCE));
	if (open_instance != NULL)
	{
		open_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(16);
		if (open_instance->composite_value == NULL)
		{
			amqpalloc_free(open_instance);
			open_instance = NULL;
		}
		else
		{
			AMQP_VALUE container_id_amqp_value;
			int result = 0;

			container_id_amqp_value = amqpvalue_create_string(container_id_value);
			if ((result == 0) && (amqpvalue_set_composite_item(open_instance->composite_value, 0, container_id_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(container_id_amqp_value);
		}
	}

	return open_instance;
}

OPEN_HANDLE open_clone(OPEN_HANDLE value)
{
	OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)amqpalloc_malloc(sizeof(OPEN_INSTANCE));
	if (open_instance != NULL)
	{
		open_instance->composite_value = amqpvalue_clone(((OPEN_INSTANCE*)value)->composite_value);
		if (open_instance->composite_value == NULL)
		{
			amqpalloc_free(open_instance);
			open_instance = NULL;
		}
	}

	return open_instance;
}

void open_destroy(OPEN_HANDLE open)
{
	if (open != NULL)
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		amqpvalue_destroy(open_instance->composite_value);
		amqpalloc_free(open_instance);
	}
}

AMQP_VALUE amqpvalue_create_open(OPEN_HANDLE open)
{
	AMQP_VALUE result;

	if (open == NULL)
	{
		result = NULL;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		result = amqpvalue_clone(open_instance->composite_value);
	}

	return result;
}

bool is_open_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 16))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_open(AMQP_VALUE value, OPEN_HANDLE* open_handle)
{
	int result;
	OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open_create_internal();
	*open_handle = open_instance;
	if (*open_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			open_destroy(*open_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* container-id */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						open_destroy(*open_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					const char* container_id;
					if (amqpvalue_get_string(item_value, &container_id) != 0)
					{
						open_destroy(*open_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* hostname */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* hostname;
					if (amqpvalue_get_string(item_value, &hostname) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* max-frame-size */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint32_t max_frame_size;
					if (amqpvalue_get_uint(item_value, &max_frame_size) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* channel-max */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint16_t channel_max;
					if (amqpvalue_get_ushort(item_value, &channel_max) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* idle-time-out */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					milliseconds idle_time_out;
					if (amqpvalue_get_milliseconds(item_value, &idle_time_out) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* outgoing-locales */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					ietf_language_tag outgoing_locales;
					if (amqpvalue_get_ietf_language_tag(item_value, &outgoing_locales) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* incoming-locales */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					ietf_language_tag incoming_locales;
					if (amqpvalue_get_ietf_language_tag(item_value, &incoming_locales) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* offered-capabilities */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* offered_capabilities;
					if (amqpvalue_get_symbol(item_value, &offered_capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* desired-capabilities */
				item_value = amqpvalue_get_list_item(list_value, 8);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* desired_capabilities;
					if (amqpvalue_get_symbol(item_value, &desired_capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* properties */
				item_value = amqpvalue_get_list_item(list_value, 9);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					fields properties;
					if (amqpvalue_get_fields(item_value, &properties) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							open_destroy(*open_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				open_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int open_get_container_id(OPEN_HANDLE open, const char** container_id_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, container_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_container_id(OPEN_HANDLE open, const char* container_id_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE container_id_amqp_value = amqpvalue_create_string(container_id_value);
		if (container_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 0, container_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(container_id_amqp_value);
		}
	}

	return result;
}

int open_get_hostname(OPEN_HANDLE open, const char** hostname_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, hostname_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_hostname(OPEN_HANDLE open, const char* hostname_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE hostname_amqp_value = amqpvalue_create_string(hostname_value);
		if (hostname_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 1, hostname_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(hostname_amqp_value);
		}
	}

	return result;
}

int open_get_max_frame_size(OPEN_HANDLE open, uint32_t* max_frame_size_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, max_frame_size_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_max_frame_size(OPEN_HANDLE open, uint32_t max_frame_size_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE max_frame_size_amqp_value = amqpvalue_create_uint(max_frame_size_value);
		if (max_frame_size_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 2, max_frame_size_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(max_frame_size_amqp_value);
		}
	}

	return result;
}

int open_get_channel_max(OPEN_HANDLE open, uint16_t* channel_max_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_ushort(item_value, channel_max_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_channel_max(OPEN_HANDLE open, uint16_t channel_max_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE channel_max_amqp_value = amqpvalue_create_ushort(channel_max_value);
		if (channel_max_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 3, channel_max_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(channel_max_amqp_value);
		}
	}

	return result;
}

int open_get_idle_time_out(OPEN_HANDLE open, milliseconds* idle_time_out_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_milliseconds(item_value, idle_time_out_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_idle_time_out(OPEN_HANDLE open, milliseconds idle_time_out_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE idle_time_out_amqp_value = amqpvalue_create_milliseconds(idle_time_out_value);
		if (idle_time_out_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 4, idle_time_out_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(idle_time_out_amqp_value);
		}
	}

	return result;
}

int open_get_outgoing_locales(OPEN_HANDLE open, ietf_language_tag* outgoing_locales_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_ietf_language_tag(item_value, outgoing_locales_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_outgoing_locales(OPEN_HANDLE open, ietf_language_tag outgoing_locales_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE outgoing_locales_amqp_value = amqpvalue_create_ietf_language_tag(outgoing_locales_value);
		if (outgoing_locales_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 5, outgoing_locales_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(outgoing_locales_amqp_value);
		}
	}

	return result;
}

int open_get_incoming_locales(OPEN_HANDLE open, ietf_language_tag* incoming_locales_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_ietf_language_tag(item_value, incoming_locales_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_incoming_locales(OPEN_HANDLE open, ietf_language_tag incoming_locales_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE incoming_locales_amqp_value = amqpvalue_create_ietf_language_tag(incoming_locales_value);
		if (incoming_locales_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 6, incoming_locales_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(incoming_locales_amqp_value);
		}
	}

	return result;
}

int open_get_offered_capabilities(OPEN_HANDLE open, const char** offered_capabilities_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, offered_capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_offered_capabilities(OPEN_HANDLE open, const char* offered_capabilities_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE offered_capabilities_amqp_value = amqpvalue_create_symbol(offered_capabilities_value);
		if (offered_capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 7, offered_capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(offered_capabilities_amqp_value);
		}
	}

	return result;
}

int open_get_desired_capabilities(OPEN_HANDLE open, const char** desired_capabilities_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 8);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, desired_capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_desired_capabilities(OPEN_HANDLE open, const char* desired_capabilities_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE desired_capabilities_amqp_value = amqpvalue_create_symbol(desired_capabilities_value);
		if (desired_capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 8, desired_capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(desired_capabilities_amqp_value);
		}
	}

	return result;
}

int open_get_properties(OPEN_HANDLE open, fields* properties_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(open_instance->composite_value, 9);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_fields(item_value, properties_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int open_set_properties(OPEN_HANDLE open, fields properties_value)
{
	int result;

	if (open == NULL)
	{
		result = __LINE__;
	}
	else
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
		AMQP_VALUE properties_amqp_value = amqpvalue_create_fields(properties_value);
		if (properties_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(open_instance->composite_value, 9, properties_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(properties_amqp_value);
		}
	}

	return result;
}


/* begin */

typedef struct BEGIN_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} BEGIN_INSTANCE;

static BEGIN_HANDLE begin_create_internal(void)
{
	BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)amqpalloc_malloc(sizeof(BEGIN_INSTANCE));
	if (begin_instance != NULL)
	{
		begin_instance->composite_value = NULL;
	}

	return begin_instance;
}

BEGIN_HANDLE begin_create(transfer_number next_outgoing_id_value, uint32_t incoming_window_value, uint32_t outgoing_window_value)
{
	BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)amqpalloc_malloc(sizeof(BEGIN_INSTANCE));
	if (begin_instance != NULL)
	{
		begin_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(17);
		if (begin_instance->composite_value == NULL)
		{
			amqpalloc_free(begin_instance);
			begin_instance = NULL;
		}
		else
		{
			AMQP_VALUE next_outgoing_id_amqp_value;
			AMQP_VALUE incoming_window_amqp_value;
			AMQP_VALUE outgoing_window_amqp_value;
			int result = 0;

			next_outgoing_id_amqp_value = amqpvalue_create_transfer_number(next_outgoing_id_value);
			if ((result == 0) && (amqpvalue_set_composite_item(begin_instance->composite_value, 1, next_outgoing_id_amqp_value) != 0))
			{
				result = __LINE__;
			}
			incoming_window_amqp_value = amqpvalue_create_uint(incoming_window_value);
			if ((result == 0) && (amqpvalue_set_composite_item(begin_instance->composite_value, 2, incoming_window_amqp_value) != 0))
			{
				result = __LINE__;
			}
			outgoing_window_amqp_value = amqpvalue_create_uint(outgoing_window_value);
			if ((result == 0) && (amqpvalue_set_composite_item(begin_instance->composite_value, 3, outgoing_window_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(next_outgoing_id_amqp_value);
			amqpvalue_destroy(incoming_window_amqp_value);
			amqpvalue_destroy(outgoing_window_amqp_value);
		}
	}

	return begin_instance;
}

BEGIN_HANDLE begin_clone(BEGIN_HANDLE value)
{
	BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)amqpalloc_malloc(sizeof(BEGIN_INSTANCE));
	if (begin_instance != NULL)
	{
		begin_instance->composite_value = amqpvalue_clone(((BEGIN_INSTANCE*)value)->composite_value);
		if (begin_instance->composite_value == NULL)
		{
			amqpalloc_free(begin_instance);
			begin_instance = NULL;
		}
	}

	return begin_instance;
}

void begin_destroy(BEGIN_HANDLE begin)
{
	if (begin != NULL)
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		amqpvalue_destroy(begin_instance->composite_value);
		amqpalloc_free(begin_instance);
	}
}

AMQP_VALUE amqpvalue_create_begin(BEGIN_HANDLE begin)
{
	AMQP_VALUE result;

	if (begin == NULL)
	{
		result = NULL;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		result = amqpvalue_clone(begin_instance->composite_value);
	}

	return result;
}

bool is_begin_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 17))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_begin(AMQP_VALUE value, BEGIN_HANDLE* begin_handle)
{
	int result;
	BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin_create_internal();
	*begin_handle = begin_instance;
	if (*begin_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			begin_destroy(*begin_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* remote-channel */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint16_t remote_channel;
					if (amqpvalue_get_ushort(item_value, &remote_channel) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							begin_destroy(*begin_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* next-outgoing-id */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					{
						begin_destroy(*begin_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					transfer_number next_outgoing_id;
					if (amqpvalue_get_transfer_number(item_value, &next_outgoing_id) != 0)
					{
						begin_destroy(*begin_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* incoming-window */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					{
						begin_destroy(*begin_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					uint32_t incoming_window;
					if (amqpvalue_get_uint(item_value, &incoming_window) != 0)
					{
						begin_destroy(*begin_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* outgoing-window */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					{
						begin_destroy(*begin_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					uint32_t outgoing_window;
					if (amqpvalue_get_uint(item_value, &outgoing_window) != 0)
					{
						begin_destroy(*begin_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* handle-max */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					handle handle_max;
					if (amqpvalue_get_handle(item_value, &handle_max) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							begin_destroy(*begin_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* offered-capabilities */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* offered_capabilities;
					if (amqpvalue_get_symbol(item_value, &offered_capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							begin_destroy(*begin_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* desired-capabilities */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* desired_capabilities;
					if (amqpvalue_get_symbol(item_value, &desired_capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							begin_destroy(*begin_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* properties */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					fields properties;
					if (amqpvalue_get_fields(item_value, &properties) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							begin_destroy(*begin_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				begin_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int begin_get_remote_channel(BEGIN_HANDLE begin, uint16_t* remote_channel_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_ushort(item_value, remote_channel_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_remote_channel(BEGIN_HANDLE begin, uint16_t remote_channel_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE remote_channel_amqp_value = amqpvalue_create_ushort(remote_channel_value);
		if (remote_channel_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 0, remote_channel_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(remote_channel_amqp_value);
		}
	}

	return result;
}

int begin_get_next_outgoing_id(BEGIN_HANDLE begin, transfer_number* next_outgoing_id_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_transfer_number(item_value, next_outgoing_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_next_outgoing_id(BEGIN_HANDLE begin, transfer_number next_outgoing_id_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE next_outgoing_id_amqp_value = amqpvalue_create_transfer_number(next_outgoing_id_value);
		if (next_outgoing_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 1, next_outgoing_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(next_outgoing_id_amqp_value);
		}
	}

	return result;
}

int begin_get_incoming_window(BEGIN_HANDLE begin, uint32_t* incoming_window_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, incoming_window_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_incoming_window(BEGIN_HANDLE begin, uint32_t incoming_window_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE incoming_window_amqp_value = amqpvalue_create_uint(incoming_window_value);
		if (incoming_window_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 2, incoming_window_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(incoming_window_amqp_value);
		}
	}

	return result;
}

int begin_get_outgoing_window(BEGIN_HANDLE begin, uint32_t* outgoing_window_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, outgoing_window_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_outgoing_window(BEGIN_HANDLE begin, uint32_t outgoing_window_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE outgoing_window_amqp_value = amqpvalue_create_uint(outgoing_window_value);
		if (outgoing_window_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 3, outgoing_window_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(outgoing_window_amqp_value);
		}
	}

	return result;
}

int begin_get_handle_max(BEGIN_HANDLE begin, handle* handle_max_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_handle(item_value, handle_max_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_handle_max(BEGIN_HANDLE begin, handle handle_max_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE handle_max_amqp_value = amqpvalue_create_handle(handle_max_value);
		if (handle_max_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 4, handle_max_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(handle_max_amqp_value);
		}
	}

	return result;
}

int begin_get_offered_capabilities(BEGIN_HANDLE begin, const char** offered_capabilities_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, offered_capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_offered_capabilities(BEGIN_HANDLE begin, const char* offered_capabilities_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE offered_capabilities_amqp_value = amqpvalue_create_symbol(offered_capabilities_value);
		if (offered_capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 5, offered_capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(offered_capabilities_amqp_value);
		}
	}

	return result;
}

int begin_get_desired_capabilities(BEGIN_HANDLE begin, const char** desired_capabilities_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, desired_capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_desired_capabilities(BEGIN_HANDLE begin, const char* desired_capabilities_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE desired_capabilities_amqp_value = amqpvalue_create_symbol(desired_capabilities_value);
		if (desired_capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 6, desired_capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(desired_capabilities_amqp_value);
		}
	}

	return result;
}

int begin_get_properties(BEGIN_HANDLE begin, fields* properties_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(begin_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_fields(item_value, properties_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int begin_set_properties(BEGIN_HANDLE begin, fields properties_value)
{
	int result;

	if (begin == NULL)
	{
		result = __LINE__;
	}
	else
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
		AMQP_VALUE properties_amqp_value = amqpvalue_create_fields(properties_value);
		if (properties_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(begin_instance->composite_value, 7, properties_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(properties_amqp_value);
		}
	}

	return result;
}


/* attach */

typedef struct ATTACH_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} ATTACH_INSTANCE;

static ATTACH_HANDLE attach_create_internal(void)
{
	ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)amqpalloc_malloc(sizeof(ATTACH_INSTANCE));
	if (attach_instance != NULL)
	{
		attach_instance->composite_value = NULL;
	}

	return attach_instance;
}

ATTACH_HANDLE attach_create(const char* name_value, handle handle_value, role role_value)
{
	ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)amqpalloc_malloc(sizeof(ATTACH_INSTANCE));
	if (attach_instance != NULL)
	{
		attach_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(18);
		if (attach_instance->composite_value == NULL)
		{
			amqpalloc_free(attach_instance);
			attach_instance = NULL;
		}
		else
		{
			AMQP_VALUE name_amqp_value;
			AMQP_VALUE handle_amqp_value;
			AMQP_VALUE role_amqp_value;
			int result = 0;

			name_amqp_value = amqpvalue_create_string(name_value);
			if ((result == 0) && (amqpvalue_set_composite_item(attach_instance->composite_value, 0, name_amqp_value) != 0))
			{
				result = __LINE__;
			}
			handle_amqp_value = amqpvalue_create_handle(handle_value);
			if ((result == 0) && (amqpvalue_set_composite_item(attach_instance->composite_value, 1, handle_amqp_value) != 0))
			{
				result = __LINE__;
			}
			role_amqp_value = amqpvalue_create_role(role_value);
			if ((result == 0) && (amqpvalue_set_composite_item(attach_instance->composite_value, 2, role_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(name_amqp_value);
			amqpvalue_destroy(handle_amqp_value);
			amqpvalue_destroy(role_amqp_value);
		}
	}

	return attach_instance;
}

ATTACH_HANDLE attach_clone(ATTACH_HANDLE value)
{
	ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)amqpalloc_malloc(sizeof(ATTACH_INSTANCE));
	if (attach_instance != NULL)
	{
		attach_instance->composite_value = amqpvalue_clone(((ATTACH_INSTANCE*)value)->composite_value);
		if (attach_instance->composite_value == NULL)
		{
			amqpalloc_free(attach_instance);
			attach_instance = NULL;
		}
	}

	return attach_instance;
}

void attach_destroy(ATTACH_HANDLE attach)
{
	if (attach != NULL)
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		amqpvalue_destroy(attach_instance->composite_value);
		amqpalloc_free(attach_instance);
	}
}

AMQP_VALUE amqpvalue_create_attach(ATTACH_HANDLE attach)
{
	AMQP_VALUE result;

	if (attach == NULL)
	{
		result = NULL;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		result = amqpvalue_clone(attach_instance->composite_value);
	}

	return result;
}

bool is_attach_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 18))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_attach(AMQP_VALUE value, ATTACH_HANDLE* attach_handle)
{
	int result;
	ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach_create_internal();
	*attach_handle = attach_instance;
	if (*attach_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			attach_destroy(*attach_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* name */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						attach_destroy(*attach_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					const char* name;
					if (amqpvalue_get_string(item_value, &name) != 0)
					{
						attach_destroy(*attach_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* handle */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					{
						attach_destroy(*attach_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					handle handle;
					if (amqpvalue_get_handle(item_value, &handle) != 0)
					{
						attach_destroy(*attach_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* role */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					{
						attach_destroy(*attach_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					role role;
					if (amqpvalue_get_role(item_value, &role) != 0)
					{
						attach_destroy(*attach_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* snd-settle-mode */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					sender_settle_mode snd_settle_mode;
					if (amqpvalue_get_sender_settle_mode(item_value, &snd_settle_mode) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* rcv-settle-mode */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					receiver_settle_mode rcv_settle_mode;
					if (amqpvalue_get_receiver_settle_mode(item_value, &rcv_settle_mode) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* source */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* target */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* unsettled */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					AMQP_VALUE unsettled;
					if (amqpvalue_get_map(item_value, &unsettled) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* incomplete-unsettled */
				item_value = amqpvalue_get_list_item(list_value, 8);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool incomplete_unsettled;
					if (amqpvalue_get_boolean(item_value, &incomplete_unsettled) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* initial-delivery-count */
				item_value = amqpvalue_get_list_item(list_value, 9);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					sequence_no initial_delivery_count;
					if (amqpvalue_get_sequence_no(item_value, &initial_delivery_count) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* max-message-size */
				item_value = amqpvalue_get_list_item(list_value, 10);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint64_t max_message_size;
					if (amqpvalue_get_ulong(item_value, &max_message_size) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* offered-capabilities */
				item_value = amqpvalue_get_list_item(list_value, 11);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* offered_capabilities;
					if (amqpvalue_get_symbol(item_value, &offered_capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* desired-capabilities */
				item_value = amqpvalue_get_list_item(list_value, 12);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* desired_capabilities;
					if (amqpvalue_get_symbol(item_value, &desired_capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* properties */
				item_value = amqpvalue_get_list_item(list_value, 13);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					fields properties;
					if (amqpvalue_get_fields(item_value, &properties) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							attach_destroy(*attach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				attach_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int attach_get_name(ATTACH_HANDLE attach, const char** name_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, name_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_name(ATTACH_HANDLE attach, const char* name_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE name_amqp_value = amqpvalue_create_string(name_value);
		if (name_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 0, name_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(name_amqp_value);
		}
	}

	return result;
}

int attach_get_handle(ATTACH_HANDLE attach, handle* handle_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_handle(item_value, handle_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_handle(ATTACH_HANDLE attach, handle handle_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE handle_amqp_value = amqpvalue_create_handle(handle_value);
		if (handle_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 1, handle_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(handle_amqp_value);
		}
	}

	return result;
}

int attach_get_role(ATTACH_HANDLE attach, role* role_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_role(item_value, role_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_role(ATTACH_HANDLE attach, role role_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE role_amqp_value = amqpvalue_create_role(role_value);
		if (role_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 2, role_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(role_amqp_value);
		}
	}

	return result;
}

int attach_get_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode* snd_settle_mode_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_sender_settle_mode(item_value, snd_settle_mode_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode snd_settle_mode_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE snd_settle_mode_amqp_value = amqpvalue_create_sender_settle_mode(snd_settle_mode_value);
		if (snd_settle_mode_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 3, snd_settle_mode_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(snd_settle_mode_amqp_value);
		}
	}

	return result;
}

int attach_get_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode* rcv_settle_mode_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_receiver_settle_mode(item_value, rcv_settle_mode_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode rcv_settle_mode_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE rcv_settle_mode_amqp_value = amqpvalue_create_receiver_settle_mode(rcv_settle_mode_value);
		if (rcv_settle_mode_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 4, rcv_settle_mode_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(rcv_settle_mode_amqp_value);
		}
	}

	return result;
}

int attach_get_source(ATTACH_HANDLE attach, AMQP_VALUE* source_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*source_value = item_value;
			result = 0;
		}
	}

	return result;
}

int attach_set_source(ATTACH_HANDLE attach, AMQP_VALUE source_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE source_amqp_value = amqpvalue_clone(source_value);
		if (source_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 5, source_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(source_amqp_value);
		}
	}

	return result;
}

int attach_get_target(ATTACH_HANDLE attach, AMQP_VALUE* target_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*target_value = item_value;
			result = 0;
		}
	}

	return result;
}

int attach_set_target(ATTACH_HANDLE attach, AMQP_VALUE target_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE target_amqp_value = amqpvalue_clone(target_value);
		if (target_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 6, target_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(target_amqp_value);
		}
	}

	return result;
}

int attach_get_unsettled(ATTACH_HANDLE attach, AMQP_VALUE* unsettled_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_map(item_value, unsettled_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_unsettled(ATTACH_HANDLE attach, AMQP_VALUE unsettled_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE unsettled_amqp_value = amqpvalue_clone(unsettled_value);
		if (unsettled_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 7, unsettled_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(unsettled_amqp_value);
		}
	}

	return result;
}

int attach_get_incomplete_unsettled(ATTACH_HANDLE attach, bool* incomplete_unsettled_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 8);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, incomplete_unsettled_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_incomplete_unsettled(ATTACH_HANDLE attach, bool incomplete_unsettled_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE incomplete_unsettled_amqp_value = amqpvalue_create_boolean(incomplete_unsettled_value);
		if (incomplete_unsettled_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 8, incomplete_unsettled_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(incomplete_unsettled_amqp_value);
		}
	}

	return result;
}

int attach_get_initial_delivery_count(ATTACH_HANDLE attach, sequence_no* initial_delivery_count_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 9);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_sequence_no(item_value, initial_delivery_count_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_initial_delivery_count(ATTACH_HANDLE attach, sequence_no initial_delivery_count_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE initial_delivery_count_amqp_value = amqpvalue_create_sequence_no(initial_delivery_count_value);
		if (initial_delivery_count_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 9, initial_delivery_count_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(initial_delivery_count_amqp_value);
		}
	}

	return result;
}

int attach_get_max_message_size(ATTACH_HANDLE attach, uint64_t* max_message_size_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 10);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_ulong(item_value, max_message_size_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_max_message_size(ATTACH_HANDLE attach, uint64_t max_message_size_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE max_message_size_amqp_value = amqpvalue_create_ulong(max_message_size_value);
		if (max_message_size_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 10, max_message_size_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(max_message_size_amqp_value);
		}
	}

	return result;
}

int attach_get_offered_capabilities(ATTACH_HANDLE attach, const char** offered_capabilities_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 11);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, offered_capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_offered_capabilities(ATTACH_HANDLE attach, const char* offered_capabilities_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE offered_capabilities_amqp_value = amqpvalue_create_symbol(offered_capabilities_value);
		if (offered_capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 11, offered_capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(offered_capabilities_amqp_value);
		}
	}

	return result;
}

int attach_get_desired_capabilities(ATTACH_HANDLE attach, const char** desired_capabilities_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 12);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, desired_capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_desired_capabilities(ATTACH_HANDLE attach, const char* desired_capabilities_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE desired_capabilities_amqp_value = amqpvalue_create_symbol(desired_capabilities_value);
		if (desired_capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 12, desired_capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(desired_capabilities_amqp_value);
		}
	}

	return result;
}

int attach_get_properties(ATTACH_HANDLE attach, fields* properties_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(attach_instance->composite_value, 13);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_fields(item_value, properties_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int attach_set_properties(ATTACH_HANDLE attach, fields properties_value)
{
	int result;

	if (attach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
		AMQP_VALUE properties_amqp_value = amqpvalue_create_fields(properties_value);
		if (properties_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(attach_instance->composite_value, 13, properties_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(properties_amqp_value);
		}
	}

	return result;
}


/* flow */

typedef struct FLOW_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} FLOW_INSTANCE;

static FLOW_HANDLE flow_create_internal(void)
{
	FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)amqpalloc_malloc(sizeof(FLOW_INSTANCE));
	if (flow_instance != NULL)
	{
		flow_instance->composite_value = NULL;
	}

	return flow_instance;
}

FLOW_HANDLE flow_create(uint32_t incoming_window_value, transfer_number next_outgoing_id_value, uint32_t outgoing_window_value)
{
	FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)amqpalloc_malloc(sizeof(FLOW_INSTANCE));
	if (flow_instance != NULL)
	{
		flow_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(19);
		if (flow_instance->composite_value == NULL)
		{
			amqpalloc_free(flow_instance);
			flow_instance = NULL;
		}
		else
		{
			AMQP_VALUE incoming_window_amqp_value;
			AMQP_VALUE next_outgoing_id_amqp_value;
			AMQP_VALUE outgoing_window_amqp_value;
			int result = 0;

			incoming_window_amqp_value = amqpvalue_create_uint(incoming_window_value);
			if ((result == 0) && (amqpvalue_set_composite_item(flow_instance->composite_value, 1, incoming_window_amqp_value) != 0))
			{
				result = __LINE__;
			}
			next_outgoing_id_amqp_value = amqpvalue_create_transfer_number(next_outgoing_id_value);
			if ((result == 0) && (amqpvalue_set_composite_item(flow_instance->composite_value, 2, next_outgoing_id_amqp_value) != 0))
			{
				result = __LINE__;
			}
			outgoing_window_amqp_value = amqpvalue_create_uint(outgoing_window_value);
			if ((result == 0) && (amqpvalue_set_composite_item(flow_instance->composite_value, 3, outgoing_window_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(incoming_window_amqp_value);
			amqpvalue_destroy(next_outgoing_id_amqp_value);
			amqpvalue_destroy(outgoing_window_amqp_value);
		}
	}

	return flow_instance;
}

FLOW_HANDLE flow_clone(FLOW_HANDLE value)
{
	FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)amqpalloc_malloc(sizeof(FLOW_INSTANCE));
	if (flow_instance != NULL)
	{
		flow_instance->composite_value = amqpvalue_clone(((FLOW_INSTANCE*)value)->composite_value);
		if (flow_instance->composite_value == NULL)
		{
			amqpalloc_free(flow_instance);
			flow_instance = NULL;
		}
	}

	return flow_instance;
}

void flow_destroy(FLOW_HANDLE flow)
{
	if (flow != NULL)
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		amqpvalue_destroy(flow_instance->composite_value);
		amqpalloc_free(flow_instance);
	}
}

AMQP_VALUE amqpvalue_create_flow(FLOW_HANDLE flow)
{
	AMQP_VALUE result;

	if (flow == NULL)
	{
		result = NULL;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		result = amqpvalue_clone(flow_instance->composite_value);
	}

	return result;
}

bool is_flow_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 19))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_flow(AMQP_VALUE value, FLOW_HANDLE* flow_handle)
{
	int result;
	FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow_create_internal();
	*flow_handle = flow_instance;
	if (*flow_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			flow_destroy(*flow_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* next-incoming-id */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					transfer_number next_incoming_id;
					if (amqpvalue_get_transfer_number(item_value, &next_incoming_id) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* incoming-window */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					{
						flow_destroy(*flow_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					uint32_t incoming_window;
					if (amqpvalue_get_uint(item_value, &incoming_window) != 0)
					{
						flow_destroy(*flow_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* next-outgoing-id */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					{
						flow_destroy(*flow_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					transfer_number next_outgoing_id;
					if (amqpvalue_get_transfer_number(item_value, &next_outgoing_id) != 0)
					{
						flow_destroy(*flow_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* outgoing-window */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					{
						flow_destroy(*flow_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					uint32_t outgoing_window;
					if (amqpvalue_get_uint(item_value, &outgoing_window) != 0)
					{
						flow_destroy(*flow_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* handle */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					handle handle;
					if (amqpvalue_get_handle(item_value, &handle) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* delivery-count */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					sequence_no delivery_count;
					if (amqpvalue_get_sequence_no(item_value, &delivery_count) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* link-credit */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint32_t link_credit;
					if (amqpvalue_get_uint(item_value, &link_credit) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* available */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint32_t available;
					if (amqpvalue_get_uint(item_value, &available) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* drain */
				item_value = amqpvalue_get_list_item(list_value, 8);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool drain;
					if (amqpvalue_get_boolean(item_value, &drain) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* echo */
				item_value = amqpvalue_get_list_item(list_value, 9);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool echo;
					if (amqpvalue_get_boolean(item_value, &echo) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* properties */
				item_value = amqpvalue_get_list_item(list_value, 10);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					fields properties;
					if (amqpvalue_get_fields(item_value, &properties) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							flow_destroy(*flow_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				flow_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int flow_get_next_incoming_id(FLOW_HANDLE flow, transfer_number* next_incoming_id_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_transfer_number(item_value, next_incoming_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_next_incoming_id(FLOW_HANDLE flow, transfer_number next_incoming_id_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE next_incoming_id_amqp_value = amqpvalue_create_transfer_number(next_incoming_id_value);
		if (next_incoming_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 0, next_incoming_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(next_incoming_id_amqp_value);
		}
	}

	return result;
}

int flow_get_incoming_window(FLOW_HANDLE flow, uint32_t* incoming_window_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, incoming_window_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_incoming_window(FLOW_HANDLE flow, uint32_t incoming_window_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE incoming_window_amqp_value = amqpvalue_create_uint(incoming_window_value);
		if (incoming_window_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 1, incoming_window_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(incoming_window_amqp_value);
		}
	}

	return result;
}

int flow_get_next_outgoing_id(FLOW_HANDLE flow, transfer_number* next_outgoing_id_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_transfer_number(item_value, next_outgoing_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_next_outgoing_id(FLOW_HANDLE flow, transfer_number next_outgoing_id_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE next_outgoing_id_amqp_value = amqpvalue_create_transfer_number(next_outgoing_id_value);
		if (next_outgoing_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 2, next_outgoing_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(next_outgoing_id_amqp_value);
		}
	}

	return result;
}

int flow_get_outgoing_window(FLOW_HANDLE flow, uint32_t* outgoing_window_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, outgoing_window_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_outgoing_window(FLOW_HANDLE flow, uint32_t outgoing_window_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE outgoing_window_amqp_value = amqpvalue_create_uint(outgoing_window_value);
		if (outgoing_window_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 3, outgoing_window_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(outgoing_window_amqp_value);
		}
	}

	return result;
}

int flow_get_handle(FLOW_HANDLE flow, handle* handle_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_handle(item_value, handle_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_handle(FLOW_HANDLE flow, handle handle_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE handle_amqp_value = amqpvalue_create_handle(handle_value);
		if (handle_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 4, handle_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(handle_amqp_value);
		}
	}

	return result;
}

int flow_get_delivery_count(FLOW_HANDLE flow, sequence_no* delivery_count_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_sequence_no(item_value, delivery_count_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_delivery_count(FLOW_HANDLE flow, sequence_no delivery_count_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE delivery_count_amqp_value = amqpvalue_create_sequence_no(delivery_count_value);
		if (delivery_count_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 5, delivery_count_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(delivery_count_amqp_value);
		}
	}

	return result;
}

int flow_get_link_credit(FLOW_HANDLE flow, uint32_t* link_credit_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, link_credit_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_link_credit(FLOW_HANDLE flow, uint32_t link_credit_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE link_credit_amqp_value = amqpvalue_create_uint(link_credit_value);
		if (link_credit_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 6, link_credit_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(link_credit_amqp_value);
		}
	}

	return result;
}

int flow_get_available(FLOW_HANDLE flow, uint32_t* available_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, available_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_available(FLOW_HANDLE flow, uint32_t available_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE available_amqp_value = amqpvalue_create_uint(available_value);
		if (available_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 7, available_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(available_amqp_value);
		}
	}

	return result;
}

int flow_get_drain(FLOW_HANDLE flow, bool* drain_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 8);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, drain_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_drain(FLOW_HANDLE flow, bool drain_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE drain_amqp_value = amqpvalue_create_boolean(drain_value);
		if (drain_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 8, drain_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(drain_amqp_value);
		}
	}

	return result;
}

int flow_get_echo(FLOW_HANDLE flow, bool* echo_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 9);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, echo_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_echo(FLOW_HANDLE flow, bool echo_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE echo_amqp_value = amqpvalue_create_boolean(echo_value);
		if (echo_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 9, echo_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(echo_amqp_value);
		}
	}

	return result;
}

int flow_get_properties(FLOW_HANDLE flow, fields* properties_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(flow_instance->composite_value, 10);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_fields(item_value, properties_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int flow_set_properties(FLOW_HANDLE flow, fields properties_value)
{
	int result;

	if (flow == NULL)
	{
		result = __LINE__;
	}
	else
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
		AMQP_VALUE properties_amqp_value = amqpvalue_create_fields(properties_value);
		if (properties_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(flow_instance->composite_value, 10, properties_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(properties_amqp_value);
		}
	}

	return result;
}


/* transfer */

typedef struct TRANSFER_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} TRANSFER_INSTANCE;

static TRANSFER_HANDLE transfer_create_internal(void)
{
	TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)amqpalloc_malloc(sizeof(TRANSFER_INSTANCE));
	if (transfer_instance != NULL)
	{
		transfer_instance->composite_value = NULL;
	}

	return transfer_instance;
}

TRANSFER_HANDLE transfer_create(handle handle_value)
{
	TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)amqpalloc_malloc(sizeof(TRANSFER_INSTANCE));
	if (transfer_instance != NULL)
	{
		transfer_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(20);
		if (transfer_instance->composite_value == NULL)
		{
			amqpalloc_free(transfer_instance);
			transfer_instance = NULL;
		}
		else
		{
			AMQP_VALUE handle_amqp_value;
			int result = 0;

			handle_amqp_value = amqpvalue_create_handle(handle_value);
			if ((result == 0) && (amqpvalue_set_composite_item(transfer_instance->composite_value, 0, handle_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(handle_amqp_value);
		}
	}

	return transfer_instance;
}

TRANSFER_HANDLE transfer_clone(TRANSFER_HANDLE value)
{
	TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)amqpalloc_malloc(sizeof(TRANSFER_INSTANCE));
	if (transfer_instance != NULL)
	{
		transfer_instance->composite_value = amqpvalue_clone(((TRANSFER_INSTANCE*)value)->composite_value);
		if (transfer_instance->composite_value == NULL)
		{
			amqpalloc_free(transfer_instance);
			transfer_instance = NULL;
		}
	}

	return transfer_instance;
}

void transfer_destroy(TRANSFER_HANDLE transfer)
{
	if (transfer != NULL)
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		amqpvalue_destroy(transfer_instance->composite_value);
		amqpalloc_free(transfer_instance);
	}
}

AMQP_VALUE amqpvalue_create_transfer(TRANSFER_HANDLE transfer)
{
	AMQP_VALUE result;

	if (transfer == NULL)
	{
		result = NULL;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		result = amqpvalue_clone(transfer_instance->composite_value);
	}

	return result;
}

bool is_transfer_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 20))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_transfer(AMQP_VALUE value, TRANSFER_HANDLE* transfer_handle)
{
	int result;
	TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer_create_internal();
	*transfer_handle = transfer_instance;
	if (*transfer_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			transfer_destroy(*transfer_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* handle */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						transfer_destroy(*transfer_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					handle handle;
					if (amqpvalue_get_handle(item_value, &handle) != 0)
					{
						transfer_destroy(*transfer_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* delivery-id */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					delivery_number delivery_id;
					if (amqpvalue_get_delivery_number(item_value, &delivery_id) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* delivery-tag */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					delivery_tag delivery_tag;
					if (amqpvalue_get_delivery_tag(item_value, &delivery_tag) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* message-format */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					message_format message_format;
					if (amqpvalue_get_message_format(item_value, &message_format) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* settled */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool settled;
					if (amqpvalue_get_boolean(item_value, &settled) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* more */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool more;
					if (amqpvalue_get_boolean(item_value, &more) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* rcv-settle-mode */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					receiver_settle_mode rcv_settle_mode;
					if (amqpvalue_get_receiver_settle_mode(item_value, &rcv_settle_mode) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* state */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* resume */
				item_value = amqpvalue_get_list_item(list_value, 8);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool resume;
					if (amqpvalue_get_boolean(item_value, &resume) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* aborted */
				item_value = amqpvalue_get_list_item(list_value, 9);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool aborted;
					if (amqpvalue_get_boolean(item_value, &aborted) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* batchable */
				item_value = amqpvalue_get_list_item(list_value, 10);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool batchable;
					if (amqpvalue_get_boolean(item_value, &batchable) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							transfer_destroy(*transfer_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				transfer_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int transfer_get_handle(TRANSFER_HANDLE transfer, handle* handle_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_handle(item_value, handle_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_handle(TRANSFER_HANDLE transfer, handle handle_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE handle_amqp_value = amqpvalue_create_handle(handle_value);
		if (handle_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 0, handle_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(handle_amqp_value);
		}
	}

	return result;
}

int transfer_get_delivery_id(TRANSFER_HANDLE transfer, delivery_number* delivery_id_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_delivery_number(item_value, delivery_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_delivery_id(TRANSFER_HANDLE transfer, delivery_number delivery_id_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE delivery_id_amqp_value = amqpvalue_create_delivery_number(delivery_id_value);
		if (delivery_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 1, delivery_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(delivery_id_amqp_value);
		}
	}

	return result;
}

int transfer_get_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag* delivery_tag_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_delivery_tag(item_value, delivery_tag_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag delivery_tag_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE delivery_tag_amqp_value = amqpvalue_create_delivery_tag(delivery_tag_value);
		if (delivery_tag_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 2, delivery_tag_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(delivery_tag_amqp_value);
		}
	}

	return result;
}

int transfer_get_message_format(TRANSFER_HANDLE transfer, message_format* message_format_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_message_format(item_value, message_format_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_message_format(TRANSFER_HANDLE transfer, message_format message_format_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE message_format_amqp_value = amqpvalue_create_message_format(message_format_value);
		if (message_format_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 3, message_format_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(message_format_amqp_value);
		}
	}

	return result;
}

int transfer_get_settled(TRANSFER_HANDLE transfer, bool* settled_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, settled_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_settled(TRANSFER_HANDLE transfer, bool settled_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE settled_amqp_value = amqpvalue_create_boolean(settled_value);
		if (settled_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 4, settled_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(settled_amqp_value);
		}
	}

	return result;
}

int transfer_get_more(TRANSFER_HANDLE transfer, bool* more_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, more_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_more(TRANSFER_HANDLE transfer, bool more_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE more_amqp_value = amqpvalue_create_boolean(more_value);
		if (more_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 5, more_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(more_amqp_value);
		}
	}

	return result;
}

int transfer_get_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode* rcv_settle_mode_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_receiver_settle_mode(item_value, rcv_settle_mode_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode rcv_settle_mode_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE rcv_settle_mode_amqp_value = amqpvalue_create_receiver_settle_mode(rcv_settle_mode_value);
		if (rcv_settle_mode_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 6, rcv_settle_mode_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(rcv_settle_mode_amqp_value);
		}
	}

	return result;
}

int transfer_get_state(TRANSFER_HANDLE transfer, AMQP_VALUE* state_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*state_value = item_value;
			result = 0;
		}
	}

	return result;
}

int transfer_set_state(TRANSFER_HANDLE transfer, AMQP_VALUE state_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE state_amqp_value = amqpvalue_clone(state_value);
		if (state_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 7, state_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(state_amqp_value);
		}
	}

	return result;
}

int transfer_get_resume(TRANSFER_HANDLE transfer, bool* resume_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 8);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, resume_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_resume(TRANSFER_HANDLE transfer, bool resume_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE resume_amqp_value = amqpvalue_create_boolean(resume_value);
		if (resume_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 8, resume_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(resume_amqp_value);
		}
	}

	return result;
}

int transfer_get_aborted(TRANSFER_HANDLE transfer, bool* aborted_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 9);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, aborted_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_aborted(TRANSFER_HANDLE transfer, bool aborted_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE aborted_amqp_value = amqpvalue_create_boolean(aborted_value);
		if (aborted_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 9, aborted_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(aborted_amqp_value);
		}
	}

	return result;
}

int transfer_get_batchable(TRANSFER_HANDLE transfer, bool* batchable_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(transfer_instance->composite_value, 10);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, batchable_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int transfer_set_batchable(TRANSFER_HANDLE transfer, bool batchable_value)
{
	int result;

	if (transfer == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
		AMQP_VALUE batchable_amqp_value = amqpvalue_create_boolean(batchable_value);
		if (batchable_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(transfer_instance->composite_value, 10, batchable_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(batchable_amqp_value);
		}
	}

	return result;
}


/* disposition */

typedef struct DISPOSITION_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} DISPOSITION_INSTANCE;

static DISPOSITION_HANDLE disposition_create_internal(void)
{
	DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)amqpalloc_malloc(sizeof(DISPOSITION_INSTANCE));
	if (disposition_instance != NULL)
	{
		disposition_instance->composite_value = NULL;
	}

	return disposition_instance;
}

DISPOSITION_HANDLE disposition_create(role role_value, delivery_number first_value)
{
	DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)amqpalloc_malloc(sizeof(DISPOSITION_INSTANCE));
	if (disposition_instance != NULL)
	{
		disposition_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(21);
		if (disposition_instance->composite_value == NULL)
		{
			amqpalloc_free(disposition_instance);
			disposition_instance = NULL;
		}
		else
		{
			AMQP_VALUE role_amqp_value;
			AMQP_VALUE first_amqp_value;
			int result = 0;

			role_amqp_value = amqpvalue_create_role(role_value);
			if ((result == 0) && (amqpvalue_set_composite_item(disposition_instance->composite_value, 0, role_amqp_value) != 0))
			{
				result = __LINE__;
			}
			first_amqp_value = amqpvalue_create_delivery_number(first_value);
			if ((result == 0) && (amqpvalue_set_composite_item(disposition_instance->composite_value, 1, first_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(role_amqp_value);
			amqpvalue_destroy(first_amqp_value);
		}
	}

	return disposition_instance;
}

DISPOSITION_HANDLE disposition_clone(DISPOSITION_HANDLE value)
{
	DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)amqpalloc_malloc(sizeof(DISPOSITION_INSTANCE));
	if (disposition_instance != NULL)
	{
		disposition_instance->composite_value = amqpvalue_clone(((DISPOSITION_INSTANCE*)value)->composite_value);
		if (disposition_instance->composite_value == NULL)
		{
			amqpalloc_free(disposition_instance);
			disposition_instance = NULL;
		}
	}

	return disposition_instance;
}

void disposition_destroy(DISPOSITION_HANDLE disposition)
{
	if (disposition != NULL)
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		amqpvalue_destroy(disposition_instance->composite_value);
		amqpalloc_free(disposition_instance);
	}
}

AMQP_VALUE amqpvalue_create_disposition(DISPOSITION_HANDLE disposition)
{
	AMQP_VALUE result;

	if (disposition == NULL)
	{
		result = NULL;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		result = amqpvalue_clone(disposition_instance->composite_value);
	}

	return result;
}

bool is_disposition_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 21))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_disposition(AMQP_VALUE value, DISPOSITION_HANDLE* disposition_handle)
{
	int result;
	DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition_create_internal();
	*disposition_handle = disposition_instance;
	if (*disposition_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			disposition_destroy(*disposition_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* role */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						disposition_destroy(*disposition_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					role role;
					if (amqpvalue_get_role(item_value, &role) != 0)
					{
						disposition_destroy(*disposition_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* first */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					{
						disposition_destroy(*disposition_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					delivery_number first;
					if (amqpvalue_get_delivery_number(item_value, &first) != 0)
					{
						disposition_destroy(*disposition_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* last */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					delivery_number last;
					if (amqpvalue_get_delivery_number(item_value, &last) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							disposition_destroy(*disposition_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* settled */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool settled;
					if (amqpvalue_get_boolean(item_value, &settled) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							disposition_destroy(*disposition_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* state */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* batchable */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool batchable;
					if (amqpvalue_get_boolean(item_value, &batchable) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							disposition_destroy(*disposition_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				disposition_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int disposition_get_role(DISPOSITION_HANDLE disposition, role* role_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(disposition_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_role(item_value, role_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int disposition_set_role(DISPOSITION_HANDLE disposition, role role_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE role_amqp_value = amqpvalue_create_role(role_value);
		if (role_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(disposition_instance->composite_value, 0, role_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(role_amqp_value);
		}
	}

	return result;
}

int disposition_get_first(DISPOSITION_HANDLE disposition, delivery_number* first_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(disposition_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_delivery_number(item_value, first_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int disposition_set_first(DISPOSITION_HANDLE disposition, delivery_number first_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE first_amqp_value = amqpvalue_create_delivery_number(first_value);
		if (first_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(disposition_instance->composite_value, 1, first_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(first_amqp_value);
		}
	}

	return result;
}

int disposition_get_last(DISPOSITION_HANDLE disposition, delivery_number* last_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(disposition_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_delivery_number(item_value, last_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int disposition_set_last(DISPOSITION_HANDLE disposition, delivery_number last_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE last_amqp_value = amqpvalue_create_delivery_number(last_value);
		if (last_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(disposition_instance->composite_value, 2, last_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(last_amqp_value);
		}
	}

	return result;
}

int disposition_get_settled(DISPOSITION_HANDLE disposition, bool* settled_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(disposition_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, settled_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int disposition_set_settled(DISPOSITION_HANDLE disposition, bool settled_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE settled_amqp_value = amqpvalue_create_boolean(settled_value);
		if (settled_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(disposition_instance->composite_value, 3, settled_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(settled_amqp_value);
		}
	}

	return result;
}

int disposition_get_state(DISPOSITION_HANDLE disposition, AMQP_VALUE* state_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(disposition_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*state_value = item_value;
			result = 0;
		}
	}

	return result;
}

int disposition_set_state(DISPOSITION_HANDLE disposition, AMQP_VALUE state_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE state_amqp_value = amqpvalue_clone(state_value);
		if (state_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(disposition_instance->composite_value, 4, state_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(state_amqp_value);
		}
	}

	return result;
}

int disposition_get_batchable(DISPOSITION_HANDLE disposition, bool* batchable_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(disposition_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, batchable_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int disposition_set_batchable(DISPOSITION_HANDLE disposition, bool batchable_value)
{
	int result;

	if (disposition == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
		AMQP_VALUE batchable_amqp_value = amqpvalue_create_boolean(batchable_value);
		if (batchable_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(disposition_instance->composite_value, 5, batchable_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(batchable_amqp_value);
		}
	}

	return result;
}


/* detach */

typedef struct DETACH_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} DETACH_INSTANCE;

static DETACH_HANDLE detach_create_internal(void)
{
	DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)amqpalloc_malloc(sizeof(DETACH_INSTANCE));
	if (detach_instance != NULL)
	{
		detach_instance->composite_value = NULL;
	}

	return detach_instance;
}

DETACH_HANDLE detach_create(handle handle_value)
{
	DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)amqpalloc_malloc(sizeof(DETACH_INSTANCE));
	if (detach_instance != NULL)
	{
		detach_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(22);
		if (detach_instance->composite_value == NULL)
		{
			amqpalloc_free(detach_instance);
			detach_instance = NULL;
		}
		else
		{
			AMQP_VALUE handle_amqp_value;
			int result = 0;

			handle_amqp_value = amqpvalue_create_handle(handle_value);
			if ((result == 0) && (amqpvalue_set_composite_item(detach_instance->composite_value, 0, handle_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(handle_amqp_value);
		}
	}

	return detach_instance;
}

DETACH_HANDLE detach_clone(DETACH_HANDLE value)
{
	DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)amqpalloc_malloc(sizeof(DETACH_INSTANCE));
	if (detach_instance != NULL)
	{
		detach_instance->composite_value = amqpvalue_clone(((DETACH_INSTANCE*)value)->composite_value);
		if (detach_instance->composite_value == NULL)
		{
			amqpalloc_free(detach_instance);
			detach_instance = NULL;
		}
	}

	return detach_instance;
}

void detach_destroy(DETACH_HANDLE detach)
{
	if (detach != NULL)
	{
		DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		amqpvalue_destroy(detach_instance->composite_value);
		amqpalloc_free(detach_instance);
	}
}

AMQP_VALUE amqpvalue_create_detach(DETACH_HANDLE detach)
{
	AMQP_VALUE result;

	if (detach == NULL)
	{
		result = NULL;
	}
	else
	{
		DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		result = amqpvalue_clone(detach_instance->composite_value);
	}

	return result;
}

bool is_detach_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 22))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_detach(AMQP_VALUE value, DETACH_HANDLE* detach_handle)
{
	int result;
	DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach_create_internal();
	*detach_handle = detach_instance;
	if (*detach_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			detach_destroy(*detach_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* handle */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						detach_destroy(*detach_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					handle handle;
					if (amqpvalue_get_handle(item_value, &handle) != 0)
					{
						detach_destroy(*detach_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* closed */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool closed;
					if (amqpvalue_get_boolean(item_value, &closed) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							detach_destroy(*detach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* error */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					ERROR_HANDLE error;
					if (amqpvalue_get_error(item_value, &error) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							detach_destroy(*detach_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				detach_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int detach_get_handle(DETACH_HANDLE detach, handle* handle_value)
{
	int result;

	if (detach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(detach_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_handle(item_value, handle_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int detach_set_handle(DETACH_HANDLE detach, handle handle_value)
{
	int result;

	if (detach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		AMQP_VALUE handle_amqp_value = amqpvalue_create_handle(handle_value);
		if (handle_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(detach_instance->composite_value, 0, handle_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(handle_amqp_value);
		}
	}

	return result;
}

int detach_get_closed(DETACH_HANDLE detach, bool* closed_value)
{
	int result;

	if (detach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(detach_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, closed_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int detach_set_closed(DETACH_HANDLE detach, bool closed_value)
{
	int result;

	if (detach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		AMQP_VALUE closed_amqp_value = amqpvalue_create_boolean(closed_value);
		if (closed_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(detach_instance->composite_value, 1, closed_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(closed_amqp_value);
		}
	}

	return result;
}

int detach_get_error(DETACH_HANDLE detach, ERROR_HANDLE* error_value)
{
	int result;

	if (detach == NULL)
	{
		result = __LINE__;
	}
	else
	{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(detach_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_error(item_value, error_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int detach_set_error(DETACH_HANDLE detach, ERROR_HANDLE error_value)
{
	int result;

	if (detach == NULL)
	{
		result = __LINE__;
	}
	else
	{
		DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
		AMQP_VALUE error_amqp_value = amqpvalue_create_error(error_value);
		if (error_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(detach_instance->composite_value, 2, error_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(error_amqp_value);
		}
	}

	return result;
}


/* end */

typedef struct END_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} END_INSTANCE;

static END_HANDLE end_create_internal(void)
{
	END_INSTANCE* end_instance = (END_INSTANCE*)amqpalloc_malloc(sizeof(END_INSTANCE));
	if (end_instance != NULL)
	{
		end_instance->composite_value = NULL;
	}

	return end_instance;
}

END_HANDLE end_create(void)
{
	END_INSTANCE* end_instance = (END_INSTANCE*)amqpalloc_malloc(sizeof(END_INSTANCE));
	if (end_instance != NULL)
	{
		end_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(23);
		if (end_instance->composite_value == NULL)
		{
			amqpalloc_free(end_instance);
			end_instance = NULL;
		}
	}

	return end_instance;
}

END_HANDLE end_clone(END_HANDLE value)
{
	END_INSTANCE* end_instance = (END_INSTANCE*)amqpalloc_malloc(sizeof(END_INSTANCE));
	if (end_instance != NULL)
	{
		end_instance->composite_value = amqpvalue_clone(((END_INSTANCE*)value)->composite_value);
		if (end_instance->composite_value == NULL)
		{
			amqpalloc_free(end_instance);
			end_instance = NULL;
		}
	}

	return end_instance;
}

void end_destroy(END_HANDLE end)
{
	if (end != NULL)
	{
		END_INSTANCE* end_instance = (END_INSTANCE*)end;
		amqpvalue_destroy(end_instance->composite_value);
		amqpalloc_free(end_instance);
	}
}

AMQP_VALUE amqpvalue_create_end(END_HANDLE end)
{
	AMQP_VALUE result;

	if (end == NULL)
	{
		result = NULL;
	}
	else
	{
		END_INSTANCE* end_instance = (END_INSTANCE*)end;
		result = amqpvalue_clone(end_instance->composite_value);
	}

	return result;
}

bool is_end_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 23))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_end(AMQP_VALUE value, END_HANDLE* end_handle)
{
	int result;
	END_INSTANCE* end_instance = (END_INSTANCE*)end_create_internal();
	*end_handle = end_instance;
	if (*end_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			end_destroy(*end_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* error */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					ERROR_HANDLE error;
					if (amqpvalue_get_error(item_value, &error) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							end_destroy(*end_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				end_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int end_get_error(END_HANDLE end, ERROR_HANDLE* error_value)
{
	int result;

	if (end == NULL)
	{
		result = __LINE__;
	}
	else
	{
			END_INSTANCE* end_instance = (END_INSTANCE*)end;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(end_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_error(item_value, error_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int end_set_error(END_HANDLE end, ERROR_HANDLE error_value)
{
	int result;

	if (end == NULL)
	{
		result = __LINE__;
	}
	else
	{
		END_INSTANCE* end_instance = (END_INSTANCE*)end;
		AMQP_VALUE error_amqp_value = amqpvalue_create_error(error_value);
		if (error_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(end_instance->composite_value, 0, error_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(error_amqp_value);
		}
	}

	return result;
}


/* close */

typedef struct CLOSE_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} CLOSE_INSTANCE;

static CLOSE_HANDLE close_create_internal(void)
{
	CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)amqpalloc_malloc(sizeof(CLOSE_INSTANCE));
	if (close_instance != NULL)
	{
		close_instance->composite_value = NULL;
	}

	return close_instance;
}

CLOSE_HANDLE close_create(void)
{
	CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)amqpalloc_malloc(sizeof(CLOSE_INSTANCE));
	if (close_instance != NULL)
	{
		close_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(24);
		if (close_instance->composite_value == NULL)
		{
			amqpalloc_free(close_instance);
			close_instance = NULL;
		}
	}

	return close_instance;
}

CLOSE_HANDLE close_clone(CLOSE_HANDLE value)
{
	CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)amqpalloc_malloc(sizeof(CLOSE_INSTANCE));
	if (close_instance != NULL)
	{
		close_instance->composite_value = amqpvalue_clone(((CLOSE_INSTANCE*)value)->composite_value);
		if (close_instance->composite_value == NULL)
		{
			amqpalloc_free(close_instance);
			close_instance = NULL;
		}
	}

	return close_instance;
}

void close_destroy(CLOSE_HANDLE close)
{
	if (close != NULL)
	{
		CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close;
		amqpvalue_destroy(close_instance->composite_value);
		amqpalloc_free(close_instance);
	}
}

AMQP_VALUE amqpvalue_create_close(CLOSE_HANDLE close)
{
	AMQP_VALUE result;

	if (close == NULL)
	{
		result = NULL;
	}
	else
	{
		CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close;
		result = amqpvalue_clone(close_instance->composite_value);
	}

	return result;
}

bool is_close_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 24))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_close(AMQP_VALUE value, CLOSE_HANDLE* close_handle)
{
	int result;
	CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close_create_internal();
	*close_handle = close_instance;
	if (*close_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			close_destroy(*close_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* error */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					ERROR_HANDLE error;
					if (amqpvalue_get_error(item_value, &error) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							close_destroy(*close_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				close_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int close_get_error(CLOSE_HANDLE close, ERROR_HANDLE* error_value)
{
	int result;

	if (close == NULL)
	{
		result = __LINE__;
	}
	else
	{
			CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(close_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_error(item_value, error_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int close_set_error(CLOSE_HANDLE close, ERROR_HANDLE error_value)
{
	int result;

	if (close == NULL)
	{
		result = __LINE__;
	}
	else
	{
		CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close;
		AMQP_VALUE error_amqp_value = amqpvalue_create_error(error_value);
		if (error_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(close_instance->composite_value, 0, error_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(error_amqp_value);
		}
	}

	return result;
}


/* sasl-code */

AMQP_VALUE amqpvalue_create_sasl_code(sasl_code value)
{
	return amqpvalue_create_ubyte(value);
}

/* sasl-mechanisms */

typedef struct SASL_MECHANISMS_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} SASL_MECHANISMS_INSTANCE;

static SASL_MECHANISMS_HANDLE sasl_mechanisms_create_internal(void)
{
	SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)amqpalloc_malloc(sizeof(SASL_MECHANISMS_INSTANCE));
	if (sasl_mechanisms_instance != NULL)
	{
		sasl_mechanisms_instance->composite_value = NULL;
	}

	return sasl_mechanisms_instance;
}

SASL_MECHANISMS_HANDLE sasl_mechanisms_create(const char* sasl_server_mechanisms_value)
{
	SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)amqpalloc_malloc(sizeof(SASL_MECHANISMS_INSTANCE));
	if (sasl_mechanisms_instance != NULL)
	{
		sasl_mechanisms_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(64);
		if (sasl_mechanisms_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_mechanisms_instance);
			sasl_mechanisms_instance = NULL;
		}
		else
		{
			AMQP_VALUE sasl_server_mechanisms_amqp_value;
			int result = 0;

			sasl_server_mechanisms_amqp_value = amqpvalue_create_symbol(sasl_server_mechanisms_value);
			if ((result == 0) && (amqpvalue_set_composite_item(sasl_mechanisms_instance->composite_value, 0, sasl_server_mechanisms_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(sasl_server_mechanisms_amqp_value);
		}
	}

	return sasl_mechanisms_instance;
}

SASL_MECHANISMS_HANDLE sasl_mechanisms_clone(SASL_MECHANISMS_HANDLE value)
{
	SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)amqpalloc_malloc(sizeof(SASL_MECHANISMS_INSTANCE));
	if (sasl_mechanisms_instance != NULL)
	{
		sasl_mechanisms_instance->composite_value = amqpvalue_clone(((SASL_MECHANISMS_INSTANCE*)value)->composite_value);
		if (sasl_mechanisms_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_mechanisms_instance);
			sasl_mechanisms_instance = NULL;
		}
	}

	return sasl_mechanisms_instance;
}

void sasl_mechanisms_destroy(SASL_MECHANISMS_HANDLE sasl_mechanisms)
{
	if (sasl_mechanisms != NULL)
	{
		SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)sasl_mechanisms;
		amqpvalue_destroy(sasl_mechanisms_instance->composite_value);
		amqpalloc_free(sasl_mechanisms_instance);
	}
}

AMQP_VALUE amqpvalue_create_sasl_mechanisms(SASL_MECHANISMS_HANDLE sasl_mechanisms)
{
	AMQP_VALUE result;

	if (sasl_mechanisms == NULL)
	{
		result = NULL;
	}
	else
	{
		SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)sasl_mechanisms;
		result = amqpvalue_clone(sasl_mechanisms_instance->composite_value);
	}

	return result;
}

bool is_sasl_mechanisms_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 64))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_sasl_mechanisms(AMQP_VALUE value, SASL_MECHANISMS_HANDLE* sasl_mechanisms_handle)
{
	int result;
	SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)sasl_mechanisms_create_internal();
	*sasl_mechanisms_handle = sasl_mechanisms_instance;
	if (*sasl_mechanisms_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			sasl_mechanisms_destroy(*sasl_mechanisms_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* sasl-server-mechanisms */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						sasl_mechanisms_destroy(*sasl_mechanisms_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					const char* sasl_server_mechanisms;
					if (amqpvalue_get_symbol(item_value, &sasl_server_mechanisms) != 0)
					{
						sasl_mechanisms_destroy(*sasl_mechanisms_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}

				sasl_mechanisms_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int sasl_mechanisms_get_sasl_server_mechanisms(SASL_MECHANISMS_HANDLE sasl_mechanisms, const char** sasl_server_mechanisms_value)
{
	int result;

	if (sasl_mechanisms == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)sasl_mechanisms;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_mechanisms_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, sasl_server_mechanisms_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_mechanisms_set_sasl_server_mechanisms(SASL_MECHANISMS_HANDLE sasl_mechanisms, const char* sasl_server_mechanisms_value)
{
	int result;

	if (sasl_mechanisms == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_MECHANISMS_INSTANCE* sasl_mechanisms_instance = (SASL_MECHANISMS_INSTANCE*)sasl_mechanisms;
		AMQP_VALUE sasl_server_mechanisms_amqp_value = amqpvalue_create_symbol(sasl_server_mechanisms_value);
		if (sasl_server_mechanisms_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_mechanisms_instance->composite_value, 0, sasl_server_mechanisms_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(sasl_server_mechanisms_amqp_value);
		}
	}

	return result;
}


/* sasl-init */

typedef struct SASL_INIT_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} SASL_INIT_INSTANCE;

static SASL_INIT_HANDLE sasl_init_create_internal(void)
{
	SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)amqpalloc_malloc(sizeof(SASL_INIT_INSTANCE));
	if (sasl_init_instance != NULL)
	{
		sasl_init_instance->composite_value = NULL;
	}

	return sasl_init_instance;
}

SASL_INIT_HANDLE sasl_init_create(const char* mechanism_value)
{
	SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)amqpalloc_malloc(sizeof(SASL_INIT_INSTANCE));
	if (sasl_init_instance != NULL)
	{
		sasl_init_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(65);
		if (sasl_init_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_init_instance);
			sasl_init_instance = NULL;
		}
		else
		{
			AMQP_VALUE mechanism_amqp_value;
			int result = 0;

			mechanism_amqp_value = amqpvalue_create_symbol(mechanism_value);
			if ((result == 0) && (amqpvalue_set_composite_item(sasl_init_instance->composite_value, 0, mechanism_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(mechanism_amqp_value);
		}
	}

	return sasl_init_instance;
}

SASL_INIT_HANDLE sasl_init_clone(SASL_INIT_HANDLE value)
{
	SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)amqpalloc_malloc(sizeof(SASL_INIT_INSTANCE));
	if (sasl_init_instance != NULL)
	{
		sasl_init_instance->composite_value = amqpvalue_clone(((SASL_INIT_INSTANCE*)value)->composite_value);
		if (sasl_init_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_init_instance);
			sasl_init_instance = NULL;
		}
	}

	return sasl_init_instance;
}

void sasl_init_destroy(SASL_INIT_HANDLE sasl_init)
{
	if (sasl_init != NULL)
	{
		SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		amqpvalue_destroy(sasl_init_instance->composite_value);
		amqpalloc_free(sasl_init_instance);
	}
}

AMQP_VALUE amqpvalue_create_sasl_init(SASL_INIT_HANDLE sasl_init)
{
	AMQP_VALUE result;

	if (sasl_init == NULL)
	{
		result = NULL;
	}
	else
	{
		SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		result = amqpvalue_clone(sasl_init_instance->composite_value);
	}

	return result;
}

bool is_sasl_init_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 65))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_sasl_init(AMQP_VALUE value, SASL_INIT_HANDLE* sasl_init_handle)
{
	int result;
	SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init_create_internal();
	*sasl_init_handle = sasl_init_instance;
	if (*sasl_init_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			sasl_init_destroy(*sasl_init_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* mechanism */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						sasl_init_destroy(*sasl_init_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					const char* mechanism;
					if (amqpvalue_get_symbol(item_value, &mechanism) != 0)
					{
						sasl_init_destroy(*sasl_init_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* initial-response */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqp_binary initial_response;
					if (amqpvalue_get_binary(item_value, &initial_response) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							sasl_init_destroy(*sasl_init_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* hostname */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* hostname;
					if (amqpvalue_get_string(item_value, &hostname) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							sasl_init_destroy(*sasl_init_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				sasl_init_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int sasl_init_get_mechanism(SASL_INIT_HANDLE sasl_init, const char** mechanism_value)
{
	int result;

	if (sasl_init == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_init_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, mechanism_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_init_set_mechanism(SASL_INIT_HANDLE sasl_init, const char* mechanism_value)
{
	int result;

	if (sasl_init == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		AMQP_VALUE mechanism_amqp_value = amqpvalue_create_symbol(mechanism_value);
		if (mechanism_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_init_instance->composite_value, 0, mechanism_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(mechanism_amqp_value);
		}
	}

	return result;
}

int sasl_init_get_initial_response(SASL_INIT_HANDLE sasl_init, amqp_binary* initial_response_value)
{
	int result;

	if (sasl_init == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_init_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_binary(item_value, initial_response_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_init_set_initial_response(SASL_INIT_HANDLE sasl_init, amqp_binary initial_response_value)
{
	int result;

	if (sasl_init == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		AMQP_VALUE initial_response_amqp_value = amqpvalue_create_binary(initial_response_value);
		if (initial_response_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_init_instance->composite_value, 1, initial_response_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(initial_response_amqp_value);
		}
	}

	return result;
}

int sasl_init_get_hostname(SASL_INIT_HANDLE sasl_init, const char** hostname_value)
{
	int result;

	if (sasl_init == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_init_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, hostname_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_init_set_hostname(SASL_INIT_HANDLE sasl_init, const char* hostname_value)
{
	int result;

	if (sasl_init == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_INIT_INSTANCE* sasl_init_instance = (SASL_INIT_INSTANCE*)sasl_init;
		AMQP_VALUE hostname_amqp_value = amqpvalue_create_string(hostname_value);
		if (hostname_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_init_instance->composite_value, 2, hostname_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(hostname_amqp_value);
		}
	}

	return result;
}


/* sasl-challenge */

typedef struct SASL_CHALLENGE_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} SASL_CHALLENGE_INSTANCE;

static SASL_CHALLENGE_HANDLE sasl_challenge_create_internal(void)
{
	SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)amqpalloc_malloc(sizeof(SASL_CHALLENGE_INSTANCE));
	if (sasl_challenge_instance != NULL)
	{
		sasl_challenge_instance->composite_value = NULL;
	}

	return sasl_challenge_instance;
}

SASL_CHALLENGE_HANDLE sasl_challenge_create(amqp_binary challenge_value)
{
	SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)amqpalloc_malloc(sizeof(SASL_CHALLENGE_INSTANCE));
	if (sasl_challenge_instance != NULL)
	{
		sasl_challenge_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(66);
		if (sasl_challenge_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_challenge_instance);
			sasl_challenge_instance = NULL;
		}
		else
		{
			AMQP_VALUE challenge_amqp_value;
			int result = 0;

			challenge_amqp_value = amqpvalue_create_binary(challenge_value);
			if ((result == 0) && (amqpvalue_set_composite_item(sasl_challenge_instance->composite_value, 0, challenge_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(challenge_amqp_value);
		}
	}

	return sasl_challenge_instance;
}

SASL_CHALLENGE_HANDLE sasl_challenge_clone(SASL_CHALLENGE_HANDLE value)
{
	SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)amqpalloc_malloc(sizeof(SASL_CHALLENGE_INSTANCE));
	if (sasl_challenge_instance != NULL)
	{
		sasl_challenge_instance->composite_value = amqpvalue_clone(((SASL_CHALLENGE_INSTANCE*)value)->composite_value);
		if (sasl_challenge_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_challenge_instance);
			sasl_challenge_instance = NULL;
		}
	}

	return sasl_challenge_instance;
}

void sasl_challenge_destroy(SASL_CHALLENGE_HANDLE sasl_challenge)
{
	if (sasl_challenge != NULL)
	{
		SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)sasl_challenge;
		amqpvalue_destroy(sasl_challenge_instance->composite_value);
		amqpalloc_free(sasl_challenge_instance);
	}
}

AMQP_VALUE amqpvalue_create_sasl_challenge(SASL_CHALLENGE_HANDLE sasl_challenge)
{
	AMQP_VALUE result;

	if (sasl_challenge == NULL)
	{
		result = NULL;
	}
	else
	{
		SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)sasl_challenge;
		result = amqpvalue_clone(sasl_challenge_instance->composite_value);
	}

	return result;
}

bool is_sasl_challenge_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 66))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_sasl_challenge(AMQP_VALUE value, SASL_CHALLENGE_HANDLE* sasl_challenge_handle)
{
	int result;
	SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)sasl_challenge_create_internal();
	*sasl_challenge_handle = sasl_challenge_instance;
	if (*sasl_challenge_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			sasl_challenge_destroy(*sasl_challenge_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* challenge */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						sasl_challenge_destroy(*sasl_challenge_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					amqp_binary challenge;
					if (amqpvalue_get_binary(item_value, &challenge) != 0)
					{
						sasl_challenge_destroy(*sasl_challenge_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}

				sasl_challenge_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int sasl_challenge_get_challenge(SASL_CHALLENGE_HANDLE sasl_challenge, amqp_binary* challenge_value)
{
	int result;

	if (sasl_challenge == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)sasl_challenge;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_challenge_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_binary(item_value, challenge_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_challenge_set_challenge(SASL_CHALLENGE_HANDLE sasl_challenge, amqp_binary challenge_value)
{
	int result;

	if (sasl_challenge == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_CHALLENGE_INSTANCE* sasl_challenge_instance = (SASL_CHALLENGE_INSTANCE*)sasl_challenge;
		AMQP_VALUE challenge_amqp_value = amqpvalue_create_binary(challenge_value);
		if (challenge_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_challenge_instance->composite_value, 0, challenge_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(challenge_amqp_value);
		}
	}

	return result;
}


/* sasl-response */

typedef struct SASL_RESPONSE_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} SASL_RESPONSE_INSTANCE;

static SASL_RESPONSE_HANDLE sasl_response_create_internal(void)
{
	SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)amqpalloc_malloc(sizeof(SASL_RESPONSE_INSTANCE));
	if (sasl_response_instance != NULL)
	{
		sasl_response_instance->composite_value = NULL;
	}

	return sasl_response_instance;
}

SASL_RESPONSE_HANDLE sasl_response_create(amqp_binary response_value)
{
	SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)amqpalloc_malloc(sizeof(SASL_RESPONSE_INSTANCE));
	if (sasl_response_instance != NULL)
	{
		sasl_response_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(67);
		if (sasl_response_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_response_instance);
			sasl_response_instance = NULL;
		}
		else
		{
			AMQP_VALUE response_amqp_value;
			int result = 0;

			response_amqp_value = amqpvalue_create_binary(response_value);
			if ((result == 0) && (amqpvalue_set_composite_item(sasl_response_instance->composite_value, 0, response_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(response_amqp_value);
		}
	}

	return sasl_response_instance;
}

SASL_RESPONSE_HANDLE sasl_response_clone(SASL_RESPONSE_HANDLE value)
{
	SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)amqpalloc_malloc(sizeof(SASL_RESPONSE_INSTANCE));
	if (sasl_response_instance != NULL)
	{
		sasl_response_instance->composite_value = amqpvalue_clone(((SASL_RESPONSE_INSTANCE*)value)->composite_value);
		if (sasl_response_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_response_instance);
			sasl_response_instance = NULL;
		}
	}

	return sasl_response_instance;
}

void sasl_response_destroy(SASL_RESPONSE_HANDLE sasl_response)
{
	if (sasl_response != NULL)
	{
		SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)sasl_response;
		amqpvalue_destroy(sasl_response_instance->composite_value);
		amqpalloc_free(sasl_response_instance);
	}
}

AMQP_VALUE amqpvalue_create_sasl_response(SASL_RESPONSE_HANDLE sasl_response)
{
	AMQP_VALUE result;

	if (sasl_response == NULL)
	{
		result = NULL;
	}
	else
	{
		SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)sasl_response;
		result = amqpvalue_clone(sasl_response_instance->composite_value);
	}

	return result;
}

bool is_sasl_response_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 67))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_sasl_response(AMQP_VALUE value, SASL_RESPONSE_HANDLE* sasl_response_handle)
{
	int result;
	SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)sasl_response_create_internal();
	*sasl_response_handle = sasl_response_instance;
	if (*sasl_response_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			sasl_response_destroy(*sasl_response_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* response */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						sasl_response_destroy(*sasl_response_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					amqp_binary response;
					if (amqpvalue_get_binary(item_value, &response) != 0)
					{
						sasl_response_destroy(*sasl_response_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}

				sasl_response_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int sasl_response_get_response(SASL_RESPONSE_HANDLE sasl_response, amqp_binary* response_value)
{
	int result;

	if (sasl_response == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)sasl_response;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_response_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_binary(item_value, response_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_response_set_response(SASL_RESPONSE_HANDLE sasl_response, amqp_binary response_value)
{
	int result;

	if (sasl_response == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_RESPONSE_INSTANCE* sasl_response_instance = (SASL_RESPONSE_INSTANCE*)sasl_response;
		AMQP_VALUE response_amqp_value = amqpvalue_create_binary(response_value);
		if (response_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_response_instance->composite_value, 0, response_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(response_amqp_value);
		}
	}

	return result;
}


/* sasl-outcome */

typedef struct SASL_OUTCOME_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} SASL_OUTCOME_INSTANCE;

static SASL_OUTCOME_HANDLE sasl_outcome_create_internal(void)
{
	SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)amqpalloc_malloc(sizeof(SASL_OUTCOME_INSTANCE));
	if (sasl_outcome_instance != NULL)
	{
		sasl_outcome_instance->composite_value = NULL;
	}

	return sasl_outcome_instance;
}

SASL_OUTCOME_HANDLE sasl_outcome_create(sasl_code code_value)
{
	SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)amqpalloc_malloc(sizeof(SASL_OUTCOME_INSTANCE));
	if (sasl_outcome_instance != NULL)
	{
		sasl_outcome_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(68);
		if (sasl_outcome_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_outcome_instance);
			sasl_outcome_instance = NULL;
		}
		else
		{
			AMQP_VALUE code_amqp_value;
			int result = 0;

			code_amqp_value = amqpvalue_create_sasl_code(code_value);
			if ((result == 0) && (amqpvalue_set_composite_item(sasl_outcome_instance->composite_value, 0, code_amqp_value) != 0))
			{
				result = __LINE__;
			}

			amqpvalue_destroy(code_amqp_value);
		}
	}

	return sasl_outcome_instance;
}

SASL_OUTCOME_HANDLE sasl_outcome_clone(SASL_OUTCOME_HANDLE value)
{
	SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)amqpalloc_malloc(sizeof(SASL_OUTCOME_INSTANCE));
	if (sasl_outcome_instance != NULL)
	{
		sasl_outcome_instance->composite_value = amqpvalue_clone(((SASL_OUTCOME_INSTANCE*)value)->composite_value);
		if (sasl_outcome_instance->composite_value == NULL)
		{
			amqpalloc_free(sasl_outcome_instance);
			sasl_outcome_instance = NULL;
		}
	}

	return sasl_outcome_instance;
}

void sasl_outcome_destroy(SASL_OUTCOME_HANDLE sasl_outcome)
{
	if (sasl_outcome != NULL)
	{
		SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome;
		amqpvalue_destroy(sasl_outcome_instance->composite_value);
		amqpalloc_free(sasl_outcome_instance);
	}
}

AMQP_VALUE amqpvalue_create_sasl_outcome(SASL_OUTCOME_HANDLE sasl_outcome)
{
	AMQP_VALUE result;

	if (sasl_outcome == NULL)
	{
		result = NULL;
	}
	else
	{
		SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome;
		result = amqpvalue_clone(sasl_outcome_instance->composite_value);
	}

	return result;
}

bool is_sasl_outcome_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 68))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_sasl_outcome(AMQP_VALUE value, SASL_OUTCOME_HANDLE* sasl_outcome_handle)
{
	int result;
	SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome_create_internal();
	*sasl_outcome_handle = sasl_outcome_instance;
	if (*sasl_outcome_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			sasl_outcome_destroy(*sasl_outcome_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* code */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					{
						sasl_outcome_destroy(*sasl_outcome_handle);
						result = __LINE__;
						break;
					}
				}
				else
				{
					sasl_code code;
					if (amqpvalue_get_sasl_code(item_value, &code) != 0)
					{
						sasl_outcome_destroy(*sasl_outcome_handle);
						result = __LINE__;
						break;
					}

					amqpvalue_destroy(item_value);
				}
				/* additional-data */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqp_binary additional_data;
					if (amqpvalue_get_binary(item_value, &additional_data) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							sasl_outcome_destroy(*sasl_outcome_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				sasl_outcome_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int sasl_outcome_get_code(SASL_OUTCOME_HANDLE sasl_outcome, sasl_code* code_value)
{
	int result;

	if (sasl_outcome == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_outcome_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_sasl_code(item_value, code_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_outcome_set_code(SASL_OUTCOME_HANDLE sasl_outcome, sasl_code code_value)
{
	int result;

	if (sasl_outcome == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome;
		AMQP_VALUE code_amqp_value = amqpvalue_create_sasl_code(code_value);
		if (code_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_outcome_instance->composite_value, 0, code_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(code_amqp_value);
		}
	}

	return result;
}

int sasl_outcome_get_additional_data(SASL_OUTCOME_HANDLE sasl_outcome, amqp_binary* additional_data_value)
{
	int result;

	if (sasl_outcome == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(sasl_outcome_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_binary(item_value, additional_data_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int sasl_outcome_set_additional_data(SASL_OUTCOME_HANDLE sasl_outcome, amqp_binary additional_data_value)
{
	int result;

	if (sasl_outcome == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SASL_OUTCOME_INSTANCE* sasl_outcome_instance = (SASL_OUTCOME_INSTANCE*)sasl_outcome;
		AMQP_VALUE additional_data_amqp_value = amqpvalue_create_binary(additional_data_value);
		if (additional_data_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(sasl_outcome_instance->composite_value, 1, additional_data_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(additional_data_amqp_value);
		}
	}

	return result;
}


/* terminus-durability */

AMQP_VALUE amqpvalue_create_terminus_durability(terminus_durability value)
{
	return amqpvalue_create_uint(value);
}

/* terminus-expiry-policy */

AMQP_VALUE amqpvalue_create_terminus_expiry_policy(terminus_expiry_policy value)
{
	return amqpvalue_create_symbol(value);
}

/* node-properties */

AMQP_VALUE amqpvalue_create_node_properties(node_properties value)
{
	return amqpvalue_create_fields(value);
}

/* filter-set */

AMQP_VALUE amqpvalue_create_filter_set(AMQP_VALUE value)
{
	return amqpvalue_clone(value);
}

/* source */

typedef struct SOURCE_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} SOURCE_INSTANCE;

static SOURCE_HANDLE source_create_internal(void)
{
	SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)amqpalloc_malloc(sizeof(SOURCE_INSTANCE));
	if (source_instance != NULL)
	{
		source_instance->composite_value = NULL;
	}

	return source_instance;
}

SOURCE_HANDLE source_create(void)
{
	SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)amqpalloc_malloc(sizeof(SOURCE_INSTANCE));
	if (source_instance != NULL)
	{
		source_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(40);
		if (source_instance->composite_value == NULL)
		{
			amqpalloc_free(source_instance);
			source_instance = NULL;
		}
	}

	return source_instance;
}

SOURCE_HANDLE source_clone(SOURCE_HANDLE value)
{
	SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)amqpalloc_malloc(sizeof(SOURCE_INSTANCE));
	if (source_instance != NULL)
	{
		source_instance->composite_value = amqpvalue_clone(((SOURCE_INSTANCE*)value)->composite_value);
		if (source_instance->composite_value == NULL)
		{
			amqpalloc_free(source_instance);
			source_instance = NULL;
		}
	}

	return source_instance;
}

void source_destroy(SOURCE_HANDLE source)
{
	if (source != NULL)
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		amqpvalue_destroy(source_instance->composite_value);
		amqpalloc_free(source_instance);
	}
}

AMQP_VALUE amqpvalue_create_source(SOURCE_HANDLE source)
{
	AMQP_VALUE result;

	if (source == NULL)
	{
		result = NULL;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		result = amqpvalue_clone(source_instance->composite_value);
	}

	return result;
}

bool is_source_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 40))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_source(AMQP_VALUE value, SOURCE_HANDLE* source_handle)
{
	int result;
	SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source_create_internal();
	*source_handle = source_instance;
	if (*source_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			source_destroy(*source_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* address */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* durable */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					terminus_durability durable;
					if (amqpvalue_get_terminus_durability(item_value, &durable) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* expiry-policy */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					terminus_expiry_policy expiry_policy;
					if (amqpvalue_get_terminus_expiry_policy(item_value, &expiry_policy) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* timeout */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					seconds timeout;
					if (amqpvalue_get_seconds(item_value, &timeout) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* dynamic */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool dynamic;
					if (amqpvalue_get_boolean(item_value, &dynamic) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* dynamic-node-properties */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					node_properties dynamic_node_properties;
					if (amqpvalue_get_node_properties(item_value, &dynamic_node_properties) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* distribution-mode */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* distribution_mode;
					if (amqpvalue_get_symbol(item_value, &distribution_mode) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* filter */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					filter_set filter;
					if (amqpvalue_get_filter_set(item_value, &filter) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* default-outcome */
				item_value = amqpvalue_get_list_item(list_value, 8);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* outcomes */
				item_value = amqpvalue_get_list_item(list_value, 9);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* outcomes;
					if (amqpvalue_get_symbol(item_value, &outcomes) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* capabilities */
				item_value = amqpvalue_get_list_item(list_value, 10);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* capabilities;
					if (amqpvalue_get_symbol(item_value, &capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							source_destroy(*source_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				source_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int source_get_address(SOURCE_HANDLE source, AMQP_VALUE* address_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*address_value = item_value;
			result = 0;
		}
	}

	return result;
}

int source_set_address(SOURCE_HANDLE source, AMQP_VALUE address_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE address_amqp_value = amqpvalue_clone(address_value);
		if (address_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 0, address_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(address_amqp_value);
		}
	}

	return result;
}

int source_get_durable(SOURCE_HANDLE source, terminus_durability* durable_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_terminus_durability(item_value, durable_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_durable(SOURCE_HANDLE source, terminus_durability durable_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE durable_amqp_value = amqpvalue_create_terminus_durability(durable_value);
		if (durable_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 1, durable_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(durable_amqp_value);
		}
	}

	return result;
}

int source_get_expiry_policy(SOURCE_HANDLE source, terminus_expiry_policy* expiry_policy_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_terminus_expiry_policy(item_value, expiry_policy_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_expiry_policy(SOURCE_HANDLE source, terminus_expiry_policy expiry_policy_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE expiry_policy_amqp_value = amqpvalue_create_terminus_expiry_policy(expiry_policy_value);
		if (expiry_policy_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 2, expiry_policy_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(expiry_policy_amqp_value);
		}
	}

	return result;
}

int source_get_timeout(SOURCE_HANDLE source, seconds* timeout_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_seconds(item_value, timeout_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_timeout(SOURCE_HANDLE source, seconds timeout_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE timeout_amqp_value = amqpvalue_create_seconds(timeout_value);
		if (timeout_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 3, timeout_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(timeout_amqp_value);
		}
	}

	return result;
}

int source_get_dynamic(SOURCE_HANDLE source, bool* dynamic_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, dynamic_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_dynamic(SOURCE_HANDLE source, bool dynamic_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE dynamic_amqp_value = amqpvalue_create_boolean(dynamic_value);
		if (dynamic_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 4, dynamic_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(dynamic_amqp_value);
		}
	}

	return result;
}

int source_get_dynamic_node_properties(SOURCE_HANDLE source, node_properties* dynamic_node_properties_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_node_properties(item_value, dynamic_node_properties_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_dynamic_node_properties(SOURCE_HANDLE source, node_properties dynamic_node_properties_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE dynamic_node_properties_amqp_value = amqpvalue_create_node_properties(dynamic_node_properties_value);
		if (dynamic_node_properties_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 5, dynamic_node_properties_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(dynamic_node_properties_amqp_value);
		}
	}

	return result;
}

int source_get_distribution_mode(SOURCE_HANDLE source, const char** distribution_mode_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, distribution_mode_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_distribution_mode(SOURCE_HANDLE source, const char* distribution_mode_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE distribution_mode_amqp_value = amqpvalue_create_symbol(distribution_mode_value);
		if (distribution_mode_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 6, distribution_mode_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(distribution_mode_amqp_value);
		}
	}

	return result;
}

int source_get_filter(SOURCE_HANDLE source, filter_set* filter_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_filter_set(item_value, filter_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_filter(SOURCE_HANDLE source, filter_set filter_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE filter_amqp_value = amqpvalue_create_filter_set(filter_value);
		if (filter_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 7, filter_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(filter_amqp_value);
		}
	}

	return result;
}

int source_get_default_outcome(SOURCE_HANDLE source, AMQP_VALUE* default_outcome_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 8);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*default_outcome_value = item_value;
			result = 0;
		}
	}

	return result;
}

int source_set_default_outcome(SOURCE_HANDLE source, AMQP_VALUE default_outcome_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE default_outcome_amqp_value = amqpvalue_clone(default_outcome_value);
		if (default_outcome_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 8, default_outcome_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(default_outcome_amqp_value);
		}
	}

	return result;
}

int source_get_outcomes(SOURCE_HANDLE source, const char** outcomes_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 9);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, outcomes_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_outcomes(SOURCE_HANDLE source, const char* outcomes_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE outcomes_amqp_value = amqpvalue_create_symbol(outcomes_value);
		if (outcomes_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 9, outcomes_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(outcomes_amqp_value);
		}
	}

	return result;
}

int source_get_capabilities(SOURCE_HANDLE source, const char** capabilities_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
			SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(source_instance->composite_value, 10);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int source_set_capabilities(SOURCE_HANDLE source, const char* capabilities_value)
{
	int result;

	if (source == NULL)
	{
		result = __LINE__;
	}
	else
	{
		SOURCE_INSTANCE* source_instance = (SOURCE_INSTANCE*)source;
		AMQP_VALUE capabilities_amqp_value = amqpvalue_create_symbol(capabilities_value);
		if (capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(source_instance->composite_value, 10, capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(capabilities_amqp_value);
		}
	}

	return result;
}


/* target */

typedef struct TARGET_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} TARGET_INSTANCE;

static TARGET_HANDLE target_create_internal(void)
{
	TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)amqpalloc_malloc(sizeof(TARGET_INSTANCE));
	if (target_instance != NULL)
	{
		target_instance->composite_value = NULL;
	}

	return target_instance;
}

TARGET_HANDLE target_create(void)
{
	TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)amqpalloc_malloc(sizeof(TARGET_INSTANCE));
	if (target_instance != NULL)
	{
		target_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(41);
		if (target_instance->composite_value == NULL)
		{
			amqpalloc_free(target_instance);
			target_instance = NULL;
		}
	}

	return target_instance;
}

TARGET_HANDLE target_clone(TARGET_HANDLE value)
{
	TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)amqpalloc_malloc(sizeof(TARGET_INSTANCE));
	if (target_instance != NULL)
	{
		target_instance->composite_value = amqpvalue_clone(((TARGET_INSTANCE*)value)->composite_value);
		if (target_instance->composite_value == NULL)
		{
			amqpalloc_free(target_instance);
			target_instance = NULL;
		}
	}

	return target_instance;
}

void target_destroy(TARGET_HANDLE target)
{
	if (target != NULL)
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		amqpvalue_destroy(target_instance->composite_value);
		amqpalloc_free(target_instance);
	}
}

AMQP_VALUE amqpvalue_create_target(TARGET_HANDLE target)
{
	AMQP_VALUE result;

	if (target == NULL)
	{
		result = NULL;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		result = amqpvalue_clone(target_instance->composite_value);
	}

	return result;
}

bool is_target_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 41))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_target(AMQP_VALUE value, TARGET_HANDLE* target_handle)
{
	int result;
	TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target_create_internal();
	*target_handle = target_instance;
	if (*target_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			target_destroy(*target_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* address */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* durable */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					terminus_durability durable;
					if (amqpvalue_get_terminus_durability(item_value, &durable) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							target_destroy(*target_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* expiry-policy */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					terminus_expiry_policy expiry_policy;
					if (amqpvalue_get_terminus_expiry_policy(item_value, &expiry_policy) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							target_destroy(*target_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* timeout */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					seconds timeout;
					if (amqpvalue_get_seconds(item_value, &timeout) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							target_destroy(*target_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* dynamic */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool dynamic;
					if (amqpvalue_get_boolean(item_value, &dynamic) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							target_destroy(*target_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* dynamic-node-properties */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					node_properties dynamic_node_properties;
					if (amqpvalue_get_node_properties(item_value, &dynamic_node_properties) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							target_destroy(*target_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* capabilities */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* capabilities;
					if (amqpvalue_get_symbol(item_value, &capabilities) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							target_destroy(*target_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				target_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int target_get_address(TARGET_HANDLE target, AMQP_VALUE* address_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*address_value = item_value;
			result = 0;
		}
	}

	return result;
}

int target_set_address(TARGET_HANDLE target, AMQP_VALUE address_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE address_amqp_value = amqpvalue_clone(address_value);
		if (address_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 0, address_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(address_amqp_value);
		}
	}

	return result;
}

int target_get_durable(TARGET_HANDLE target, terminus_durability* durable_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_terminus_durability(item_value, durable_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int target_set_durable(TARGET_HANDLE target, terminus_durability durable_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE durable_amqp_value = amqpvalue_create_terminus_durability(durable_value);
		if (durable_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 1, durable_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(durable_amqp_value);
		}
	}

	return result;
}

int target_get_expiry_policy(TARGET_HANDLE target, terminus_expiry_policy* expiry_policy_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_terminus_expiry_policy(item_value, expiry_policy_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int target_set_expiry_policy(TARGET_HANDLE target, terminus_expiry_policy expiry_policy_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE expiry_policy_amqp_value = amqpvalue_create_terminus_expiry_policy(expiry_policy_value);
		if (expiry_policy_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 2, expiry_policy_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(expiry_policy_amqp_value);
		}
	}

	return result;
}

int target_get_timeout(TARGET_HANDLE target, seconds* timeout_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_seconds(item_value, timeout_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int target_set_timeout(TARGET_HANDLE target, seconds timeout_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE timeout_amqp_value = amqpvalue_create_seconds(timeout_value);
		if (timeout_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 3, timeout_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(timeout_amqp_value);
		}
	}

	return result;
}

int target_get_dynamic(TARGET_HANDLE target, bool* dynamic_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, dynamic_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int target_set_dynamic(TARGET_HANDLE target, bool dynamic_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE dynamic_amqp_value = amqpvalue_create_boolean(dynamic_value);
		if (dynamic_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 4, dynamic_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(dynamic_amqp_value);
		}
	}

	return result;
}

int target_get_dynamic_node_properties(TARGET_HANDLE target, node_properties* dynamic_node_properties_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_node_properties(item_value, dynamic_node_properties_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int target_set_dynamic_node_properties(TARGET_HANDLE target, node_properties dynamic_node_properties_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE dynamic_node_properties_amqp_value = amqpvalue_create_node_properties(dynamic_node_properties_value);
		if (dynamic_node_properties_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 5, dynamic_node_properties_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(dynamic_node_properties_amqp_value);
		}
	}

	return result;
}

int target_get_capabilities(TARGET_HANDLE target, const char** capabilities_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
			TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(target_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, capabilities_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int target_set_capabilities(TARGET_HANDLE target, const char* capabilities_value)
{
	int result;

	if (target == NULL)
	{
		result = __LINE__;
	}
	else
	{
		TARGET_INSTANCE* target_instance = (TARGET_INSTANCE*)target;
		AMQP_VALUE capabilities_amqp_value = amqpvalue_create_symbol(capabilities_value);
		if (capabilities_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(target_instance->composite_value, 6, capabilities_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(capabilities_amqp_value);
		}
	}

	return result;
}


/* annotations */

AMQP_VALUE amqpvalue_create_annotations(AMQP_VALUE value)
{
	return amqpvalue_clone(value);
}

/* message-id-ulong */

AMQP_VALUE amqpvalue_create_message_id_ulong(message_id_ulong value)
{
	return amqpvalue_create_ulong(value);
}

/* message-id-uuid */

AMQP_VALUE amqpvalue_create_message_id_uuid(message_id_uuid value)
{
	return amqpvalue_create_uuid(value);
}

/* message-id-binary */

AMQP_VALUE amqpvalue_create_message_id_binary(message_id_binary value)
{
	return amqpvalue_create_binary(value);
}

/* message-id-string */

AMQP_VALUE amqpvalue_create_message_id_string(message_id_string value)
{
	return amqpvalue_create_string(value);
}

/* address-string */

AMQP_VALUE amqpvalue_create_address_string(address_string value)
{
	return amqpvalue_create_string(value);
}

/* header */

typedef struct HEADER_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} HEADER_INSTANCE;

static HEADER_HANDLE header_create_internal(void)
{
	HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)amqpalloc_malloc(sizeof(HEADER_INSTANCE));
	if (header_instance != NULL)
	{
		header_instance->composite_value = NULL;
	}

	return header_instance;
}

HEADER_HANDLE header_create(void)
{
	HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)amqpalloc_malloc(sizeof(HEADER_INSTANCE));
	if (header_instance != NULL)
	{
		header_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(112);
		if (header_instance->composite_value == NULL)
		{
			amqpalloc_free(header_instance);
			header_instance = NULL;
		}
	}

	return header_instance;
}

HEADER_HANDLE header_clone(HEADER_HANDLE value)
{
	HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)amqpalloc_malloc(sizeof(HEADER_INSTANCE));
	if (header_instance != NULL)
	{
		header_instance->composite_value = amqpvalue_clone(((HEADER_INSTANCE*)value)->composite_value);
		if (header_instance->composite_value == NULL)
		{
			amqpalloc_free(header_instance);
			header_instance = NULL;
		}
	}

	return header_instance;
}

void header_destroy(HEADER_HANDLE header)
{
	if (header != NULL)
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		amqpvalue_destroy(header_instance->composite_value);
		amqpalloc_free(header_instance);
	}
}

AMQP_VALUE amqpvalue_create_header(HEADER_HANDLE header)
{
	AMQP_VALUE result;

	if (header == NULL)
	{
		result = NULL;
	}
	else
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		result = amqpvalue_clone(header_instance->composite_value);
	}

	return result;
}

bool is_header_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 112))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_header(AMQP_VALUE value, HEADER_HANDLE* header_handle)
{
	int result;
	HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header_create_internal();
	*header_handle = header_instance;
	if (*header_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			header_destroy(*header_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* durable */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool durable;
					if (amqpvalue_get_boolean(item_value, &durable) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							header_destroy(*header_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* priority */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint8_t priority;
					if (amqpvalue_get_ubyte(item_value, &priority) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							header_destroy(*header_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* ttl */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					milliseconds ttl;
					if (amqpvalue_get_milliseconds(item_value, &ttl) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							header_destroy(*header_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* first-acquirer */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					bool first_acquirer;
					if (amqpvalue_get_boolean(item_value, &first_acquirer) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							header_destroy(*header_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* delivery-count */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					uint32_t delivery_count;
					if (amqpvalue_get_uint(item_value, &delivery_count) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							header_destroy(*header_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				header_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int header_get_durable(HEADER_HANDLE header, bool* durable_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
			HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(header_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, durable_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int header_set_durable(HEADER_HANDLE header, bool durable_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE durable_amqp_value = amqpvalue_create_boolean(durable_value);
		if (durable_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(header_instance->composite_value, 0, durable_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(durable_amqp_value);
		}
	}

	return result;
}

int header_get_priority(HEADER_HANDLE header, uint8_t* priority_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
			HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(header_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_ubyte(item_value, priority_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int header_set_priority(HEADER_HANDLE header, uint8_t priority_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE priority_amqp_value = amqpvalue_create_ubyte(priority_value);
		if (priority_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(header_instance->composite_value, 1, priority_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(priority_amqp_value);
		}
	}

	return result;
}

int header_get_ttl(HEADER_HANDLE header, milliseconds* ttl_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
			HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(header_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_milliseconds(item_value, ttl_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int header_set_ttl(HEADER_HANDLE header, milliseconds ttl_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE ttl_amqp_value = amqpvalue_create_milliseconds(ttl_value);
		if (ttl_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(header_instance->composite_value, 2, ttl_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(ttl_amqp_value);
		}
	}

	return result;
}

int header_get_first_acquirer(HEADER_HANDLE header, bool* first_acquirer_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
			HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(header_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_boolean(item_value, first_acquirer_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int header_set_first_acquirer(HEADER_HANDLE header, bool first_acquirer_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE first_acquirer_amqp_value = amqpvalue_create_boolean(first_acquirer_value);
		if (first_acquirer_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(header_instance->composite_value, 3, first_acquirer_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(first_acquirer_amqp_value);
		}
	}

	return result;
}

int header_get_delivery_count(HEADER_HANDLE header, uint32_t* delivery_count_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
			HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(header_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_uint(item_value, delivery_count_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int header_set_delivery_count(HEADER_HANDLE header, uint32_t delivery_count_value)
{
	int result;

	if (header == NULL)
	{
		result = __LINE__;
	}
	else
	{
		HEADER_INSTANCE* header_instance = (HEADER_INSTANCE*)header;
		AMQP_VALUE delivery_count_amqp_value = amqpvalue_create_uint(delivery_count_value);
		if (delivery_count_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(header_instance->composite_value, 4, delivery_count_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(delivery_count_amqp_value);
		}
	}

	return result;
}


/* delivery-annotations */

AMQP_VALUE amqpvalue_create_delivery_annotations(delivery_annotations value)
{

	AMQP_VALUE result;
	AMQP_VALUE described_value = amqpvalue_create_annotations(value);
	if (described_value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE descriptor = amqpvalue_create_uint(113);
		if (descriptor == NULL)
		{
			result = NULL;
		}
		else
		{
			result = amqpvalue_create_described(descriptor, described_value);

			amqpvalue_destroy(descriptor);
		}

		amqpvalue_destroy(described_value);
	}

	return result;
}

/* message-annotations */

AMQP_VALUE amqpvalue_create_message_annotations(message_annotations value)
{

	AMQP_VALUE result;
	AMQP_VALUE described_value = amqpvalue_create_annotations(value);
	if (described_value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE descriptor = amqpvalue_create_uint(114);
		if (descriptor == NULL)
		{
			result = NULL;
		}
		else
		{
			result = amqpvalue_create_described(descriptor, described_value);

			amqpvalue_destroy(descriptor);
		}

		amqpvalue_destroy(described_value);
	}

	return result;
}

/* application-properties */

AMQP_VALUE amqpvalue_create_application_properties(AMQP_VALUE value)
{
	return amqpvalue_clone(value);
}

/* data */

AMQP_VALUE amqpvalue_create_data(data value)
{

	AMQP_VALUE result;
	AMQP_VALUE described_value = amqpvalue_create_binary(value);
	if (described_value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE descriptor = amqpvalue_create_uint(117);
		if (descriptor == NULL)
		{
			result = NULL;
		}
		else
		{
			result = amqpvalue_create_described(descriptor, described_value);

			amqpvalue_destroy(descriptor);
		}

		amqpvalue_destroy(described_value);
	}

	return result;
}

/* amqp-sequence */

AMQP_VALUE amqpvalue_create_amqp_sequence(AMQP_VALUE value)
{
	return amqpvalue_clone(value);
}

/* amqp-value */

AMQP_VALUE amqpvalue_create_amqp_value(AMQP_VALUE value)
{
	return amqpvalue_clone(value);
}

/* footer */

AMQP_VALUE amqpvalue_create_footer(footer value)
{

	AMQP_VALUE result;
	AMQP_VALUE described_value = amqpvalue_create_annotations(value);
	if (described_value == NULL)
	{
		result = NULL;
	}
	else
	{
		AMQP_VALUE descriptor = amqpvalue_create_uint(120);
		if (descriptor == NULL)
		{
			result = NULL;
		}
		else
		{
			result = amqpvalue_create_described(descriptor, described_value);

			amqpvalue_destroy(descriptor);
		}

		amqpvalue_destroy(described_value);
	}

	return result;
}

/* properties */

typedef struct PROPERTIES_INSTANCE_TAG
{
	AMQP_VALUE composite_value;
} PROPERTIES_INSTANCE;

static PROPERTIES_HANDLE properties_create_internal(void)
{
	PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)amqpalloc_malloc(sizeof(PROPERTIES_INSTANCE));
	if (properties_instance != NULL)
	{
		properties_instance->composite_value = NULL;
	}

	return properties_instance;
}

PROPERTIES_HANDLE properties_create(void)
{
	PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)amqpalloc_malloc(sizeof(PROPERTIES_INSTANCE));
	if (properties_instance != NULL)
	{
		properties_instance->composite_value = amqpvalue_create_composite_with_ulong_descriptor(115);
		if (properties_instance->composite_value == NULL)
		{
			amqpalloc_free(properties_instance);
			properties_instance = NULL;
		}
	}

	return properties_instance;
}

PROPERTIES_HANDLE properties_clone(PROPERTIES_HANDLE value)
{
	PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)amqpalloc_malloc(sizeof(PROPERTIES_INSTANCE));
	if (properties_instance != NULL)
	{
		properties_instance->composite_value = amqpvalue_clone(((PROPERTIES_INSTANCE*)value)->composite_value);
		if (properties_instance->composite_value == NULL)
		{
			amqpalloc_free(properties_instance);
			properties_instance = NULL;
		}
	}

	return properties_instance;
}

void properties_destroy(PROPERTIES_HANDLE properties)
{
	if (properties != NULL)
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		amqpvalue_destroy(properties_instance->composite_value);
		amqpalloc_free(properties_instance);
	}
}

AMQP_VALUE amqpvalue_create_properties(PROPERTIES_HANDLE properties)
{
	AMQP_VALUE result;

	if (properties == NULL)
	{
		result = NULL;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		result = amqpvalue_clone(properties_instance->composite_value);
	}

	return result;
}

bool is_properties_type_by_descriptor(AMQP_VALUE descriptor)
{
	bool result;

	uint64_t descriptor_ulong;
	if ((amqpvalue_get_ulong(descriptor, &descriptor_ulong) == 0) &&
		(descriptor_ulong == 115))
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}


int amqpvalue_get_properties(AMQP_VALUE value, PROPERTIES_HANDLE* properties_handle)
{
	int result;
	PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties_create_internal();
	*properties_handle = properties_instance;
	if (*properties_handle == NULL)
	{
		result = __LINE__;
	}
	else
	{
		AMQP_VALUE list_value = amqpvalue_get_described_value(value);
		if (list_value == NULL)
		{
			properties_destroy(*properties_handle);
			result = __LINE__;
		}
		else
		{
			AMQP_VALUE item_value;
			do
			{
				/* message-id */
				item_value = amqpvalue_get_list_item(list_value, 0);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* user-id */
				item_value = amqpvalue_get_list_item(list_value, 1);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqp_binary user_id;
					if (amqpvalue_get_binary(item_value, &user_id) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* to */
				item_value = amqpvalue_get_list_item(list_value, 2);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* subject */
				item_value = amqpvalue_get_list_item(list_value, 3);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* subject;
					if (amqpvalue_get_string(item_value, &subject) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* reply-to */
				item_value = amqpvalue_get_list_item(list_value, 4);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* correlation-id */
				item_value = amqpvalue_get_list_item(list_value, 5);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					amqpvalue_destroy(item_value);
				}
				/* content-type */
				item_value = amqpvalue_get_list_item(list_value, 6);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* content_type;
					if (amqpvalue_get_symbol(item_value, &content_type) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* content-encoding */
				item_value = amqpvalue_get_list_item(list_value, 7);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* content_encoding;
					if (amqpvalue_get_symbol(item_value, &content_encoding) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* absolute-expiry-time */
				item_value = amqpvalue_get_list_item(list_value, 8);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					timestamp absolute_expiry_time;
					if (amqpvalue_get_timestamp(item_value, &absolute_expiry_time) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* creation-time */
				item_value = amqpvalue_get_list_item(list_value, 9);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					timestamp creation_time;
					if (amqpvalue_get_timestamp(item_value, &creation_time) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* group-id */
				item_value = amqpvalue_get_list_item(list_value, 10);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* group_id;
					if (amqpvalue_get_string(item_value, &group_id) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* group-sequence */
				item_value = amqpvalue_get_list_item(list_value, 11);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					sequence_no group_sequence;
					if (amqpvalue_get_sequence_no(item_value, &group_sequence) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}
				/* reply-to-group-id */
				item_value = amqpvalue_get_list_item(list_value, 12);
				if (item_value == NULL)
				{
					/* do nothing */
				}
				else
				{
					const char* reply_to_group_id;
					if (amqpvalue_get_string(item_value, &reply_to_group_id) != 0)
					{
						if (amqpvalue_get_type(item_value) != AMQP_TYPE_NULL)
						{
							properties_destroy(*properties_handle);
							result = __LINE__;
							break;
						}
					}

					amqpvalue_destroy(item_value);
				}

				properties_instance->composite_value = amqpvalue_clone(value);

				result = 0;
			} while (0);
		}
	}

	return result;
}

int properties_get_message_id(PROPERTIES_HANDLE properties, AMQP_VALUE* message_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 0);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*message_id_value = item_value;
			result = 0;
		}
	}

	return result;
}

int properties_set_message_id(PROPERTIES_HANDLE properties, AMQP_VALUE message_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE message_id_amqp_value = amqpvalue_clone(message_id_value);
		if (message_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 0, message_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(message_id_amqp_value);
		}
	}

	return result;
}

int properties_get_user_id(PROPERTIES_HANDLE properties, amqp_binary* user_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 1);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_binary(item_value, user_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_user_id(PROPERTIES_HANDLE properties, amqp_binary user_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE user_id_amqp_value = amqpvalue_create_binary(user_id_value);
		if (user_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 1, user_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(user_id_amqp_value);
		}
	}

	return result;
}

int properties_get_to(PROPERTIES_HANDLE properties, AMQP_VALUE* to_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 2);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*to_value = item_value;
			result = 0;
		}
	}

	return result;
}

int properties_set_to(PROPERTIES_HANDLE properties, AMQP_VALUE to_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE to_amqp_value = amqpvalue_clone(to_value);
		if (to_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 2, to_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(to_amqp_value);
		}
	}

	return result;
}

int properties_get_subject(PROPERTIES_HANDLE properties, const char** subject_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 3);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, subject_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_subject(PROPERTIES_HANDLE properties, const char* subject_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE subject_amqp_value = amqpvalue_create_string(subject_value);
		if (subject_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 3, subject_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(subject_amqp_value);
		}
	}

	return result;
}

int properties_get_reply_to(PROPERTIES_HANDLE properties, AMQP_VALUE* reply_to_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 4);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*reply_to_value = item_value;
			result = 0;
		}
	}

	return result;
}

int properties_set_reply_to(PROPERTIES_HANDLE properties, AMQP_VALUE reply_to_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE reply_to_amqp_value = amqpvalue_clone(reply_to_value);
		if (reply_to_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 4, reply_to_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(reply_to_amqp_value);
		}
	}

	return result;
}

int properties_get_correlation_id(PROPERTIES_HANDLE properties, AMQP_VALUE* correlation_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 5);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			*correlation_id_value = item_value;
			result = 0;
		}
	}

	return result;
}

int properties_set_correlation_id(PROPERTIES_HANDLE properties, AMQP_VALUE correlation_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE correlation_id_amqp_value = amqpvalue_clone(correlation_id_value);
		if (correlation_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 5, correlation_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(correlation_id_amqp_value);
		}
	}

	return result;
}

int properties_get_content_type(PROPERTIES_HANDLE properties, const char** content_type_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 6);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, content_type_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_content_type(PROPERTIES_HANDLE properties, const char* content_type_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE content_type_amqp_value = amqpvalue_create_symbol(content_type_value);
		if (content_type_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 6, content_type_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(content_type_amqp_value);
		}
	}

	return result;
}

int properties_get_content_encoding(PROPERTIES_HANDLE properties, const char** content_encoding_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 7);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_symbol(item_value, content_encoding_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_content_encoding(PROPERTIES_HANDLE properties, const char* content_encoding_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE content_encoding_amqp_value = amqpvalue_create_symbol(content_encoding_value);
		if (content_encoding_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 7, content_encoding_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(content_encoding_amqp_value);
		}
	}

	return result;
}

int properties_get_absolute_expiry_time(PROPERTIES_HANDLE properties, timestamp* absolute_expiry_time_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 8);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_timestamp(item_value, absolute_expiry_time_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_absolute_expiry_time(PROPERTIES_HANDLE properties, timestamp absolute_expiry_time_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE absolute_expiry_time_amqp_value = amqpvalue_create_timestamp(absolute_expiry_time_value);
		if (absolute_expiry_time_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 8, absolute_expiry_time_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(absolute_expiry_time_amqp_value);
		}
	}

	return result;
}

int properties_get_creation_time(PROPERTIES_HANDLE properties, timestamp* creation_time_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 9);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_timestamp(item_value, creation_time_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_creation_time(PROPERTIES_HANDLE properties, timestamp creation_time_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE creation_time_amqp_value = amqpvalue_create_timestamp(creation_time_value);
		if (creation_time_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 9, creation_time_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(creation_time_amqp_value);
		}
	}

	return result;
}

int properties_get_group_id(PROPERTIES_HANDLE properties, const char** group_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 10);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, group_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_group_id(PROPERTIES_HANDLE properties, const char* group_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE group_id_amqp_value = amqpvalue_create_string(group_id_value);
		if (group_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 10, group_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(group_id_amqp_value);
		}
	}

	return result;
}

int properties_get_group_sequence(PROPERTIES_HANDLE properties, sequence_no* group_sequence_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 11);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_sequence_no(item_value, group_sequence_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_group_sequence(PROPERTIES_HANDLE properties, sequence_no group_sequence_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE group_sequence_amqp_value = amqpvalue_create_sequence_no(group_sequence_value);
		if (group_sequence_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 11, group_sequence_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(group_sequence_amqp_value);
		}
	}

	return result;
}

int properties_get_reply_to_group_id(PROPERTIES_HANDLE properties, const char** reply_to_group_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
			PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE item_value = amqpvalue_get_composite_item_in_place(properties_instance->composite_value, 12);
		if (item_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_get_string(item_value, reply_to_group_id_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}
		}
	}

	return result;
}

int properties_set_reply_to_group_id(PROPERTIES_HANDLE properties, const char* reply_to_group_id_value)
{
	int result;

	if (properties == NULL)
	{
		result = __LINE__;
	}
	else
	{
		PROPERTIES_INSTANCE* properties_instance = (PROPERTIES_INSTANCE*)properties;
		AMQP_VALUE reply_to_group_id_amqp_value = amqpvalue_create_string(reply_to_group_id_value);
		if (reply_to_group_id_amqp_value == NULL)
		{
			result = __LINE__;
		}
		else
		{
			if (amqpvalue_set_composite_item(properties_instance->composite_value, 12, reply_to_group_id_amqp_value) != 0)
			{
				result = __LINE__;
			}
			else
			{
				result = 0;
			}

			amqpvalue_destroy(reply_to_group_id_amqp_value);
		}
	}

	return result;
}


