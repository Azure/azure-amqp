

#include "amqpvalue.h"
#include "amqp_definitions.h"
#include <stdlib.h>

/* error */

	ERROR_HANDLE error_create(symbol condition)
	{
		return NULL;
	}

	void error_destroy(ERROR_HANDLE error)
	{
	}

	int error_get_condition(ERROR_HANDLE error, uint32_t* condition)
	{
		return __LINE__;
	}

	int error_set_condition(ERROR_HANDLE error, uint32_t condition)
	{
		return __LINE__;
	}
	int error_get_description(ERROR_HANDLE error, uint32_t* description)
	{
		return __LINE__;
	}

	int error_set_description(ERROR_HANDLE error, uint32_t description)
	{
		return __LINE__;
	}
	int error_get_info(ERROR_HANDLE error, uint32_t* info)
	{
		return __LINE__;
	}

	int error_set_info(ERROR_HANDLE error, uint32_t info)
	{
		return __LINE__;
	}

/* open */

	OPEN_HANDLE open_create(const char* container_id)
	{
		return NULL;
	}

	void open_destroy(OPEN_HANDLE open)
	{
	}

	int open_get_container_id(OPEN_HANDLE open, uint32_t* container_id)
	{
		return __LINE__;
	}

	int open_set_container_id(OPEN_HANDLE open, uint32_t container_id)
	{
		return __LINE__;
	}
	int open_get_hostname(OPEN_HANDLE open, uint32_t* hostname)
	{
		return __LINE__;
	}

	int open_set_hostname(OPEN_HANDLE open, uint32_t hostname)
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
	int open_get_channel_max(OPEN_HANDLE open, uint32_t* channel_max)
	{
		return __LINE__;
	}

	int open_set_channel_max(OPEN_HANDLE open, uint32_t channel_max)
	{
		return __LINE__;
	}
	int open_get_idle_time_out(OPEN_HANDLE open, uint32_t* idle_time_out)
	{
		return __LINE__;
	}

	int open_set_idle_time_out(OPEN_HANDLE open, uint32_t idle_time_out)
	{
		return __LINE__;
	}
	int open_get_outgoing_locales(OPEN_HANDLE open, uint32_t* outgoing_locales)
	{
		return __LINE__;
	}

	int open_set_outgoing_locales(OPEN_HANDLE open, uint32_t outgoing_locales)
	{
		return __LINE__;
	}
	int open_get_incoming_locales(OPEN_HANDLE open, uint32_t* incoming_locales)
	{
		return __LINE__;
	}

	int open_set_incoming_locales(OPEN_HANDLE open, uint32_t incoming_locales)
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
	int open_get_properties(OPEN_HANDLE open, uint32_t* properties)
	{
		return __LINE__;
	}

	int open_set_properties(OPEN_HANDLE open, uint32_t properties)
	{
		return __LINE__;
	}

/* begin */

	BEGIN_HANDLE begin_create(transfer_number next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window)
	{
		return NULL;
	}

	void begin_destroy(BEGIN_HANDLE begin)
	{
	}

	int begin_get_remote_channel(BEGIN_HANDLE begin, uint32_t* remote_channel)
	{
		return __LINE__;
	}

	int begin_set_remote_channel(BEGIN_HANDLE begin, uint32_t remote_channel)
	{
		return __LINE__;
	}
	int begin_get_next_outgoing_id(BEGIN_HANDLE begin, uint32_t* next_outgoing_id)
	{
		return __LINE__;
	}

	int begin_set_next_outgoing_id(BEGIN_HANDLE begin, uint32_t next_outgoing_id)
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
	int begin_get_handle_max(BEGIN_HANDLE begin, uint32_t* handle_max)
	{
		return __LINE__;
	}

	int begin_set_handle_max(BEGIN_HANDLE begin, uint32_t handle_max)
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
	int begin_get_properties(BEGIN_HANDLE begin, uint32_t* properties)
	{
		return __LINE__;
	}

	int begin_set_properties(BEGIN_HANDLE begin, uint32_t properties)
	{
		return __LINE__;
	}

