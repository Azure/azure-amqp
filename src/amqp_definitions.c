

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

	int error_get_condition(ERROR_HANDLE error, uint32_t* condition)
	{
		return __LINE__;
	}

	int error_set_condition(ERROR_HANDLE error, uint32_t condition)
	{
		return __LINE__;
	}

	int error_get_description(ERROR_HANDLE error, const char** description)
	{
		return __LINE__;
	}

	int error_set_description(ERROR_HANDLE error, const char* description)
	{
		return __LINE__;
	}

	int error_get_info(ERROR_HANDLE error, fields* info)
	{
		return __LINE__;
	}

	int error_set_info(ERROR_HANDLE error, fields info)
	{
		return __LINE__;
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

	int open_get_container_id(OPEN_HANDLE open, const char** container_id)
	{
		return __LINE__;
	}

	int open_set_container_id(OPEN_HANDLE open, const char* container_id)
	{
		return __LINE__;
	}

	int open_get_hostname(OPEN_HANDLE open, const char** hostname)
	{
		return __LINE__;
	}

	int open_set_hostname(OPEN_HANDLE open, const char* hostname)
	{
		return __LINE__;
	}

	int open_get_max_frame_size(OPEN_HANDLE open, uint32_t* max_frame_size)
	{
		return __LINE__;
	}

	int open_set_max_frame_size(OPEN_HANDLE open, uint32_t max_frame_size)
	{
		return __LINE__;
	}

	int open_get_channel_max(OPEN_HANDLE open, uint16_t* channel_max)
	{
		return __LINE__;
	}

	int open_set_channel_max(OPEN_HANDLE open, uint16_t channel_max)
	{
		return __LINE__;
	}

	int open_get_idle_time_out(OPEN_HANDLE open, milliseconds* idle_time_out)
	{
		return __LINE__;
	}

	int open_set_idle_time_out(OPEN_HANDLE open, milliseconds idle_time_out)
	{
		return __LINE__;
	}

	int open_get_outgoing_locales(OPEN_HANDLE open, ietf_language_tag* outgoing_locales)
	{
		return __LINE__;
	}

	int open_set_outgoing_locales(OPEN_HANDLE open, ietf_language_tag outgoing_locales)
	{
		return __LINE__;
	}

	int open_get_incoming_locales(OPEN_HANDLE open, ietf_language_tag* incoming_locales)
	{
		return __LINE__;
	}

	int open_set_incoming_locales(OPEN_HANDLE open, ietf_language_tag incoming_locales)
	{
		return __LINE__;
	}

	int open_get_offered_capabilities(OPEN_HANDLE open, uint32_t* offered_capabilities)
	{
		return __LINE__;
	}

	int open_set_offered_capabilities(OPEN_HANDLE open, uint32_t offered_capabilities)
	{
		return __LINE__;
	}

	int open_get_desired_capabilities(OPEN_HANDLE open, uint32_t* desired_capabilities)
	{
		return __LINE__;
	}

	int open_set_desired_capabilities(OPEN_HANDLE open, uint32_t desired_capabilities)
	{
		return __LINE__;
	}

	int open_get_properties(OPEN_HANDLE open, fields* properties)
	{
		return __LINE__;
	}

	int open_set_properties(OPEN_HANDLE open, fields properties)
	{
		return __LINE__;
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

	int begin_get_remote_channel(BEGIN_HANDLE begin, uint16_t* remote_channel)
	{
		return __LINE__;
	}

	int begin_set_remote_channel(BEGIN_HANDLE begin, uint16_t remote_channel)
	{
		return __LINE__;
	}

	int begin_get_next_outgoing_id(BEGIN_HANDLE begin, transfer_number* next_outgoing_id)
	{
		return __LINE__;
	}

	int begin_set_next_outgoing_id(BEGIN_HANDLE begin, transfer_number next_outgoing_id)
	{
		return __LINE__;
	}

	int begin_get_incoming_window(BEGIN_HANDLE begin, uint32_t* incoming_window)
	{
		return __LINE__;
	}

	int begin_set_incoming_window(BEGIN_HANDLE begin, uint32_t incoming_window)
	{
		return __LINE__;
	}

	int begin_get_outgoing_window(BEGIN_HANDLE begin, uint32_t* outgoing_window)
	{
		return __LINE__;
	}

	int begin_set_outgoing_window(BEGIN_HANDLE begin, uint32_t outgoing_window)
	{
		return __LINE__;
	}

	int begin_get_handle_max(BEGIN_HANDLE begin, handle* handle_max)
	{
		return __LINE__;
	}

	int begin_set_handle_max(BEGIN_HANDLE begin, handle handle_max)
	{
		return __LINE__;
	}

	int begin_get_offered_capabilities(BEGIN_HANDLE begin, uint32_t* offered_capabilities)
	{
		return __LINE__;
	}

	int begin_set_offered_capabilities(BEGIN_HANDLE begin, uint32_t offered_capabilities)
	{
		return __LINE__;
	}

	int begin_get_desired_capabilities(BEGIN_HANDLE begin, uint32_t* desired_capabilities)
	{
		return __LINE__;
	}

	int begin_set_desired_capabilities(BEGIN_HANDLE begin, uint32_t desired_capabilities)
	{
		return __LINE__;
	}

	int begin_get_properties(BEGIN_HANDLE begin, fields* properties)
	{
		return __LINE__;
	}

	int begin_set_properties(BEGIN_HANDLE begin, fields properties)
	{
		return __LINE__;
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

	int attach_get_name(ATTACH_HANDLE attach, const char** name)
	{
		return __LINE__;
	}

	int attach_set_name(ATTACH_HANDLE attach, const char* name)
	{
		return __LINE__;
	}

	int attach_get_handle(ATTACH_HANDLE attach, handle* handle)
	{
		return __LINE__;
	}

	int attach_set_handle(ATTACH_HANDLE attach, handle handle)
	{
		return __LINE__;
	}

	int attach_get_role(ATTACH_HANDLE attach, role* role)
	{
		return __LINE__;
	}

	int attach_set_role(ATTACH_HANDLE attach, role role)
	{
		return __LINE__;
	}

	int attach_get_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode* snd_settle_mode)
	{
		return __LINE__;
	}

	int attach_set_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode snd_settle_mode)
	{
		return __LINE__;
	}

	int attach_get_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode* rcv_settle_mode)
	{
		return __LINE__;
	}

	int attach_set_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode rcv_settle_mode)
	{
		return __LINE__;
	}

	int attach_get_source(ATTACH_HANDLE attach, AMQP_VALUE* source)
	{
		return __LINE__;
	}

	int attach_set_source(ATTACH_HANDLE attach, AMQP_VALUE source)
	{
		return __LINE__;
	}

	int attach_get_target(ATTACH_HANDLE attach, AMQP_VALUE* target)
	{
		return __LINE__;
	}

	int attach_set_target(ATTACH_HANDLE attach, AMQP_VALUE target)
	{
		return __LINE__;
	}

	int attach_get_unsettled(ATTACH_HANDLE attach, AMQP_VALUE* unsettled)
	{
		return __LINE__;
	}

	int attach_set_unsettled(ATTACH_HANDLE attach, AMQP_VALUE unsettled)
	{
		return __LINE__;
	}

	int attach_get_incomplete_unsettled(ATTACH_HANDLE attach, bool* incomplete_unsettled)
	{
		return __LINE__;
	}

	int attach_set_incomplete_unsettled(ATTACH_HANDLE attach, bool incomplete_unsettled)
	{
		return __LINE__;
	}

	int attach_get_initial_delivery_count(ATTACH_HANDLE attach, sequence_no* initial_delivery_count)
	{
		return __LINE__;
	}

	int attach_set_initial_delivery_count(ATTACH_HANDLE attach, sequence_no initial_delivery_count)
	{
		return __LINE__;
	}

	int attach_get_max_message_size(ATTACH_HANDLE attach, uint64_t* max_message_size)
	{
		return __LINE__;
	}

	int attach_set_max_message_size(ATTACH_HANDLE attach, uint64_t max_message_size)
	{
		return __LINE__;
	}

	int attach_get_offered_capabilities(ATTACH_HANDLE attach, uint32_t* offered_capabilities)
	{
		return __LINE__;
	}

	int attach_set_offered_capabilities(ATTACH_HANDLE attach, uint32_t offered_capabilities)
	{
		return __LINE__;
	}

	int attach_get_desired_capabilities(ATTACH_HANDLE attach, uint32_t* desired_capabilities)
	{
		return __LINE__;
	}

	int attach_set_desired_capabilities(ATTACH_HANDLE attach, uint32_t desired_capabilities)
	{
		return __LINE__;
	}

	int attach_get_properties(ATTACH_HANDLE attach, fields* properties)
	{
		return __LINE__;
	}

	int attach_set_properties(ATTACH_HANDLE attach, fields properties)
	{
		return __LINE__;
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

	int flow_get_next_incoming_id(FLOW_HANDLE flow, transfer_number* next_incoming_id)
	{
		return __LINE__;
	}

	int flow_set_next_incoming_id(FLOW_HANDLE flow, transfer_number next_incoming_id)
	{
		return __LINE__;
	}

	int flow_get_incoming_window(FLOW_HANDLE flow, uint32_t* incoming_window)
	{
		return __LINE__;
	}

	int flow_set_incoming_window(FLOW_HANDLE flow, uint32_t incoming_window)
	{
		return __LINE__;
	}

	int flow_get_next_outgoing_id(FLOW_HANDLE flow, transfer_number* next_outgoing_id)
	{
		return __LINE__;
	}

	int flow_set_next_outgoing_id(FLOW_HANDLE flow, transfer_number next_outgoing_id)
	{
		return __LINE__;
	}

	int flow_get_outgoing_window(FLOW_HANDLE flow, uint32_t* outgoing_window)
	{
		return __LINE__;
	}

	int flow_set_outgoing_window(FLOW_HANDLE flow, uint32_t outgoing_window)
	{
		return __LINE__;
	}

	int flow_get_handle(FLOW_HANDLE flow, handle* handle)
	{
		return __LINE__;
	}

	int flow_set_handle(FLOW_HANDLE flow, handle handle)
	{
		return __LINE__;
	}

	int flow_get_delivery_count(FLOW_HANDLE flow, sequence_no* delivery_count)
	{
		return __LINE__;
	}

	int flow_set_delivery_count(FLOW_HANDLE flow, sequence_no delivery_count)
	{
		return __LINE__;
	}

	int flow_get_link_credit(FLOW_HANDLE flow, uint32_t* link_credit)
	{
		return __LINE__;
	}

	int flow_set_link_credit(FLOW_HANDLE flow, uint32_t link_credit)
	{
		return __LINE__;
	}

	int flow_get_available(FLOW_HANDLE flow, uint32_t* available)
	{
		return __LINE__;
	}

	int flow_set_available(FLOW_HANDLE flow, uint32_t available)
	{
		return __LINE__;
	}

	int flow_get_drain(FLOW_HANDLE flow, bool* drain)
	{
		return __LINE__;
	}

	int flow_set_drain(FLOW_HANDLE flow, bool drain)
	{
		return __LINE__;
	}

	int flow_get_echo(FLOW_HANDLE flow, bool* echo)
	{
		return __LINE__;
	}

	int flow_set_echo(FLOW_HANDLE flow, bool echo)
	{
		return __LINE__;
	}

	int flow_get_properties(FLOW_HANDLE flow, fields* properties)
	{
		return __LINE__;
	}

	int flow_set_properties(FLOW_HANDLE flow, fields properties)
	{
		return __LINE__;
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

	int transfer_get_handle(TRANSFER_HANDLE transfer, handle* handle)
	{
		return __LINE__;
	}

	int transfer_set_handle(TRANSFER_HANDLE transfer, handle handle)
	{
		return __LINE__;
	}

	int transfer_get_delivery_id(TRANSFER_HANDLE transfer, delivery_number* delivery_id)
	{
		return __LINE__;
	}

	int transfer_set_delivery_id(TRANSFER_HANDLE transfer, delivery_number delivery_id)
	{
		return __LINE__;
	}

	int transfer_get_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag* delivery_tag)
	{
		return __LINE__;
	}

	int transfer_set_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag delivery_tag)
	{
		return __LINE__;
	}

	int transfer_get_message_format(TRANSFER_HANDLE transfer, message_format* message_format)
	{
		return __LINE__;
	}

	int transfer_set_message_format(TRANSFER_HANDLE transfer, message_format message_format)
	{
		return __LINE__;
	}

	int transfer_get_settled(TRANSFER_HANDLE transfer, bool* settled)
	{
		return __LINE__;
	}

	int transfer_set_settled(TRANSFER_HANDLE transfer, bool settled)
	{
		return __LINE__;
	}

	int transfer_get_more(TRANSFER_HANDLE transfer, bool* more)
	{
		return __LINE__;
	}

	int transfer_set_more(TRANSFER_HANDLE transfer, bool more)
	{
		return __LINE__;
	}

	int transfer_get_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode* rcv_settle_mode)
	{
		return __LINE__;
	}

	int transfer_set_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode rcv_settle_mode)
	{
		return __LINE__;
	}

	int transfer_get_state(TRANSFER_HANDLE transfer, AMQP_VALUE* state)
	{
		return __LINE__;
	}

	int transfer_set_state(TRANSFER_HANDLE transfer, AMQP_VALUE state)
	{
		return __LINE__;
	}

	int transfer_get_resume(TRANSFER_HANDLE transfer, bool* resume)
	{
		return __LINE__;
	}

	int transfer_set_resume(TRANSFER_HANDLE transfer, bool resume)
	{
		return __LINE__;
	}

	int transfer_get_aborted(TRANSFER_HANDLE transfer, bool* aborted)
	{
		return __LINE__;
	}

	int transfer_set_aborted(TRANSFER_HANDLE transfer, bool aborted)
	{
		return __LINE__;
	}

	int transfer_get_batchable(TRANSFER_HANDLE transfer, bool* batchable)
	{
		return __LINE__;
	}

	int transfer_set_batchable(TRANSFER_HANDLE transfer, bool batchable)
	{
		return __LINE__;
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

	int disposition_get_role(DISPOSITION_HANDLE disposition, role* role)
	{
		return __LINE__;
	}

	int disposition_set_role(DISPOSITION_HANDLE disposition, role role)
	{
		return __LINE__;
	}

	int disposition_get_first(DISPOSITION_HANDLE disposition, delivery_number* first)
	{
		return __LINE__;
	}

	int disposition_set_first(DISPOSITION_HANDLE disposition, delivery_number first)
	{
		return __LINE__;
	}

	int disposition_get_last(DISPOSITION_HANDLE disposition, delivery_number* last)
	{
		return __LINE__;
	}

	int disposition_set_last(DISPOSITION_HANDLE disposition, delivery_number last)
	{
		return __LINE__;
	}

	int disposition_get_settled(DISPOSITION_HANDLE disposition, bool* settled)
	{
		return __LINE__;
	}

	int disposition_set_settled(DISPOSITION_HANDLE disposition, bool settled)
	{
		return __LINE__;
	}

	int disposition_get_state(DISPOSITION_HANDLE disposition, AMQP_VALUE* state)
	{
		return __LINE__;
	}

	int disposition_set_state(DISPOSITION_HANDLE disposition, AMQP_VALUE state)
	{
		return __LINE__;
	}

	int disposition_get_batchable(DISPOSITION_HANDLE disposition, bool* batchable)
	{
		return __LINE__;
	}

	int disposition_set_batchable(DISPOSITION_HANDLE disposition, bool batchable)
	{
		return __LINE__;
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

	int detach_get_handle(DETACH_HANDLE detach, handle* handle)
	{
		return __LINE__;
	}

	int detach_set_handle(DETACH_HANDLE detach, handle handle)
	{
		return __LINE__;
	}

	int detach_get_closed(DETACH_HANDLE detach, bool* closed)
	{
		return __LINE__;
	}

	int detach_set_closed(DETACH_HANDLE detach, bool closed)
	{
		return __LINE__;
	}

	int detach_get_error(DETACH_HANDLE detach, ERROR_HANDLE* error)
	{
		return __LINE__;
	}

	int detach_set_error(DETACH_HANDLE detach, ERROR_HANDLE error)
	{
		return __LINE__;
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

	int end_get_error(END_HANDLE end, ERROR_HANDLE* error)
	{
		return __LINE__;
	}

	int end_set_error(END_HANDLE end, ERROR_HANDLE error)
	{
		return __LINE__;
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

	int close_get_error(CLOSE_HANDLE close, ERROR_HANDLE* error)
	{
		return __LINE__;
	}

	int close_set_error(CLOSE_HANDLE close, ERROR_HANDLE error)
	{
		return __LINE__;
	}


