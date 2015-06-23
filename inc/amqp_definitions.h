

#ifndef AMQP_DEFINITIONS_H
#define AMQP_DEFINITIONS_H

#ifdef __cplusplus
#include <cstdint>
#include <cstdbool>
extern "C" {
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "amqpvalue.h"

/* role */

	typedef bool role;

	#define role_sender false
	#define role_receiver true

/* sender-settle-mode */

	typedef uint8_t sender_settle_mode;

	#define sender_settle_mode_unsettled 0
	#define sender_settle_mode_settled 1
	#define sender_settle_mode_mixed 2

/* receiver-settle-mode */

	typedef uint8_t receiver_settle_mode;

	#define receiver_settle_mode_first 0
	#define receiver_settle_mode_second 1

/* handle */

	typedef uint32_t handle;


/* seconds */

	typedef uint32_t seconds;


/* milliseconds */

	typedef uint32_t milliseconds;


/* delivery-tag */

	typedef amqp_binary delivery_tag;


/* sequence-no */

	typedef uint32_t sequence_no;


/* delivery-number */

	typedef AMQP_VALUE delivery_number;


/* transfer-number */

	typedef AMQP_VALUE transfer_number;


/* message-format */

	typedef uint32_t message_format;


/* ietf-language-tag */

	typedef uint32_t ietf_language_tag;


/* fields */

	typedef AMQP_VALUE fields;


/* error */

	typedef void* ERROR_HANDLE;

	extern ERROR_HANDLE error_create(uint32_t condition);
	extern void error_destroy(ERROR_HANDLE error);

	extern int error_get_condition(ERROR_HANDLE error, uint32_t* condition);
	extern int error_set_condition(ERROR_HANDLE error, uint32_t condition);
	extern int error_get_description(ERROR_HANDLE error, const char** description);
	extern int error_set_description(ERROR_HANDLE error, const char* description);
	extern int error_get_info(ERROR_HANDLE error, AMQP_VALUE* info);
	extern int error_set_info(ERROR_HANDLE error, AMQP_VALUE info);

/* amqp-error */

	typedef uint32_t amqp_error;

	#define amqp_error_internal_error amqp_internal_error
	#define amqp_error_not_found amqp_not_found
	#define amqp_error_unauthorized_access amqp_unauthorized_access
	#define amqp_error_decode_error amqp_decode_error
	#define amqp_error_resource_limit_exceeded amqp_resource_limit_exceeded
	#define amqp_error_not_allowed amqp_not_allowed
	#define amqp_error_invalid_field amqp_invalid_field
	#define amqp_error_not_implemented amqp_not_implemented
	#define amqp_error_resource_locked amqp_resource_locked
	#define amqp_error_precondition_failed amqp_precondition_failed
	#define amqp_error_resource_deleted amqp_resource_deleted
	#define amqp_error_illegal_state amqp_illegal_state
	#define amqp_error_frame_size_too_small amqp_frame_size_too_small

/* connection-error */

	typedef uint32_t connection_error;

	#define connection_error_connection_forced amqp_connection_forced
	#define connection_error_framing_error amqp_connection_framing_error
	#define connection_error_redirect amqp_connection_redirect

/* session-error */

	typedef uint32_t session_error;

	#define session_error_window_violation amqp_session_window_violation
	#define session_error_errant_link amqp_session_errant_link
	#define session_error_handle_in_use amqp_session_handle_in_use
	#define session_error_unattached_handle amqp_session_unattached_handle

/* link-error */

	typedef uint32_t link_error;

	#define link_error_detach_forced amqp_link_detach_forced
	#define link_error_transfer_limit_exceeded amqp_link_transfer_limit_exceeded
	#define link_error_message_size_exceeded amqp_link_message_size_exceeded
	#define link_error_redirect amqp_link_redirect
	#define link_error_stolen amqp_link_stolen

/* open */

	typedef void* OPEN_HANDLE;

	extern OPEN_HANDLE open_create(const char* container_id);
	extern void open_destroy(OPEN_HANDLE open);

	extern int open_get_container_id(OPEN_HANDLE open, const char** container_id);
	extern int open_set_container_id(OPEN_HANDLE open, const char* container_id);
	extern int open_get_hostname(OPEN_HANDLE open, const char** hostname);
	extern int open_set_hostname(OPEN_HANDLE open, const char* hostname);
	extern int open_get_max_frame_size(OPEN_HANDLE open, uint32_t* max_frame_size);
	extern int open_set_max_frame_size(OPEN_HANDLE open, uint32_t max_frame_size);
	extern int open_get_channel_max(OPEN_HANDLE open, uint16_t* channel_max);
	extern int open_set_channel_max(OPEN_HANDLE open, uint16_t channel_max);
	extern int open_get_idle_time_out(OPEN_HANDLE open, AMQP_VALUE* idle_time_out);
	extern int open_set_idle_time_out(OPEN_HANDLE open, AMQP_VALUE idle_time_out);
	extern int open_get_outgoing_locales(OPEN_HANDLE open, AMQP_VALUE* outgoing_locales);
	extern int open_set_outgoing_locales(OPEN_HANDLE open, AMQP_VALUE outgoing_locales);
	extern int open_get_incoming_locales(OPEN_HANDLE open, AMQP_VALUE* incoming_locales);
	extern int open_set_incoming_locales(OPEN_HANDLE open, AMQP_VALUE incoming_locales);
	extern int open_get_offered_capabilities(OPEN_HANDLE open, uint32_t* offered_capabilities);
	extern int open_set_offered_capabilities(OPEN_HANDLE open, uint32_t offered_capabilities);
	extern int open_get_desired_capabilities(OPEN_HANDLE open, uint32_t* desired_capabilities);
	extern int open_set_desired_capabilities(OPEN_HANDLE open, uint32_t desired_capabilities);
	extern int open_get_properties(OPEN_HANDLE open, AMQP_VALUE* properties);
	extern int open_set_properties(OPEN_HANDLE open, AMQP_VALUE properties);

/* begin */

	typedef void* BEGIN_HANDLE;

	extern BEGIN_HANDLE begin_create(AMQP_VALUE next_outgoing_id, uint32_t incoming_window, uint32_t outgoing_window);
	extern void begin_destroy(BEGIN_HANDLE begin);

	extern int begin_get_remote_channel(BEGIN_HANDLE begin, uint16_t* remote_channel);
	extern int begin_set_remote_channel(BEGIN_HANDLE begin, uint16_t remote_channel);
	extern int begin_get_next_outgoing_id(BEGIN_HANDLE begin, AMQP_VALUE* next_outgoing_id);
	extern int begin_set_next_outgoing_id(BEGIN_HANDLE begin, AMQP_VALUE next_outgoing_id);
	extern int begin_get_incoming_window(BEGIN_HANDLE begin, uint32_t* incoming_window);
	extern int begin_set_incoming_window(BEGIN_HANDLE begin, uint32_t incoming_window);
	extern int begin_get_outgoing_window(BEGIN_HANDLE begin, uint32_t* outgoing_window);
	extern int begin_set_outgoing_window(BEGIN_HANDLE begin, uint32_t outgoing_window);
	extern int begin_get_handle_max(BEGIN_HANDLE begin, AMQP_VALUE* handle_max);
	extern int begin_set_handle_max(BEGIN_HANDLE begin, AMQP_VALUE handle_max);
	extern int begin_get_offered_capabilities(BEGIN_HANDLE begin, uint32_t* offered_capabilities);
	extern int begin_set_offered_capabilities(BEGIN_HANDLE begin, uint32_t offered_capabilities);
	extern int begin_get_desired_capabilities(BEGIN_HANDLE begin, uint32_t* desired_capabilities);
	extern int begin_set_desired_capabilities(BEGIN_HANDLE begin, uint32_t desired_capabilities);
	extern int begin_get_properties(BEGIN_HANDLE begin, AMQP_VALUE* properties);
	extern int begin_set_properties(BEGIN_HANDLE begin, AMQP_VALUE properties);

/* attach */

	typedef void* ATTACH_HANDLE;

	extern ATTACH_HANDLE attach_create(const char* name, AMQP_VALUE handle, AMQP_VALUE role);
	extern void attach_destroy(ATTACH_HANDLE attach);

	extern int attach_get_name(ATTACH_HANDLE attach, const char** name);
	extern int attach_set_name(ATTACH_HANDLE attach, const char* name);
	extern int attach_get_handle(ATTACH_HANDLE attach, AMQP_VALUE* handle);
	extern int attach_set_handle(ATTACH_HANDLE attach, AMQP_VALUE handle);
	extern int attach_get_role(ATTACH_HANDLE attach, AMQP_VALUE* role);
	extern int attach_set_role(ATTACH_HANDLE attach, AMQP_VALUE role);
	extern int attach_get_snd_settle_mode(ATTACH_HANDLE attach, AMQP_VALUE* snd_settle_mode);
	extern int attach_set_snd_settle_mode(ATTACH_HANDLE attach, AMQP_VALUE snd_settle_mode);
	extern int attach_get_rcv_settle_mode(ATTACH_HANDLE attach, AMQP_VALUE* rcv_settle_mode);
	extern int attach_set_rcv_settle_mode(ATTACH_HANDLE attach, AMQP_VALUE rcv_settle_mode);
	extern int attach_get_source(ATTACH_HANDLE attach, AMQP_VALUE* source);
	extern int attach_set_source(ATTACH_HANDLE attach, AMQP_VALUE source);
	extern int attach_get_target(ATTACH_HANDLE attach, AMQP_VALUE* target);
	extern int attach_set_target(ATTACH_HANDLE attach, AMQP_VALUE target);
	extern int attach_get_unsettled(ATTACH_HANDLE attach, AMQP_VALUE* unsettled);
	extern int attach_set_unsettled(ATTACH_HANDLE attach, AMQP_VALUE unsettled);
	extern int attach_get_incomplete_unsettled(ATTACH_HANDLE attach, bool* incomplete_unsettled);
	extern int attach_set_incomplete_unsettled(ATTACH_HANDLE attach, bool incomplete_unsettled);
	extern int attach_get_initial_delivery_count(ATTACH_HANDLE attach, AMQP_VALUE* initial_delivery_count);
	extern int attach_set_initial_delivery_count(ATTACH_HANDLE attach, AMQP_VALUE initial_delivery_count);
	extern int attach_get_max_message_size(ATTACH_HANDLE attach, uint64_t* max_message_size);
	extern int attach_set_max_message_size(ATTACH_HANDLE attach, uint64_t max_message_size);
	extern int attach_get_offered_capabilities(ATTACH_HANDLE attach, uint32_t* offered_capabilities);
	extern int attach_set_offered_capabilities(ATTACH_HANDLE attach, uint32_t offered_capabilities);
	extern int attach_get_desired_capabilities(ATTACH_HANDLE attach, uint32_t* desired_capabilities);
	extern int attach_set_desired_capabilities(ATTACH_HANDLE attach, uint32_t desired_capabilities);
	extern int attach_get_properties(ATTACH_HANDLE attach, AMQP_VALUE* properties);
	extern int attach_set_properties(ATTACH_HANDLE attach, AMQP_VALUE properties);

/* flow */

	typedef void* FLOW_HANDLE;

	extern FLOW_HANDLE flow_create(uint32_t incoming_window, AMQP_VALUE next_outgoing_id, uint32_t outgoing_window);
	extern void flow_destroy(FLOW_HANDLE flow);

	extern int flow_get_next_incoming_id(FLOW_HANDLE flow, AMQP_VALUE* next_incoming_id);
	extern int flow_set_next_incoming_id(FLOW_HANDLE flow, AMQP_VALUE next_incoming_id);
	extern int flow_get_incoming_window(FLOW_HANDLE flow, uint32_t* incoming_window);
	extern int flow_set_incoming_window(FLOW_HANDLE flow, uint32_t incoming_window);
	extern int flow_get_next_outgoing_id(FLOW_HANDLE flow, AMQP_VALUE* next_outgoing_id);
	extern int flow_set_next_outgoing_id(FLOW_HANDLE flow, AMQP_VALUE next_outgoing_id);
	extern int flow_get_outgoing_window(FLOW_HANDLE flow, uint32_t* outgoing_window);
	extern int flow_set_outgoing_window(FLOW_HANDLE flow, uint32_t outgoing_window);
	extern int flow_get_handle(FLOW_HANDLE flow, AMQP_VALUE* handle);
	extern int flow_set_handle(FLOW_HANDLE flow, AMQP_VALUE handle);
	extern int flow_get_delivery_count(FLOW_HANDLE flow, AMQP_VALUE* delivery_count);
	extern int flow_set_delivery_count(FLOW_HANDLE flow, AMQP_VALUE delivery_count);
	extern int flow_get_link_credit(FLOW_HANDLE flow, uint32_t* link_credit);
	extern int flow_set_link_credit(FLOW_HANDLE flow, uint32_t link_credit);
	extern int flow_get_available(FLOW_HANDLE flow, uint32_t* available);
	extern int flow_set_available(FLOW_HANDLE flow, uint32_t available);
	extern int flow_get_drain(FLOW_HANDLE flow, bool* drain);
	extern int flow_set_drain(FLOW_HANDLE flow, bool drain);
	extern int flow_get_echo(FLOW_HANDLE flow, bool* echo);
	extern int flow_set_echo(FLOW_HANDLE flow, bool echo);
	extern int flow_get_properties(FLOW_HANDLE flow, AMQP_VALUE* properties);
	extern int flow_set_properties(FLOW_HANDLE flow, AMQP_VALUE properties);

/* transfer */

	typedef void* TRANSFER_HANDLE;

	extern TRANSFER_HANDLE transfer_create(AMQP_VALUE handle);
	extern void transfer_destroy(TRANSFER_HANDLE transfer);

	extern int transfer_get_handle(TRANSFER_HANDLE transfer, AMQP_VALUE* handle);
	extern int transfer_set_handle(TRANSFER_HANDLE transfer, AMQP_VALUE handle);
	extern int transfer_get_delivery_id(TRANSFER_HANDLE transfer, AMQP_VALUE* delivery_id);
	extern int transfer_set_delivery_id(TRANSFER_HANDLE transfer, AMQP_VALUE delivery_id);
	extern int transfer_get_delivery_tag(TRANSFER_HANDLE transfer, AMQP_VALUE* delivery_tag);
	extern int transfer_set_delivery_tag(TRANSFER_HANDLE transfer, AMQP_VALUE delivery_tag);
	extern int transfer_get_message_format(TRANSFER_HANDLE transfer, AMQP_VALUE* message_format);
	extern int transfer_set_message_format(TRANSFER_HANDLE transfer, AMQP_VALUE message_format);
	extern int transfer_get_settled(TRANSFER_HANDLE transfer, bool* settled);
	extern int transfer_set_settled(TRANSFER_HANDLE transfer, bool settled);
	extern int transfer_get_more(TRANSFER_HANDLE transfer, bool* more);
	extern int transfer_set_more(TRANSFER_HANDLE transfer, bool more);
	extern int transfer_get_rcv_settle_mode(TRANSFER_HANDLE transfer, AMQP_VALUE* rcv_settle_mode);
	extern int transfer_set_rcv_settle_mode(TRANSFER_HANDLE transfer, AMQP_VALUE rcv_settle_mode);
	extern int transfer_get_state(TRANSFER_HANDLE transfer, AMQP_VALUE* state);
	extern int transfer_set_state(TRANSFER_HANDLE transfer, AMQP_VALUE state);
	extern int transfer_get_resume(TRANSFER_HANDLE transfer, bool* resume);
	extern int transfer_set_resume(TRANSFER_HANDLE transfer, bool resume);
	extern int transfer_get_aborted(TRANSFER_HANDLE transfer, bool* aborted);
	extern int transfer_set_aborted(TRANSFER_HANDLE transfer, bool aborted);
	extern int transfer_get_batchable(TRANSFER_HANDLE transfer, bool* batchable);
	extern int transfer_set_batchable(TRANSFER_HANDLE transfer, bool batchable);

/* disposition */

	typedef void* DISPOSITION_HANDLE;

	extern DISPOSITION_HANDLE disposition_create(AMQP_VALUE role, AMQP_VALUE first);
	extern void disposition_destroy(DISPOSITION_HANDLE disposition);

	extern int disposition_get_role(DISPOSITION_HANDLE disposition, AMQP_VALUE* role);
	extern int disposition_set_role(DISPOSITION_HANDLE disposition, AMQP_VALUE role);
	extern int disposition_get_first(DISPOSITION_HANDLE disposition, AMQP_VALUE* first);
	extern int disposition_set_first(DISPOSITION_HANDLE disposition, AMQP_VALUE first);
	extern int disposition_get_last(DISPOSITION_HANDLE disposition, AMQP_VALUE* last);
	extern int disposition_set_last(DISPOSITION_HANDLE disposition, AMQP_VALUE last);
	extern int disposition_get_settled(DISPOSITION_HANDLE disposition, bool* settled);
	extern int disposition_set_settled(DISPOSITION_HANDLE disposition, bool settled);
	extern int disposition_get_state(DISPOSITION_HANDLE disposition, AMQP_VALUE* state);
	extern int disposition_set_state(DISPOSITION_HANDLE disposition, AMQP_VALUE state);
	extern int disposition_get_batchable(DISPOSITION_HANDLE disposition, bool* batchable);
	extern int disposition_set_batchable(DISPOSITION_HANDLE disposition, bool batchable);

/* detach */

	typedef void* DETACH_HANDLE;

	extern DETACH_HANDLE detach_create(AMQP_VALUE handle);
	extern void detach_destroy(DETACH_HANDLE detach);

	extern int detach_get_handle(DETACH_HANDLE detach, AMQP_VALUE* handle);
	extern int detach_set_handle(DETACH_HANDLE detach, AMQP_VALUE handle);
	extern int detach_get_closed(DETACH_HANDLE detach, bool* closed);
	extern int detach_set_closed(DETACH_HANDLE detach, bool closed);
	extern int detach_get_error(DETACH_HANDLE detach, AMQP_VALUE* error);
	extern int detach_set_error(DETACH_HANDLE detach, AMQP_VALUE error);

/* end */

	typedef void* END_HANDLE;

	extern END_HANDLE end_create();
	extern void end_destroy(END_HANDLE end);

	extern int end_get_error(END_HANDLE end, AMQP_VALUE* error);
	extern int end_set_error(END_HANDLE end, AMQP_VALUE error);

/* close */

	typedef void* CLOSE_HANDLE;

	extern CLOSE_HANDLE close_create();
	extern void close_destroy(CLOSE_HANDLE close);

	extern int close_get_error(CLOSE_HANDLE close, AMQP_VALUE* error);
	extern int close_set_error(CLOSE_HANDLE close, AMQP_VALUE error);


#ifdef __cplusplus
}
#endif

#endif /* AMQP_DEFINITIONS_H */