/* attach */

	ATTACH_HANDLE attach_create(const char* name, handle handle, role role)
	{
		return NULL;
	}

	void attach_destroy(ATTACH_HANDLE attach)
	{
	}

	int attach_get_name(ATTACH_HANDLE attach, uint32_t* name)
	{
		return __LINE__;
	}

	int attach_set_name(ATTACH_HANDLE attach, uint32_t name)
	{
		return __LINE__;
	}
	int attach_get_handle(ATTACH_HANDLE attach, uint32_t* handle)
	{
		return __LINE__;
	}

	int attach_set_handle(ATTACH_HANDLE attach, uint32_t handle)
	{
		return __LINE__;
	}
	int attach_get_role(ATTACH_HANDLE attach, uint32_t* role)
	{
		return __LINE__;
	}

	int attach_set_role(ATTACH_HANDLE attach, uint32_t role)
	{
		return __LINE__;
	}
	int attach_get_snd_settle_mode(ATTACH_HANDLE attach, uint32_t* snd_settle_mode)
	{
		return __LINE__;
	}

	int attach_set_snd_settle_mode(ATTACH_HANDLE attach, uint32_t snd_settle_mode)
	{
		return __LINE__;
	}
	int attach_get_rcv_settle_mode(ATTACH_HANDLE attach, uint32_t* rcv_settle_mode)
	{
		return __LINE__;
	}

	int attach_set_rcv_settle_mode(ATTACH_HANDLE attach, uint32_t rcv_settle_mode)
	{
		return __LINE__;
	}
	int attach_get_source(ATTACH_HANDLE attach, uint32_t* source)
	{
		return __LINE__;
	}

	int attach_set_source(ATTACH_HANDLE attach, uint32_t source)
	{
		return __LINE__;
	}
	int attach_get_target(ATTACH_HANDLE attach, uint32_t* target)
	{
		return __LINE__;
	}

	int attach_set_target(ATTACH_HANDLE attach, uint32_t target)
	{
		return __LINE__;
	}
	int attach_get_unsettled(ATTACH_HANDLE attach, uint32_t* unsettled)
	{
		return __LINE__;
	}

	int attach_set_unsettled(ATTACH_HANDLE attach, uint32_t unsettled)
	{
		return __LINE__;
	}
	int attach_get_incomplete_unsettled(ATTACH_HANDLE attach, uint32_t* incomplete_unsettled)
	{
		return __LINE__;
	}

	int attach_set_incomplete_unsettled(ATTACH_HANDLE attach, uint32_t incomplete_unsettled)
	{
		return __LINE__;
	}
	int attach_get_initial_delivery_count(ATTACH_HANDLE attach, uint32_t* initial_delivery_count)
	{
		return __LINE__;
	}

	int attach_set_initial_delivery_count(ATTACH_HANDLE attach, uint32_t initial_delivery_count)
	{
		return __LINE__;
	}
	int attach_get_max_message_size(ATTACH_HANDLE attach, uint32_t* max_message_size)
	{
		return __LINE__;
	}

	int attach_set_max_message_size(ATTACH_HANDLE attach, uint32_t max_message_size)
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
	int attach_get_properties(ATTACH_HANDLE attach, uint32_t* properties)
	{
		return __LINE__;
	}

	int attach_set_properties(ATTACH_HANDLE attach, uint32_t properties)
	{
		return __LINE__;
	}

