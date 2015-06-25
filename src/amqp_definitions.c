

#include "amqpvalue.h"
#include "amqp_definitions.h"
#include <stdlib.h>

/* error */

	typedef struct ERROR_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} ERROR_INSTANCE;

	ERROR_HANDLE error_create(uint32_t condition)
	{
		ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)malloc(sizeof(ERROR_INSTANCE));
		if (error_instance != NULL)
		{
			error_instance->list = amqpvalue_create_list(1);
			if (error_instance->list == NULL)
			{
				free(error_instance);
				error_instance = NULL;
			}
			else
			{
				AMQP_VALUE condition_value;
				int result = 0;

				condition_value = amqpvalue_create_symbol(condition);
				if ((result == 0) && (amqpvalue_set_list_item(error_instance->list, 0, condition_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(condition_value);
			}
		}

		return error_instance;
	}

	void error_destroy(ERROR_HANDLE error)
	{
		if (error != NULL)
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			amqpvalue_destroy(error_instance->list);
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
			result = amqpvalue_clone(error_instance->list);
		}

		return result;
	}

	int error_get_condition(ERROR_HANDLE error, uint32_t* condition)
	{
		int result;

		if (error == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			result = 0;
		}

		return result;
	}

	int error_set_condition(ERROR_HANDLE error, uint32_t condition)
	{
		int result;

		if (error == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			AMQP_VALUE condition_value = amqpvalue_create_symbol(condition);
			if ((condition_value == NULL) ||
				(amqpvalue_set_list_item(error_instance->list, 0, condition_value) != 0))
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

	int error_get_description(ERROR_HANDLE error, const char** description)
	{
		int result;

		if (error == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			result = 0;
		}

		return result;
	}

	int error_set_description(ERROR_HANDLE error, const char* description)
	{
		int result;

		if (error == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			AMQP_VALUE description_value = amqpvalue_create_string(description);
			if ((description_value == NULL) ||
				(amqpvalue_set_list_item(error_instance->list, 1, description_value) != 0))
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

	int error_get_info(ERROR_HANDLE error, fields* info)
	{
		int result;

		if (error == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			result = 0;
		}

		return result;
	}

	int error_set_info(ERROR_HANDLE error, fields info)
	{
		int result;

		if (error == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ERROR_INSTANCE* error_instance = (ERROR_INSTANCE*)error;
			AMQP_VALUE info_value = amqpvalue_create_fields(info);
			if ((info_value == NULL) ||
				(amqpvalue_set_list_item(error_instance->list, 2, info_value) != 0))
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


/* open */

	typedef struct OPEN_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} OPEN_INSTANCE;

	OPEN_HANDLE open_create(const char* container_id)
	{
		OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)malloc(sizeof(OPEN_INSTANCE));
		if (open_instance != NULL)
		{
			open_instance->list = amqpvalue_create_list(1);
			if (open_instance->list == NULL)
			{
				free(open_instance);
				open_instance = NULL;
			}
			else
			{
				AMQP_VALUE container_id_value;
				int result = 0;

				container_id_value = amqpvalue_create_string(container_id);
				if ((result == 0) && (amqpvalue_set_list_item(open_instance->list, 0, container_id_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(container_id_value);
			}
		}

		return open_instance;
	}

	void open_destroy(OPEN_HANDLE open)
	{
		if (open != NULL)
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			amqpvalue_destroy(open_instance->list);
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
			result = amqpvalue_clone(open_instance->list);
		}

		return result;
	}

	int open_get_container_id(OPEN_HANDLE open, const char** container_id)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_container_id(OPEN_HANDLE open, const char* container_id)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE container_id_value = amqpvalue_create_string(container_id);
			if ((container_id_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 0, container_id_value) != 0))
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

	int open_get_hostname(OPEN_HANDLE open, const char** hostname)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_hostname(OPEN_HANDLE open, const char* hostname)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE hostname_value = amqpvalue_create_string(hostname);
			if ((hostname_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 1, hostname_value) != 0))
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

	int open_get_max_frame_size(OPEN_HANDLE open, uint32_t* max_frame_size)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_max_frame_size(OPEN_HANDLE open, uint32_t max_frame_size)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE max_frame_size_value = amqpvalue_create_uint(max_frame_size);
			if ((max_frame_size_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 2, max_frame_size_value) != 0))
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

	int open_get_channel_max(OPEN_HANDLE open, uint16_t* channel_max)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_channel_max(OPEN_HANDLE open, uint16_t channel_max)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE channel_max_value = amqpvalue_create_ushort(channel_max);
			if ((channel_max_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 3, channel_max_value) != 0))
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

	int open_get_idle_time_out(OPEN_HANDLE open, milliseconds* idle_time_out)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_idle_time_out(OPEN_HANDLE open, milliseconds idle_time_out)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE idle_time_out_value = amqpvalue_create_milliseconds(idle_time_out);
			if ((idle_time_out_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 4, idle_time_out_value) != 0))
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

	int open_get_outgoing_locales(OPEN_HANDLE open, ietf_language_tag* outgoing_locales)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_outgoing_locales(OPEN_HANDLE open, ietf_language_tag outgoing_locales)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE outgoing_locales_value = amqpvalue_create_ietf_language_tag(outgoing_locales);
			if ((outgoing_locales_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 5, outgoing_locales_value) != 0))
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

	int open_get_incoming_locales(OPEN_HANDLE open, ietf_language_tag* incoming_locales)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_incoming_locales(OPEN_HANDLE open, ietf_language_tag incoming_locales)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE incoming_locales_value = amqpvalue_create_ietf_language_tag(incoming_locales);
			if ((incoming_locales_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 6, incoming_locales_value) != 0))
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

	int open_get_offered_capabilities(OPEN_HANDLE open, uint32_t* offered_capabilities)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_offered_capabilities(OPEN_HANDLE open, uint32_t offered_capabilities)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE offered_capabilities_value = amqpvalue_create_symbol(offered_capabilities);
			if ((offered_capabilities_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 7, offered_capabilities_value) != 0))
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

	int open_get_desired_capabilities(OPEN_HANDLE open, uint32_t* desired_capabilities)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_desired_capabilities(OPEN_HANDLE open, uint32_t desired_capabilities)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE desired_capabilities_value = amqpvalue_create_symbol(desired_capabilities);
			if ((desired_capabilities_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 8, desired_capabilities_value) != 0))
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

	int open_get_properties(OPEN_HANDLE open, fields* properties)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			result = 0;
		}

		return result;
	}

	int open_set_properties(OPEN_HANDLE open, fields properties)
	{
		int result;

		if (open == NULL)
		{
			result = __LINE__;
		}
		else
		{
			OPEN_INSTANCE* open_instance = (OPEN_INSTANCE*)open;
			AMQP_VALUE properties_value = amqpvalue_create_fields(properties);
			if ((properties_value == NULL) ||
				(amqpvalue_set_list_item(open_instance->list, 9, properties_value) != 0))
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


/* begin */

	typedef struct BEGIN_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} BEGIN_INSTANCE;

	BEGIN_HANDLE begin_create(transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
	{
		BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)malloc(sizeof(BEGIN_INSTANCE));
		if (begin_instance != NULL)
		{
			begin_instance->list = amqpvalue_create_list(3);
			if (begin_instance->list == NULL)
			{
				free(begin_instance);
				begin_instance = NULL;
			}
			else
			{
				AMQP_VALUE next_outgoing_id_value;
				AMQP_VALUE incoming_window_value;
				AMQP_VALUE outgoing_window_value;
				int result = 0;

				next_outgoing_id_value = amqpvalue_create_transfer_number(next_outgoing_id);
				if ((result == 0) && (amqpvalue_set_list_item(begin_instance->list, 0, next_outgoing_id_value) != 0))
				{
					result = __LINE__;
				}
				incoming_window_value = amqpvalue_create_uint(incoming_window);
				if ((result == 0) && (amqpvalue_set_list_item(begin_instance->list, 1, incoming_window_value) != 0))
				{
					result = __LINE__;
				}
				outgoing_window_value = amqpvalue_create_uint(outgoing_window);
				if ((result == 0) && (amqpvalue_set_list_item(begin_instance->list, 2, outgoing_window_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(next_outgoing_id_value);
				amqpvalue_destroy(incoming_window_value);
				amqpvalue_destroy(outgoing_window_value);
			}
		}

		return begin_instance;
	}

	void begin_destroy(BEGIN_HANDLE begin)
	{
		if (begin != NULL)
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			amqpvalue_destroy(begin_instance->list);
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
			result = amqpvalue_clone(begin_instance->list);
		}

		return result;
	}

	int begin_get_remote_channel(BEGIN_HANDLE begin, uint16_t* remote_channel)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_remote_channel(BEGIN_HANDLE begin, uint16_t remote_channel)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE remote_channel_value = amqpvalue_create_ushort(remote_channel);
			if ((remote_channel_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 0, remote_channel_value) != 0))
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

	int begin_get_next_outgoing_id(BEGIN_HANDLE begin, transfer_number* next_outgoing_id)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_next_outgoing_id(BEGIN_HANDLE begin, transfer_number next_outgoing_id)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE next_outgoing_id_value = amqpvalue_create_transfer_number(next_outgoing_id);
			if ((next_outgoing_id_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 1, next_outgoing_id_value) != 0))
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

	int begin_get_incoming_window(BEGIN_HANDLE begin, uint32_t* incoming_window)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_incoming_window(BEGIN_HANDLE begin, uint32_t incoming_window)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE incoming_window_value = amqpvalue_create_uint(incoming_window);
			if ((incoming_window_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 2, incoming_window_value) != 0))
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

	int begin_get_outgoing_window(BEGIN_HANDLE begin, uint32_t* outgoing_window)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_outgoing_window(BEGIN_HANDLE begin, uint32_t outgoing_window)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE outgoing_window_value = amqpvalue_create_uint(outgoing_window);
			if ((outgoing_window_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 3, outgoing_window_value) != 0))
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

	int begin_get_handle_max(BEGIN_HANDLE begin, handle* handle_max)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_handle_max(BEGIN_HANDLE begin, handle handle_max)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE handle_max_value = amqpvalue_create_handle(handle_max);
			if ((handle_max_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 4, handle_max_value) != 0))
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

	int begin_get_offered_capabilities(BEGIN_HANDLE begin, uint32_t* offered_capabilities)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_offered_capabilities(BEGIN_HANDLE begin, uint32_t offered_capabilities)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE offered_capabilities_value = amqpvalue_create_symbol(offered_capabilities);
			if ((offered_capabilities_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 5, offered_capabilities_value) != 0))
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

	int begin_get_desired_capabilities(BEGIN_HANDLE begin, uint32_t* desired_capabilities)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_desired_capabilities(BEGIN_HANDLE begin, uint32_t desired_capabilities)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE desired_capabilities_value = amqpvalue_create_symbol(desired_capabilities);
			if ((desired_capabilities_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 6, desired_capabilities_value) != 0))
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

	int begin_get_properties(BEGIN_HANDLE begin, fields* properties)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			result = 0;
		}

		return result;
	}

	int begin_set_properties(BEGIN_HANDLE begin, fields properties)
	{
		int result;

		if (begin == NULL)
		{
			result = __LINE__;
		}
		else
		{
			BEGIN_INSTANCE* begin_instance = (BEGIN_INSTANCE*)begin;
			AMQP_VALUE properties_value = amqpvalue_create_fields(properties);
			if ((properties_value == NULL) ||
				(amqpvalue_set_list_item(begin_instance->list, 7, properties_value) != 0))
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


/* attach */

	typedef struct ATTACH_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} ATTACH_INSTANCE;

	ATTACH_HANDLE attach_create(const char* name, handle handle, role role)
	{
		ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)malloc(sizeof(ATTACH_INSTANCE));
		if (attach_instance != NULL)
		{
			attach_instance->list = amqpvalue_create_list(3);
			if (attach_instance->list == NULL)
			{
				free(attach_instance);
				attach_instance = NULL;
			}
			else
			{
				AMQP_VALUE name_value;
				AMQP_VALUE handle_value;
				AMQP_VALUE role_value;
				int result = 0;

				name_value = amqpvalue_create_string(name);
				if ((result == 0) && (amqpvalue_set_list_item(attach_instance->list, 0, name_value) != 0))
				{
					result = __LINE__;
				}
				handle_value = amqpvalue_create_handle(handle);
				if ((result == 0) && (amqpvalue_set_list_item(attach_instance->list, 1, handle_value) != 0))
				{
					result = __LINE__;
				}
				role_value = amqpvalue_create_role(role);
				if ((result == 0) && (amqpvalue_set_list_item(attach_instance->list, 2, role_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(name_value);
				amqpvalue_destroy(handle_value);
				amqpvalue_destroy(role_value);
			}
		}

		return attach_instance;
	}

	void attach_destroy(ATTACH_HANDLE attach)
	{
		if (attach != NULL)
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			amqpvalue_destroy(attach_instance->list);
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
			result = amqpvalue_clone(attach_instance->list);
		}

		return result;
	}

	int attach_get_name(ATTACH_HANDLE attach, const char** name)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_name(ATTACH_HANDLE attach, const char* name)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE name_value = amqpvalue_create_string(name);
			if ((name_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 0, name_value) != 0))
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

	int attach_get_handle(ATTACH_HANDLE attach, handle* handle)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_handle(ATTACH_HANDLE attach, handle handle)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE handle_value = amqpvalue_create_handle(handle);
			if ((handle_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 1, handle_value) != 0))
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

	int attach_get_role(ATTACH_HANDLE attach, role* role)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_role(ATTACH_HANDLE attach, role role)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE role_value = amqpvalue_create_role(role);
			if ((role_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 2, role_value) != 0))
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

	int attach_get_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode* snd_settle_mode)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode snd_settle_mode)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE snd_settle_mode_value = amqpvalue_create_sender_settle_mode(snd_settle_mode);
			if ((snd_settle_mode_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 3, snd_settle_mode_value) != 0))
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

	int attach_get_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode* rcv_settle_mode)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode rcv_settle_mode)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE rcv_settle_mode_value = amqpvalue_create_receiver_settle_mode(rcv_settle_mode);
			if ((rcv_settle_mode_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 4, rcv_settle_mode_value) != 0))
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

	int attach_get_source(ATTACH_HANDLE attach, AMQP_VALUE* source)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_source(ATTACH_HANDLE attach, AMQP_VALUE source)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE source_value = amqpvalue_clone(source);
			if ((source_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 5, source_value) != 0))
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

	int attach_get_target(ATTACH_HANDLE attach, AMQP_VALUE* target)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_target(ATTACH_HANDLE attach, AMQP_VALUE target)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE target_value = amqpvalue_clone(target);
			if ((target_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 6, target_value) != 0))
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

	int attach_get_unsettled(ATTACH_HANDLE attach, AMQP_VALUE* unsettled)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_unsettled(ATTACH_HANDLE attach, AMQP_VALUE unsettled)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE unsettled_value = amqpvalue_clone(unsettled);
			if ((unsettled_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 7, unsettled_value) != 0))
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

	int attach_get_incomplete_unsettled(ATTACH_HANDLE attach, bool* incomplete_unsettled)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_incomplete_unsettled(ATTACH_HANDLE attach, bool incomplete_unsettled)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE incomplete_unsettled_value = amqpvalue_create_boolean(incomplete_unsettled);
			if ((incomplete_unsettled_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 8, incomplete_unsettled_value) != 0))
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

	int attach_get_initial_delivery_count(ATTACH_HANDLE attach, sequence_no* initial_delivery_count)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_initial_delivery_count(ATTACH_HANDLE attach, sequence_no initial_delivery_count)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE initial_delivery_count_value = amqpvalue_create_sequence_no(initial_delivery_count);
			if ((initial_delivery_count_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 9, initial_delivery_count_value) != 0))
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

	int attach_get_max_message_size(ATTACH_HANDLE attach, uint64_t* max_message_size)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_max_message_size(ATTACH_HANDLE attach, uint64_t max_message_size)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE max_message_size_value = amqpvalue_create_ulong(max_message_size);
			if ((max_message_size_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 10, max_message_size_value) != 0))
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

	int attach_get_offered_capabilities(ATTACH_HANDLE attach, uint32_t* offered_capabilities)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_offered_capabilities(ATTACH_HANDLE attach, uint32_t offered_capabilities)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE offered_capabilities_value = amqpvalue_create_symbol(offered_capabilities);
			if ((offered_capabilities_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 11, offered_capabilities_value) != 0))
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

	int attach_get_desired_capabilities(ATTACH_HANDLE attach, uint32_t* desired_capabilities)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_desired_capabilities(ATTACH_HANDLE attach, uint32_t desired_capabilities)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE desired_capabilities_value = amqpvalue_create_symbol(desired_capabilities);
			if ((desired_capabilities_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 12, desired_capabilities_value) != 0))
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

	int attach_get_properties(ATTACH_HANDLE attach, fields* properties)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			result = 0;
		}

		return result;
	}

	int attach_set_properties(ATTACH_HANDLE attach, fields properties)
	{
		int result;

		if (attach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			ATTACH_INSTANCE* attach_instance = (ATTACH_INSTANCE*)attach;
			AMQP_VALUE properties_value = amqpvalue_create_fields(properties);
			if ((properties_value == NULL) ||
				(amqpvalue_set_list_item(attach_instance->list, 13, properties_value) != 0))
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


/* flow */

	typedef struct FLOW_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} FLOW_INSTANCE;

	FLOW_HANDLE flow_create(uint32_t incoming_window, transfer_number next_outgoing_id, uint32_t outgoing_window)
	{
		FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)malloc(sizeof(FLOW_INSTANCE));
		if (flow_instance != NULL)
		{
			flow_instance->list = amqpvalue_create_list(3);
			if (flow_instance->list == NULL)
			{
				free(flow_instance);
				flow_instance = NULL;
			}
			else
			{
				AMQP_VALUE incoming_window_value;
				AMQP_VALUE next_outgoing_id_value;
				AMQP_VALUE outgoing_window_value;
				int result = 0;

				incoming_window_value = amqpvalue_create_uint(incoming_window);
				if ((result == 0) && (amqpvalue_set_list_item(flow_instance->list, 0, incoming_window_value) != 0))
				{
					result = __LINE__;
				}
				next_outgoing_id_value = amqpvalue_create_transfer_number(next_outgoing_id);
				if ((result == 0) && (amqpvalue_set_list_item(flow_instance->list, 1, next_outgoing_id_value) != 0))
				{
					result = __LINE__;
				}
				outgoing_window_value = amqpvalue_create_uint(outgoing_window);
				if ((result == 0) && (amqpvalue_set_list_item(flow_instance->list, 2, outgoing_window_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(incoming_window_value);
				amqpvalue_destroy(next_outgoing_id_value);
				amqpvalue_destroy(outgoing_window_value);
			}
		}

		return flow_instance;
	}

	void flow_destroy(FLOW_HANDLE flow)
	{
		if (flow != NULL)
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			amqpvalue_destroy(flow_instance->list);
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
			result = amqpvalue_clone(flow_instance->list);
		}

		return result;
	}

	int flow_get_next_incoming_id(FLOW_HANDLE flow, transfer_number* next_incoming_id)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_next_incoming_id(FLOW_HANDLE flow, transfer_number next_incoming_id)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE next_incoming_id_value = amqpvalue_create_transfer_number(next_incoming_id);
			if ((next_incoming_id_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 0, next_incoming_id_value) != 0))
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

	int flow_get_incoming_window(FLOW_HANDLE flow, uint32_t* incoming_window)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_incoming_window(FLOW_HANDLE flow, uint32_t incoming_window)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE incoming_window_value = amqpvalue_create_uint(incoming_window);
			if ((incoming_window_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 1, incoming_window_value) != 0))
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

	int flow_get_next_outgoing_id(FLOW_HANDLE flow, transfer_number* next_outgoing_id)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_next_outgoing_id(FLOW_HANDLE flow, transfer_number next_outgoing_id)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE next_outgoing_id_value = amqpvalue_create_transfer_number(next_outgoing_id);
			if ((next_outgoing_id_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 2, next_outgoing_id_value) != 0))
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

	int flow_get_outgoing_window(FLOW_HANDLE flow, uint32_t* outgoing_window)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_outgoing_window(FLOW_HANDLE flow, uint32_t outgoing_window)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE outgoing_window_value = amqpvalue_create_uint(outgoing_window);
			if ((outgoing_window_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 3, outgoing_window_value) != 0))
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

	int flow_get_handle(FLOW_HANDLE flow, handle* handle)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_handle(FLOW_HANDLE flow, handle handle)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE handle_value = amqpvalue_create_handle(handle);
			if ((handle_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 4, handle_value) != 0))
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

	int flow_get_delivery_count(FLOW_HANDLE flow, sequence_no* delivery_count)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_delivery_count(FLOW_HANDLE flow, sequence_no delivery_count)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE delivery_count_value = amqpvalue_create_sequence_no(delivery_count);
			if ((delivery_count_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 5, delivery_count_value) != 0))
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

	int flow_get_link_credit(FLOW_HANDLE flow, uint32_t* link_credit)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_link_credit(FLOW_HANDLE flow, uint32_t link_credit)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE link_credit_value = amqpvalue_create_uint(link_credit);
			if ((link_credit_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 6, link_credit_value) != 0))
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

	int flow_get_available(FLOW_HANDLE flow, uint32_t* available)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_available(FLOW_HANDLE flow, uint32_t available)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE available_value = amqpvalue_create_uint(available);
			if ((available_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 7, available_value) != 0))
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

	int flow_get_drain(FLOW_HANDLE flow, bool* drain)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_drain(FLOW_HANDLE flow, bool drain)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE drain_value = amqpvalue_create_boolean(drain);
			if ((drain_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 8, drain_value) != 0))
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

	int flow_get_echo(FLOW_HANDLE flow, bool* echo)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_echo(FLOW_HANDLE flow, bool echo)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE echo_value = amqpvalue_create_boolean(echo);
			if ((echo_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 9, echo_value) != 0))
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

	int flow_get_properties(FLOW_HANDLE flow, fields* properties)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			result = 0;
		}

		return result;
	}

	int flow_set_properties(FLOW_HANDLE flow, fields properties)
	{
		int result;

		if (flow == NULL)
		{
			result = __LINE__;
		}
		else
		{
			FLOW_INSTANCE* flow_instance = (FLOW_INSTANCE*)flow;
			AMQP_VALUE properties_value = amqpvalue_create_fields(properties);
			if ((properties_value == NULL) ||
				(amqpvalue_set_list_item(flow_instance->list, 10, properties_value) != 0))
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


/* transfer */

	typedef struct TRANSFER_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} TRANSFER_INSTANCE;

	TRANSFER_HANDLE transfer_create(handle handle)
	{
		TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)malloc(sizeof(TRANSFER_INSTANCE));
		if (transfer_instance != NULL)
		{
			transfer_instance->list = amqpvalue_create_list(1);
			if (transfer_instance->list == NULL)
			{
				free(transfer_instance);
				transfer_instance = NULL;
			}
			else
			{
				AMQP_VALUE handle_value;
				int result = 0;

				handle_value = amqpvalue_create_handle(handle);
				if ((result == 0) && (amqpvalue_set_list_item(transfer_instance->list, 0, handle_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(handle_value);
			}
		}

		return transfer_instance;
	}

	void transfer_destroy(TRANSFER_HANDLE transfer)
	{
		if (transfer != NULL)
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			amqpvalue_destroy(transfer_instance->list);
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
			result = amqpvalue_clone(transfer_instance->list);
		}

		return result;
	}

	int transfer_get_handle(TRANSFER_HANDLE transfer, handle* handle)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_handle(TRANSFER_HANDLE transfer, handle handle)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE handle_value = amqpvalue_create_handle(handle);
			if ((handle_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 0, handle_value) != 0))
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

	int transfer_get_delivery_id(TRANSFER_HANDLE transfer, delivery_number* delivery_id)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_delivery_id(TRANSFER_HANDLE transfer, delivery_number delivery_id)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE delivery_id_value = amqpvalue_create_delivery_number(delivery_id);
			if ((delivery_id_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 1, delivery_id_value) != 0))
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

	int transfer_get_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag* delivery_tag)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag delivery_tag)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE delivery_tag_value = amqpvalue_create_delivery_tag(delivery_tag);
			if ((delivery_tag_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 2, delivery_tag_value) != 0))
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

	int transfer_get_message_format(TRANSFER_HANDLE transfer, message_format* message_format)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_message_format(TRANSFER_HANDLE transfer, message_format message_format)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE message_format_value = amqpvalue_create_message_format(message_format);
			if ((message_format_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 3, message_format_value) != 0))
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

	int transfer_get_settled(TRANSFER_HANDLE transfer, bool* settled)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_settled(TRANSFER_HANDLE transfer, bool settled)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE settled_value = amqpvalue_create_boolean(settled);
			if ((settled_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 4, settled_value) != 0))
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

	int transfer_get_more(TRANSFER_HANDLE transfer, bool* more)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_more(TRANSFER_HANDLE transfer, bool more)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE more_value = amqpvalue_create_boolean(more);
			if ((more_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 5, more_value) != 0))
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

	int transfer_get_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode* rcv_settle_mode)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode rcv_settle_mode)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE rcv_settle_mode_value = amqpvalue_create_receiver_settle_mode(rcv_settle_mode);
			if ((rcv_settle_mode_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 6, rcv_settle_mode_value) != 0))
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

	int transfer_get_state(TRANSFER_HANDLE transfer, AMQP_VALUE* state)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_state(TRANSFER_HANDLE transfer, AMQP_VALUE state)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE state_value = amqpvalue_clone(state);
			if ((state_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 7, state_value) != 0))
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

	int transfer_get_resume(TRANSFER_HANDLE transfer, bool* resume)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_resume(TRANSFER_HANDLE transfer, bool resume)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE resume_value = amqpvalue_create_boolean(resume);
			if ((resume_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 8, resume_value) != 0))
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

	int transfer_get_aborted(TRANSFER_HANDLE transfer, bool* aborted)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_aborted(TRANSFER_HANDLE transfer, bool aborted)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE aborted_value = amqpvalue_create_boolean(aborted);
			if ((aborted_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 9, aborted_value) != 0))
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

	int transfer_get_batchable(TRANSFER_HANDLE transfer, bool* batchable)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			result = 0;
		}

		return result;
	}

	int transfer_set_batchable(TRANSFER_HANDLE transfer, bool batchable)
	{
		int result;

		if (transfer == NULL)
		{
			result = __LINE__;
		}
		else
		{
			TRANSFER_INSTANCE* transfer_instance = (TRANSFER_INSTANCE*)transfer;
			AMQP_VALUE batchable_value = amqpvalue_create_boolean(batchable);
			if ((batchable_value == NULL) ||
				(amqpvalue_set_list_item(transfer_instance->list, 10, batchable_value) != 0))
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


/* disposition */

	typedef struct DISPOSITION_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} DISPOSITION_INSTANCE;

	DISPOSITION_HANDLE disposition_create(role role, delivery_number first)
	{
		DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)malloc(sizeof(DISPOSITION_INSTANCE));
		if (disposition_instance != NULL)
		{
			disposition_instance->list = amqpvalue_create_list(2);
			if (disposition_instance->list == NULL)
			{
				free(disposition_instance);
				disposition_instance = NULL;
			}
			else
			{
				AMQP_VALUE role_value;
				AMQP_VALUE first_value;
				int result = 0;

				role_value = amqpvalue_create_role(role);
				if ((result == 0) && (amqpvalue_set_list_item(disposition_instance->list, 0, role_value) != 0))
				{
					result = __LINE__;
				}
				first_value = amqpvalue_create_delivery_number(first);
				if ((result == 0) && (amqpvalue_set_list_item(disposition_instance->list, 1, first_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(role_value);
				amqpvalue_destroy(first_value);
			}
		}

		return disposition_instance;
	}

	void disposition_destroy(DISPOSITION_HANDLE disposition)
	{
		if (disposition != NULL)
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			amqpvalue_destroy(disposition_instance->list);
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
			result = amqpvalue_clone(disposition_instance->list);
		}

		return result;
	}

	int disposition_get_role(DISPOSITION_HANDLE disposition, role* role)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			result = 0;
		}

		return result;
	}

	int disposition_set_role(DISPOSITION_HANDLE disposition, role role)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			AMQP_VALUE role_value = amqpvalue_create_role(role);
			if ((role_value == NULL) ||
				(amqpvalue_set_list_item(disposition_instance->list, 0, role_value) != 0))
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

	int disposition_get_first(DISPOSITION_HANDLE disposition, delivery_number* first)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			result = 0;
		}

		return result;
	}

	int disposition_set_first(DISPOSITION_HANDLE disposition, delivery_number first)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			AMQP_VALUE first_value = amqpvalue_create_delivery_number(first);
			if ((first_value == NULL) ||
				(amqpvalue_set_list_item(disposition_instance->list, 1, first_value) != 0))
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

	int disposition_get_last(DISPOSITION_HANDLE disposition, delivery_number* last)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			result = 0;
		}

		return result;
	}

	int disposition_set_last(DISPOSITION_HANDLE disposition, delivery_number last)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			AMQP_VALUE last_value = amqpvalue_create_delivery_number(last);
			if ((last_value == NULL) ||
				(amqpvalue_set_list_item(disposition_instance->list, 2, last_value) != 0))
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

	int disposition_get_settled(DISPOSITION_HANDLE disposition, bool* settled)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			result = 0;
		}

		return result;
	}

	int disposition_set_settled(DISPOSITION_HANDLE disposition, bool settled)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			AMQP_VALUE settled_value = amqpvalue_create_boolean(settled);
			if ((settled_value == NULL) ||
				(amqpvalue_set_list_item(disposition_instance->list, 3, settled_value) != 0))
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

	int disposition_get_state(DISPOSITION_HANDLE disposition, AMQP_VALUE* state)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			result = 0;
		}

		return result;
	}

	int disposition_set_state(DISPOSITION_HANDLE disposition, AMQP_VALUE state)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			AMQP_VALUE state_value = amqpvalue_clone(state);
			if ((state_value == NULL) ||
				(amqpvalue_set_list_item(disposition_instance->list, 4, state_value) != 0))
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

	int disposition_get_batchable(DISPOSITION_HANDLE disposition, bool* batchable)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			result = 0;
		}

		return result;
	}

	int disposition_set_batchable(DISPOSITION_HANDLE disposition, bool batchable)
	{
		int result;

		if (disposition == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DISPOSITION_INSTANCE* disposition_instance = (DISPOSITION_INSTANCE*)disposition;
			AMQP_VALUE batchable_value = amqpvalue_create_boolean(batchable);
			if ((batchable_value == NULL) ||
				(amqpvalue_set_list_item(disposition_instance->list, 5, batchable_value) != 0))
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


/* detach */

	typedef struct DETACH_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} DETACH_INSTANCE;

	DETACH_HANDLE detach_create(handle handle)
	{
		DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)malloc(sizeof(DETACH_INSTANCE));
		if (detach_instance != NULL)
		{
			detach_instance->list = amqpvalue_create_list(1);
			if (detach_instance->list == NULL)
			{
				free(detach_instance);
				detach_instance = NULL;
			}
			else
			{
				AMQP_VALUE handle_value;
				int result = 0;

				handle_value = amqpvalue_create_handle(handle);
				if ((result == 0) && (amqpvalue_set_list_item(detach_instance->list, 0, handle_value) != 0))
				{
					result = __LINE__;
				}

				amqpvalue_destroy(handle_value);
			}
		}

		return detach_instance;
	}

	void detach_destroy(DETACH_HANDLE detach)
	{
		if (detach != NULL)
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			amqpvalue_destroy(detach_instance->list);
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
			result = amqpvalue_clone(detach_instance->list);
		}

		return result;
	}

	int detach_get_handle(DETACH_HANDLE detach, handle* handle)
	{
		int result;

		if (detach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			result = 0;
		}

		return result;
	}

	int detach_set_handle(DETACH_HANDLE detach, handle handle)
	{
		int result;

		if (detach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			AMQP_VALUE handle_value = amqpvalue_create_handle(handle);
			if ((handle_value == NULL) ||
				(amqpvalue_set_list_item(detach_instance->list, 0, handle_value) != 0))
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

	int detach_get_closed(DETACH_HANDLE detach, bool* closed)
	{
		int result;

		if (detach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			result = 0;
		}

		return result;
	}

	int detach_set_closed(DETACH_HANDLE detach, bool closed)
	{
		int result;

		if (detach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			AMQP_VALUE closed_value = amqpvalue_create_boolean(closed);
			if ((closed_value == NULL) ||
				(amqpvalue_set_list_item(detach_instance->list, 1, closed_value) != 0))
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

	int detach_get_error(DETACH_HANDLE detach, ERROR_HANDLE* error)
	{
		int result;

		if (detach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			result = 0;
		}

		return result;
	}

	int detach_set_error(DETACH_HANDLE detach, ERROR_HANDLE error)
	{
		int result;

		if (detach == NULL)
		{
			result = __LINE__;
		}
		else
		{
			DETACH_INSTANCE* detach_instance = (DETACH_INSTANCE*)detach;
			AMQP_VALUE error_value = amqpvalue_create_error(error);
			if ((error_value == NULL) ||
				(amqpvalue_set_list_item(detach_instance->list, 2, error_value) != 0))
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


/* end */

	typedef struct END_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} END_INSTANCE;

	END_HANDLE end_create(void)
	{
		END_INSTANCE* end_instance = (END_INSTANCE*)malloc(sizeof(END_INSTANCE));
		if (end_instance != NULL)
		{
			end_instance->list = amqpvalue_create_list(0);
			if (end_instance->list == NULL)
			{
				free(end_instance);
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
			amqpvalue_destroy(end_instance->list);
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
			result = amqpvalue_clone(end_instance->list);
		}

		return result;
	}

	int end_get_error(END_HANDLE end, ERROR_HANDLE* error)
	{
		int result;

		if (end == NULL)
		{
			result = __LINE__;
		}
		else
		{
			END_INSTANCE* end_instance = (END_INSTANCE*)end;
			result = 0;
		}

		return result;
	}

	int end_set_error(END_HANDLE end, ERROR_HANDLE error)
	{
		int result;

		if (end == NULL)
		{
			result = __LINE__;
		}
		else
		{
			END_INSTANCE* end_instance = (END_INSTANCE*)end;
			AMQP_VALUE error_value = amqpvalue_create_error(error);
			if ((error_value == NULL) ||
				(amqpvalue_set_list_item(end_instance->list, 0, error_value) != 0))
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


/* close */

	typedef struct CLOSE_INSTANCE_TAG
	{
		AMQP_VALUE list;
	} CLOSE_INSTANCE;

	CLOSE_HANDLE close_create(void)
	{
		CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)malloc(sizeof(CLOSE_INSTANCE));
		if (close_instance != NULL)
		{
			close_instance->list = amqpvalue_create_list(0);
			if (close_instance->list == NULL)
			{
				free(close_instance);
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
			amqpvalue_destroy(close_instance->list);
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
			result = amqpvalue_clone(close_instance->list);
		}

		return result;
	}

	int close_get_error(CLOSE_HANDLE close, ERROR_HANDLE* error)
	{
		int result;

		if (close == NULL)
		{
			result = __LINE__;
		}
		else
		{
			CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close;
			result = 0;
		}

		return result;
	}

	int close_set_error(CLOSE_HANDLE close, ERROR_HANDLE error)
	{
		int result;

		if (close == NULL)
		{
			result = __LINE__;
		}
		else
		{
			CLOSE_INSTANCE* close_instance = (CLOSE_INSTANCE*)close;
			AMQP_VALUE error_value = amqpvalue_create_error(error);
			if ((error_value == NULL) ||
				(amqpvalue_set_list_item(close_instance->list, 0, error_value) != 0))
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