/* flow */

	FLOW_HANDLE flow_create(uint32_t incoming_window, transfer_number next_outgoing_id, uint32_t outgoing_window)
	{
		return NULL;
	}

	void flow_destroy(FLOW_HANDLE flow)
	{
	}

	int flow_get_next_incoming_id(FLOW_HANDLE flow, uint32_t* next_incoming_id)
	{
		return __LINE__;
	}

	int flow_set_next_incoming_id(FLOW_HANDLE flow, uint32_t next_incoming_id)
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
	int flow_get_next_outgoing_id(FLOW_HANDLE flow, uint32_t* next_outgoing_id)
	{
		return __LINE__;
	}

	int flow_set_next_outgoing_id(FLOW_HANDLE flow, uint32_t next_outgoing_id)
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
	int flow_get_handle(FLOW_HANDLE flow, uint32_t* handle)
	{
		return __LINE__;
	}

	int flow_set_handle(FLOW_HANDLE flow, uint32_t handle)
	{
		return __LINE__;
	}
	int flow_get_delivery_count(FLOW_HANDLE flow, uint32_t* delivery_count)
	{
		return __LINE__;
	}

	int flow_set_delivery_count(FLOW_HANDLE flow, uint32_t delivery_count)
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
	int flow_get_drain(FLOW_HANDLE flow, uint32_t* drain)
	{
		return __LINE__;
	}

	int flow_set_drain(FLOW_HANDLE flow, uint32_t drain)
	{
		return __LINE__;
	}
	int flow_get_echo(FLOW_HANDLE flow, uint32_t* echo)
	{
		return __LINE__;
	}

	int flow_set_echo(FLOW_HANDLE flow, uint32_t echo)
	{
		return __LINE__;
	}
	int flow_get_properties(FLOW_HANDLE flow, uint32_t* properties)
	{
		return __LINE__;
	}

	int flow_set_properties(FLOW_HANDLE flow, uint32_t properties)
	{
		return __LINE__;
	}

/* transfer */

	TRANSFER_HANDLE transfer_create(handle handle)
	{
		return NULL;
	}

	void transfer_destroy(TRANSFER_HANDLE transfer)
	{
	}

	int transfer_get_handle(TRANSFER_HANDLE transfer, uint32_t* handle)
	{
		return __LINE__;
	}

	int transfer_set_handle(TRANSFER_HANDLE transfer, uint32_t handle)
	{
		return __LINE__;
	}
	int transfer_get_delivery_id(TRANSFER_HANDLE transfer, uint32_t* delivery_id)
	{
		return __LINE__;
	}

	int transfer_set_delivery_id(TRANSFER_HANDLE transfer, uint32_t delivery_id)
	{
		return __LINE__;
	}
	int transfer_get_delivery_tag(TRANSFER_HANDLE transfer, uint32_t* delivery_tag)
	{
		return __LINE__;
	}

	int transfer_set_delivery_tag(TRANSFER_HANDLE transfer, uint32_t delivery_tag)
	{
		return __LINE__;
	}
	int transfer_get_message_format(TRANSFER_HANDLE transfer, uint32_t* message_format)
	{
		return __LINE__;
	}

	int transfer_set_message_format(TRANSFER_HANDLE transfer, uint32_t message_format)
	{
		return __LINE__;
	}
	int transfer_get_settled(TRANSFER_HANDLE transfer, uint32_t* settled)
	{
		return __LINE__;
	}

	int transfer_set_settled(TRANSFER_HANDLE transfer, uint32_t settled)
	{
		return __LINE__;
	}
	int transfer_get_more(TRANSFER_HANDLE transfer, uint32_t* more)
	{
		return __LINE__;
	}

	int transfer_set_more(TRANSFER_HANDLE transfer, uint32_t more)
	{
		return __LINE__;
	}
	int transfer_get_rcv_settle_mode(TRANSFER_HANDLE transfer, uint32_t* rcv_settle_mode)
	{
		return __LINE__;
	}

	int transfer_set_rcv_settle_mode(TRANSFER_HANDLE transfer, uint32_t rcv_settle_mode)
	{
		return __LINE__;
	}
	int transfer_get_state(TRANSFER_HANDLE transfer, uint32_t* state)
	{
		return __LINE__;
	}

	int transfer_set_state(TRANSFER_HANDLE transfer, uint32_t state)
	{
		return __LINE__;
	}
	int transfer_get_resume(TRANSFER_HANDLE transfer, uint32_t* resume)
	{
		return __LINE__;
	}

	int transfer_set_resume(TRANSFER_HANDLE transfer, uint32_t resume)
	{
		return __LINE__;
	}
	int transfer_get_aborted(TRANSFER_HANDLE transfer, uint32_t* aborted)
	{
		return __LINE__;
	}

	int transfer_set_aborted(TRANSFER_HANDLE transfer, uint32_t aborted)
	{
		return __LINE__;
	}
	int transfer_get_batchable(TRANSFER_HANDLE transfer, uint32_t* batchable)
	{
		return __LINE__;
	}

	int transfer_set_batchable(TRANSFER_HANDLE transfer, uint32_t batchable)
	{
		return __LINE__;
	}

/* disposition */

	DISPOSITION_HANDLE disposition_create(role role, delivery_number first)
	{
		return NULL;
	}

	void disposition_destroy(DISPOSITION_HANDLE disposition)
	{
	}

	int disposition_get_role(DISPOSITION_HANDLE disposition, uint32_t* role)
	{
		return __LINE__;
	}

	int disposition_set_role(DISPOSITION_HANDLE disposition, uint32_t role)
	{
		return __LINE__;
	}
	int disposition_get_first(DISPOSITION_HANDLE disposition, uint32_t* first)
	{
		return __LINE__;
	}

	int disposition_set_first(DISPOSITION_HANDLE disposition, uint32_t first)
	{
		return __LINE__;
	}
	int disposition_get_last(DISPOSITION_HANDLE disposition, uint32_t* last)
	{
		return __LINE__;
	}

	int disposition_set_last(DISPOSITION_HANDLE disposition, uint32_t last)
	{
		return __LINE__;
	}
	int disposition_get_settled(DISPOSITION_HANDLE disposition, uint32_t* settled)
	{
		return __LINE__;
	}

	int disposition_set_settled(DISPOSITION_HANDLE disposition, uint32_t settled)
	{
		return __LINE__;
	}
	int disposition_get_state(DISPOSITION_HANDLE disposition, uint32_t* state)
	{
		return __LINE__;
	}

	int disposition_set_state(DISPOSITION_HANDLE disposition, uint32_t state)
	{
		return __LINE__;
	}
	int disposition_get_batchable(DISPOSITION_HANDLE disposition, uint32_t* batchable)
	{
		return __LINE__;
	}

	int disposition_set_batchable(DISPOSITION_HANDLE disposition, uint32_t batchable)
	{
		return __LINE__;
	}

/* detach */

	DETACH_HANDLE detach_create(handle handle)
	{
		return NULL;
	}

	void detach_destroy(DETACH_HANDLE detach)
	{
	}

	int detach_get_handle(DETACH_HANDLE detach, uint32_t* handle)
	{
		return __LINE__;
	}

	int detach_set_handle(DETACH_HANDLE detach, uint32_t handle)
	{
		return __LINE__;
	}
	int detach_get_closed(DETACH_HANDLE detach, uint32_t* closed)
	{
		return __LINE__;
	}

	int detach_set_closed(DETACH_HANDLE detach, uint32_t closed)
	{
		return __LINE__;
	}
	int detach_get_error(DETACH_HANDLE detach, uint32_t* error)
	{
		return __LINE__;
	}

	int detach_set_error(DETACH_HANDLE detach, uint32_t error)
	{
		return __LINE__;
	}

/* end */

	END_HANDLE end_create()
	{
		return NULL;
	}

	void end_destroy(END_HANDLE end)
	{
	}

	int end_get_error(END_HANDLE end, uint32_t* error)
	{
		return __LINE__;
	}

	int end_set_error(END_HANDLE end, uint32_t error)
	{
		return __LINE__;
	}

/* close */

	CLOSE_HANDLE close_create()
	{
		return NULL;
	}

	void close_destroy(CLOSE_HANDLE close)
	{
	}

	int close_get_error(CLOSE_HANDLE close, uint32_t* error)
	{
		return __LINE__;
	}

	int close_set_error(CLOSE_HANDLE close, uint32_t error)
	{
		return __LINE__;
	}

