

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

	#define amqpvalue_create_role amqpvalue_create_boolean
	#define amqpvalue_get_role amqpvalue_get_boolean

	#define role_sender false
	#define role_receiver true

/* sender-settle-mode */

	typedef uint8_t sender_settle_mode;

	#define amqpvalue_create_sender_settle_mode amqpvalue_create_ubyte
	#define amqpvalue_get_sender_settle_mode amqpvalue_get_ubyte

	#define sender_settle_mode_unsettled 0
	#define sender_settle_mode_settled 1
	#define sender_settle_mode_mixed 2

/* receiver-settle-mode */

	typedef uint8_t receiver_settle_mode;

	#define amqpvalue_create_receiver_settle_mode amqpvalue_create_ubyte
	#define amqpvalue_get_receiver_settle_mode amqpvalue_get_ubyte

	#define receiver_settle_mode_first 0
	#define receiver_settle_mode_second 1

/* handle */

	typedef uint32_t handle;

	#define amqpvalue_create_handle amqpvalue_create_uint
	#define amqpvalue_get_handle amqpvalue_get_uint


/* seconds */

	typedef uint32_t seconds;

	#define amqpvalue_create_seconds amqpvalue_create_uint
	#define amqpvalue_get_seconds amqpvalue_get_uint


/* milliseconds */

	typedef uint32_t milliseconds;

	#define amqpvalue_create_milliseconds amqpvalue_create_uint
	#define amqpvalue_get_milliseconds amqpvalue_get_uint


/* delivery-tag */

	typedef amqp_binary delivery_tag;

	#define amqpvalue_create_delivery_tag amqpvalue_create_binary
	#define amqpvalue_get_delivery_tag amqpvalue_get_binary


/* sequence-no */

	typedef uint32_t sequence_no;

	#define amqpvalue_create_sequence_no amqpvalue_create_uint
	#define amqpvalue_get_sequence_no amqpvalue_get_uint


/* delivery-number */

	typedef sequence_no delivery_number;

	#define amqpvalue_create_delivery_number amqpvalue_create_sequence_no
	#define amqpvalue_get_delivery_number amqpvalue_get_sequence_no


/* transfer-number */

	typedef sequence_no transfer_number;

	#define amqpvalue_create_transfer_number amqpvalue_create_sequence_no
	#define amqpvalue_get_transfer_number amqpvalue_get_sequence_no


/* message-format */

	typedef uint32_t message_format;

	#define amqpvalue_create_message_format amqpvalue_create_uint
	#define amqpvalue_get_message_format amqpvalue_get_uint


/* ietf-language-tag */

	typedef const char* ietf_language_tag;

	#define amqpvalue_create_ietf_language_tag amqpvalue_create_symbol
	#define amqpvalue_get_ietf_language_tag amqpvalue_get_symbol


/* fields */

	typedef AMQP_VALUE fields;

	#define amqpvalue_create_fields amqpvalue_clone
	#define amqpvalue_get_fields amqpvalue_get_map


/* error */

	typedef void* ERROR_HANDLE;

	extern ERROR_HANDLE error_create(const char* condition_value);
	extern ERROR_HANDLE error_clone(ERROR_HANDLE value);
	extern void error_destroy(ERROR_HANDLE error);
	extern bool is_error_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_error(AMQP_VALUE value, ERROR_HANDLE* ERROR_handle);
	extern AMQP_VALUE amqpvalue_create_error(ERROR_HANDLE error);

	extern int error_get_condition(ERROR_HANDLE error, const char** condition_value);
	extern int error_set_condition(ERROR_HANDLE error, const char* condition_value);
	extern int error_get_description(ERROR_HANDLE error, const char** description_value);
	extern int error_set_description(ERROR_HANDLE error, const char* description_value);
	extern int error_get_info(ERROR_HANDLE error, fields* info_value);
	extern int error_set_info(ERROR_HANDLE error, fields info_value);

/* amqp-error */

	typedef const char* amqp_error;

	#define amqpvalue_create_amqp_error amqpvalue_create_symbol
	#define amqpvalue_get_amqp_error amqpvalue_get_symbol

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

	typedef const char* connection_error;

	#define amqpvalue_create_connection_error amqpvalue_create_symbol
	#define amqpvalue_get_connection_error amqpvalue_get_symbol

	#define connection_error_connection_forced amqp_connection_forced
	#define connection_error_framing_error amqp_connection_framing_error
	#define connection_error_redirect amqp_connection_redirect

/* session-error */

	typedef const char* session_error;

	#define amqpvalue_create_session_error amqpvalue_create_symbol
	#define amqpvalue_get_session_error amqpvalue_get_symbol

	#define session_error_window_violation amqp_session_window_violation
	#define session_error_errant_link amqp_session_errant_link
	#define session_error_handle_in_use amqp_session_handle_in_use
	#define session_error_unattached_handle amqp_session_unattached_handle

/* link-error */

	typedef const char* link_error;

	#define amqpvalue_create_link_error amqpvalue_create_symbol
	#define amqpvalue_get_link_error amqpvalue_get_symbol

	#define link_error_detach_forced amqp_link_detach_forced
	#define link_error_transfer_limit_exceeded amqp_link_transfer_limit_exceeded
	#define link_error_message_size_exceeded amqp_link_message_size_exceeded
	#define link_error_redirect amqp_link_redirect
	#define link_error_stolen amqp_link_stolen

/* open */

	typedef void* OPEN_HANDLE;

	extern OPEN_HANDLE open_create(const char* container_id_value);
	extern OPEN_HANDLE open_clone(OPEN_HANDLE value);
	extern void open_destroy(OPEN_HANDLE open);
	extern bool is_open_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_open(AMQP_VALUE value, OPEN_HANDLE* OPEN_handle);
	extern AMQP_VALUE amqpvalue_create_open(OPEN_HANDLE open);

	extern int open_get_container_id(OPEN_HANDLE open, const char** container_id_value);
	extern int open_set_container_id(OPEN_HANDLE open, const char* container_id_value);
	extern int open_get_hostname(OPEN_HANDLE open, const char** hostname_value);
	extern int open_set_hostname(OPEN_HANDLE open, const char* hostname_value);
	extern int open_get_max_frame_size(OPEN_HANDLE open, uint32_t* max_frame_size_value);
	extern int open_set_max_frame_size(OPEN_HANDLE open, uint32_t max_frame_size_value);
	extern int open_get_channel_max(OPEN_HANDLE open, uint16_t* channel_max_value);
	extern int open_set_channel_max(OPEN_HANDLE open, uint16_t channel_max_value);
	extern int open_get_idle_time_out(OPEN_HANDLE open, milliseconds* idle_time_out_value);
	extern int open_set_idle_time_out(OPEN_HANDLE open, milliseconds idle_time_out_value);
	extern int open_get_outgoing_locales(OPEN_HANDLE open, ietf_language_tag* outgoing_locales_value);
	extern int open_set_outgoing_locales(OPEN_HANDLE open, ietf_language_tag outgoing_locales_value);
	extern int open_get_incoming_locales(OPEN_HANDLE open, ietf_language_tag* incoming_locales_value);
	extern int open_set_incoming_locales(OPEN_HANDLE open, ietf_language_tag incoming_locales_value);
	extern int open_get_offered_capabilities(OPEN_HANDLE open, const char** offered_capabilities_value);
	extern int open_set_offered_capabilities(OPEN_HANDLE open, const char* offered_capabilities_value);
	extern int open_get_desired_capabilities(OPEN_HANDLE open, const char** desired_capabilities_value);
	extern int open_set_desired_capabilities(OPEN_HANDLE open, const char* desired_capabilities_value);
	extern int open_get_properties(OPEN_HANDLE open, fields* properties_value);
	extern int open_set_properties(OPEN_HANDLE open, fields properties_value);

/* begin */

	typedef void* BEGIN_HANDLE;

	extern BEGIN_HANDLE begin_create(transfer_number next_outgoing_id_value, uint32_t incoming_window_value, uint32_t outgoing_window_value);
	extern BEGIN_HANDLE begin_clone(BEGIN_HANDLE value);
	extern void begin_destroy(BEGIN_HANDLE begin);
	extern bool is_begin_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_begin(AMQP_VALUE value, BEGIN_HANDLE* BEGIN_handle);
	extern AMQP_VALUE amqpvalue_create_begin(BEGIN_HANDLE begin);

	extern int begin_get_remote_channel(BEGIN_HANDLE begin, uint16_t* remote_channel_value);
	extern int begin_set_remote_channel(BEGIN_HANDLE begin, uint16_t remote_channel_value);
	extern int begin_get_next_outgoing_id(BEGIN_HANDLE begin, transfer_number* next_outgoing_id_value);
	extern int begin_set_next_outgoing_id(BEGIN_HANDLE begin, transfer_number next_outgoing_id_value);
	extern int begin_get_incoming_window(BEGIN_HANDLE begin, uint32_t* incoming_window_value);
	extern int begin_set_incoming_window(BEGIN_HANDLE begin, uint32_t incoming_window_value);
	extern int begin_get_outgoing_window(BEGIN_HANDLE begin, uint32_t* outgoing_window_value);
	extern int begin_set_outgoing_window(BEGIN_HANDLE begin, uint32_t outgoing_window_value);
	extern int begin_get_handle_max(BEGIN_HANDLE begin, handle* handle_max_value);
	extern int begin_set_handle_max(BEGIN_HANDLE begin, handle handle_max_value);
	extern int begin_get_offered_capabilities(BEGIN_HANDLE begin, const char** offered_capabilities_value);
	extern int begin_set_offered_capabilities(BEGIN_HANDLE begin, const char* offered_capabilities_value);
	extern int begin_get_desired_capabilities(BEGIN_HANDLE begin, const char** desired_capabilities_value);
	extern int begin_set_desired_capabilities(BEGIN_HANDLE begin, const char* desired_capabilities_value);
	extern int begin_get_properties(BEGIN_HANDLE begin, fields* properties_value);
	extern int begin_set_properties(BEGIN_HANDLE begin, fields properties_value);

/* attach */

	typedef void* ATTACH_HANDLE;

	extern ATTACH_HANDLE attach_create(const char* name_value, handle handle_value, role role_value);
	extern ATTACH_HANDLE attach_clone(ATTACH_HANDLE value);
	extern void attach_destroy(ATTACH_HANDLE attach);
	extern bool is_attach_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_attach(AMQP_VALUE value, ATTACH_HANDLE* ATTACH_handle);
	extern AMQP_VALUE amqpvalue_create_attach(ATTACH_HANDLE attach);

	extern int attach_get_name(ATTACH_HANDLE attach, const char** name_value);
	extern int attach_set_name(ATTACH_HANDLE attach, const char* name_value);
	extern int attach_get_handle(ATTACH_HANDLE attach, handle* handle_value);
	extern int attach_set_handle(ATTACH_HANDLE attach, handle handle_value);
	extern int attach_get_role(ATTACH_HANDLE attach, role* role_value);
	extern int attach_set_role(ATTACH_HANDLE attach, role role_value);
	extern int attach_get_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode* snd_settle_mode_value);
	extern int attach_set_snd_settle_mode(ATTACH_HANDLE attach, sender_settle_mode snd_settle_mode_value);
	extern int attach_get_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode* rcv_settle_mode_value);
	extern int attach_set_rcv_settle_mode(ATTACH_HANDLE attach, receiver_settle_mode rcv_settle_mode_value);
	extern int attach_get_source(ATTACH_HANDLE attach, AMQP_VALUE* source_value);
	extern int attach_set_source(ATTACH_HANDLE attach, AMQP_VALUE source_value);
	extern int attach_get_target(ATTACH_HANDLE attach, AMQP_VALUE* target_value);
	extern int attach_set_target(ATTACH_HANDLE attach, AMQP_VALUE target_value);
	extern int attach_get_unsettled(ATTACH_HANDLE attach, AMQP_VALUE* unsettled_value);
	extern int attach_set_unsettled(ATTACH_HANDLE attach, AMQP_VALUE unsettled_value);
	extern int attach_get_incomplete_unsettled(ATTACH_HANDLE attach, bool* incomplete_unsettled_value);
	extern int attach_set_incomplete_unsettled(ATTACH_HANDLE attach, bool incomplete_unsettled_value);
	extern int attach_get_initial_delivery_count(ATTACH_HANDLE attach, sequence_no* initial_delivery_count_value);
	extern int attach_set_initial_delivery_count(ATTACH_HANDLE attach, sequence_no initial_delivery_count_value);
	extern int attach_get_max_message_size(ATTACH_HANDLE attach, uint64_t* max_message_size_value);
	extern int attach_set_max_message_size(ATTACH_HANDLE attach, uint64_t max_message_size_value);
	extern int attach_get_offered_capabilities(ATTACH_HANDLE attach, const char** offered_capabilities_value);
	extern int attach_set_offered_capabilities(ATTACH_HANDLE attach, const char* offered_capabilities_value);
	extern int attach_get_desired_capabilities(ATTACH_HANDLE attach, const char** desired_capabilities_value);
	extern int attach_set_desired_capabilities(ATTACH_HANDLE attach, const char* desired_capabilities_value);
	extern int attach_get_properties(ATTACH_HANDLE attach, fields* properties_value);
	extern int attach_set_properties(ATTACH_HANDLE attach, fields properties_value);

/* flow */

	typedef void* FLOW_HANDLE;

	extern FLOW_HANDLE flow_create(uint32_t incoming_window_value, transfer_number next_outgoing_id_value, uint32_t outgoing_window_value);
	extern FLOW_HANDLE flow_clone(FLOW_HANDLE value);
	extern void flow_destroy(FLOW_HANDLE flow);
	extern bool is_flow_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_flow(AMQP_VALUE value, FLOW_HANDLE* FLOW_handle);
	extern AMQP_VALUE amqpvalue_create_flow(FLOW_HANDLE flow);

	extern int flow_get_next_incoming_id(FLOW_HANDLE flow, transfer_number* next_incoming_id_value);
	extern int flow_set_next_incoming_id(FLOW_HANDLE flow, transfer_number next_incoming_id_value);
	extern int flow_get_incoming_window(FLOW_HANDLE flow, uint32_t* incoming_window_value);
	extern int flow_set_incoming_window(FLOW_HANDLE flow, uint32_t incoming_window_value);
	extern int flow_get_next_outgoing_id(FLOW_HANDLE flow, transfer_number* next_outgoing_id_value);
	extern int flow_set_next_outgoing_id(FLOW_HANDLE flow, transfer_number next_outgoing_id_value);
	extern int flow_get_outgoing_window(FLOW_HANDLE flow, uint32_t* outgoing_window_value);
	extern int flow_set_outgoing_window(FLOW_HANDLE flow, uint32_t outgoing_window_value);
	extern int flow_get_handle(FLOW_HANDLE flow, handle* handle_value);
	extern int flow_set_handle(FLOW_HANDLE flow, handle handle_value);
	extern int flow_get_delivery_count(FLOW_HANDLE flow, sequence_no* delivery_count_value);
	extern int flow_set_delivery_count(FLOW_HANDLE flow, sequence_no delivery_count_value);
	extern int flow_get_link_credit(FLOW_HANDLE flow, uint32_t* link_credit_value);
	extern int flow_set_link_credit(FLOW_HANDLE flow, uint32_t link_credit_value);
	extern int flow_get_available(FLOW_HANDLE flow, uint32_t* available_value);
	extern int flow_set_available(FLOW_HANDLE flow, uint32_t available_value);
	extern int flow_get_drain(FLOW_HANDLE flow, bool* drain_value);
	extern int flow_set_drain(FLOW_HANDLE flow, bool drain_value);
	extern int flow_get_echo(FLOW_HANDLE flow, bool* echo_value);
	extern int flow_set_echo(FLOW_HANDLE flow, bool echo_value);
	extern int flow_get_properties(FLOW_HANDLE flow, fields* properties_value);
	extern int flow_set_properties(FLOW_HANDLE flow, fields properties_value);

/* transfer */

	typedef void* TRANSFER_HANDLE;

	extern TRANSFER_HANDLE transfer_create(handle handle_value);
	extern TRANSFER_HANDLE transfer_clone(TRANSFER_HANDLE value);
	extern void transfer_destroy(TRANSFER_HANDLE transfer);
	extern bool is_transfer_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_transfer(AMQP_VALUE value, TRANSFER_HANDLE* TRANSFER_handle);
	extern AMQP_VALUE amqpvalue_create_transfer(TRANSFER_HANDLE transfer);

	extern int transfer_get_handle(TRANSFER_HANDLE transfer, handle* handle_value);
	extern int transfer_set_handle(TRANSFER_HANDLE transfer, handle handle_value);
	extern int transfer_get_delivery_id(TRANSFER_HANDLE transfer, delivery_number* delivery_id_value);
	extern int transfer_set_delivery_id(TRANSFER_HANDLE transfer, delivery_number delivery_id_value);
	extern int transfer_get_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag* delivery_tag_value);
	extern int transfer_set_delivery_tag(TRANSFER_HANDLE transfer, delivery_tag delivery_tag_value);
	extern int transfer_get_message_format(TRANSFER_HANDLE transfer, message_format* message_format_value);
	extern int transfer_set_message_format(TRANSFER_HANDLE transfer, message_format message_format_value);
	extern int transfer_get_settled(TRANSFER_HANDLE transfer, bool* settled_value);
	extern int transfer_set_settled(TRANSFER_HANDLE transfer, bool settled_value);
	extern int transfer_get_more(TRANSFER_HANDLE transfer, bool* more_value);
	extern int transfer_set_more(TRANSFER_HANDLE transfer, bool more_value);
	extern int transfer_get_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode* rcv_settle_mode_value);
	extern int transfer_set_rcv_settle_mode(TRANSFER_HANDLE transfer, receiver_settle_mode rcv_settle_mode_value);
	extern int transfer_get_state(TRANSFER_HANDLE transfer, AMQP_VALUE* state_value);
	extern int transfer_set_state(TRANSFER_HANDLE transfer, AMQP_VALUE state_value);
	extern int transfer_get_resume(TRANSFER_HANDLE transfer, bool* resume_value);
	extern int transfer_set_resume(TRANSFER_HANDLE transfer, bool resume_value);
	extern int transfer_get_aborted(TRANSFER_HANDLE transfer, bool* aborted_value);
	extern int transfer_set_aborted(TRANSFER_HANDLE transfer, bool aborted_value);
	extern int transfer_get_batchable(TRANSFER_HANDLE transfer, bool* batchable_value);
	extern int transfer_set_batchable(TRANSFER_HANDLE transfer, bool batchable_value);

/* disposition */

	typedef void* DISPOSITION_HANDLE;

	extern DISPOSITION_HANDLE disposition_create(role role_value, delivery_number first_value);
	extern DISPOSITION_HANDLE disposition_clone(DISPOSITION_HANDLE value);
	extern void disposition_destroy(DISPOSITION_HANDLE disposition);
	extern bool is_disposition_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_disposition(AMQP_VALUE value, DISPOSITION_HANDLE* DISPOSITION_handle);
	extern AMQP_VALUE amqpvalue_create_disposition(DISPOSITION_HANDLE disposition);

	extern int disposition_get_role(DISPOSITION_HANDLE disposition, role* role_value);
	extern int disposition_set_role(DISPOSITION_HANDLE disposition, role role_value);
	extern int disposition_get_first(DISPOSITION_HANDLE disposition, delivery_number* first_value);
	extern int disposition_set_first(DISPOSITION_HANDLE disposition, delivery_number first_value);
	extern int disposition_get_last(DISPOSITION_HANDLE disposition, delivery_number* last_value);
	extern int disposition_set_last(DISPOSITION_HANDLE disposition, delivery_number last_value);
	extern int disposition_get_settled(DISPOSITION_HANDLE disposition, bool* settled_value);
	extern int disposition_set_settled(DISPOSITION_HANDLE disposition, bool settled_value);
	extern int disposition_get_state(DISPOSITION_HANDLE disposition, AMQP_VALUE* state_value);
	extern int disposition_set_state(DISPOSITION_HANDLE disposition, AMQP_VALUE state_value);
	extern int disposition_get_batchable(DISPOSITION_HANDLE disposition, bool* batchable_value);
	extern int disposition_set_batchable(DISPOSITION_HANDLE disposition, bool batchable_value);

/* detach */

	typedef void* DETACH_HANDLE;

	extern DETACH_HANDLE detach_create(handle handle_value);
	extern DETACH_HANDLE detach_clone(DETACH_HANDLE value);
	extern void detach_destroy(DETACH_HANDLE detach);
	extern bool is_detach_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_detach(AMQP_VALUE value, DETACH_HANDLE* DETACH_handle);
	extern AMQP_VALUE amqpvalue_create_detach(DETACH_HANDLE detach);

	extern int detach_get_handle(DETACH_HANDLE detach, handle* handle_value);
	extern int detach_set_handle(DETACH_HANDLE detach, handle handle_value);
	extern int detach_get_closed(DETACH_HANDLE detach, bool* closed_value);
	extern int detach_set_closed(DETACH_HANDLE detach, bool closed_value);
	extern int detach_get_error(DETACH_HANDLE detach, ERROR_HANDLE* error_value);
	extern int detach_set_error(DETACH_HANDLE detach, ERROR_HANDLE error_value);

/* end */

	typedef void* END_HANDLE;

	extern END_HANDLE end_create(void);
	extern END_HANDLE end_clone(END_HANDLE value);
	extern void end_destroy(END_HANDLE end);
	extern bool is_end_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_end(AMQP_VALUE value, END_HANDLE* END_handle);
	extern AMQP_VALUE amqpvalue_create_end(END_HANDLE end);

	extern int end_get_error(END_HANDLE end, ERROR_HANDLE* error_value);
	extern int end_set_error(END_HANDLE end, ERROR_HANDLE error_value);

/* close */

	typedef void* CLOSE_HANDLE;

	extern CLOSE_HANDLE close_create(void);
	extern CLOSE_HANDLE close_clone(CLOSE_HANDLE value);
	extern void close_destroy(CLOSE_HANDLE close);
	extern bool is_close_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_close(AMQP_VALUE value, CLOSE_HANDLE* CLOSE_handle);
	extern AMQP_VALUE amqpvalue_create_close(CLOSE_HANDLE close);

	extern int close_get_error(CLOSE_HANDLE close, ERROR_HANDLE* error_value);
	extern int close_set_error(CLOSE_HANDLE close, ERROR_HANDLE error_value);

/* sasl-code */

	typedef uint8_t sasl_code;

	#define amqpvalue_create_sasl_code amqpvalue_create_ubyte
	#define amqpvalue_get_sasl_code amqpvalue_get_ubyte

	#define sasl_code_ok 0
	#define sasl_code_auth 1
	#define sasl_code_sys 2
	#define sasl_code_sys_perm 3
	#define sasl_code_sys_temp 4

/* sasl-mechanisms */

	typedef void* SASL_MECHANISMS_HANDLE;

	extern SASL_MECHANISMS_HANDLE sasl_mechanisms_create(const char* sasl_server_mechanisms_value);
	extern SASL_MECHANISMS_HANDLE sasl_mechanisms_clone(SASL_MECHANISMS_HANDLE value);
	extern void sasl_mechanisms_destroy(SASL_MECHANISMS_HANDLE sasl_mechanisms);
	extern bool is_sasl_mechanisms_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_sasl_mechanisms(AMQP_VALUE value, SASL_MECHANISMS_HANDLE* SASL_MECHANISMS_handle);
	extern AMQP_VALUE amqpvalue_create_sasl_mechanisms(SASL_MECHANISMS_HANDLE sasl_mechanisms);

	extern int sasl_mechanisms_get_sasl_server_mechanisms(SASL_MECHANISMS_HANDLE sasl_mechanisms, const char** sasl_server_mechanisms_value);
	extern int sasl_mechanisms_set_sasl_server_mechanisms(SASL_MECHANISMS_HANDLE sasl_mechanisms, const char* sasl_server_mechanisms_value);

/* sasl-init */

	typedef void* SASL_INIT_HANDLE;

	extern SASL_INIT_HANDLE sasl_init_create(const char* mechanism_value);
	extern SASL_INIT_HANDLE sasl_init_clone(SASL_INIT_HANDLE value);
	extern void sasl_init_destroy(SASL_INIT_HANDLE sasl_init);
	extern bool is_sasl_init_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_sasl_init(AMQP_VALUE value, SASL_INIT_HANDLE* SASL_INIT_handle);
	extern AMQP_VALUE amqpvalue_create_sasl_init(SASL_INIT_HANDLE sasl_init);

	extern int sasl_init_get_mechanism(SASL_INIT_HANDLE sasl_init, const char** mechanism_value);
	extern int sasl_init_set_mechanism(SASL_INIT_HANDLE sasl_init, const char* mechanism_value);
	extern int sasl_init_get_initial_response(SASL_INIT_HANDLE sasl_init, amqp_binary* initial_response_value);
	extern int sasl_init_set_initial_response(SASL_INIT_HANDLE sasl_init, amqp_binary initial_response_value);
	extern int sasl_init_get_hostname(SASL_INIT_HANDLE sasl_init, const char** hostname_value);
	extern int sasl_init_set_hostname(SASL_INIT_HANDLE sasl_init, const char* hostname_value);

/* sasl-challenge */

	typedef void* SASL_CHALLENGE_HANDLE;

	extern SASL_CHALLENGE_HANDLE sasl_challenge_create(amqp_binary challenge_value);
	extern SASL_CHALLENGE_HANDLE sasl_challenge_clone(SASL_CHALLENGE_HANDLE value);
	extern void sasl_challenge_destroy(SASL_CHALLENGE_HANDLE sasl_challenge);
	extern bool is_sasl_challenge_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_sasl_challenge(AMQP_VALUE value, SASL_CHALLENGE_HANDLE* SASL_CHALLENGE_handle);
	extern AMQP_VALUE amqpvalue_create_sasl_challenge(SASL_CHALLENGE_HANDLE sasl_challenge);

	extern int sasl_challenge_get_challenge(SASL_CHALLENGE_HANDLE sasl_challenge, amqp_binary* challenge_value);
	extern int sasl_challenge_set_challenge(SASL_CHALLENGE_HANDLE sasl_challenge, amqp_binary challenge_value);

/* sasl-response */

	typedef void* SASL_RESPONSE_HANDLE;

	extern SASL_RESPONSE_HANDLE sasl_response_create(amqp_binary response_value);
	extern SASL_RESPONSE_HANDLE sasl_response_clone(SASL_RESPONSE_HANDLE value);
	extern void sasl_response_destroy(SASL_RESPONSE_HANDLE sasl_response);
	extern bool is_sasl_response_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_sasl_response(AMQP_VALUE value, SASL_RESPONSE_HANDLE* SASL_RESPONSE_handle);
	extern AMQP_VALUE amqpvalue_create_sasl_response(SASL_RESPONSE_HANDLE sasl_response);

	extern int sasl_response_get_response(SASL_RESPONSE_HANDLE sasl_response, amqp_binary* response_value);
	extern int sasl_response_set_response(SASL_RESPONSE_HANDLE sasl_response, amqp_binary response_value);

/* sasl-outcome */

	typedef void* SASL_OUTCOME_HANDLE;

	extern SASL_OUTCOME_HANDLE sasl_outcome_create(sasl_code code_value);
	extern SASL_OUTCOME_HANDLE sasl_outcome_clone(SASL_OUTCOME_HANDLE value);
	extern void sasl_outcome_destroy(SASL_OUTCOME_HANDLE sasl_outcome);
	extern bool is_sasl_outcome_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_sasl_outcome(AMQP_VALUE value, SASL_OUTCOME_HANDLE* SASL_OUTCOME_handle);
	extern AMQP_VALUE amqpvalue_create_sasl_outcome(SASL_OUTCOME_HANDLE sasl_outcome);

	extern int sasl_outcome_get_code(SASL_OUTCOME_HANDLE sasl_outcome, sasl_code* code_value);
	extern int sasl_outcome_set_code(SASL_OUTCOME_HANDLE sasl_outcome, sasl_code code_value);
	extern int sasl_outcome_get_additional_data(SASL_OUTCOME_HANDLE sasl_outcome, amqp_binary* additional_data_value);
	extern int sasl_outcome_set_additional_data(SASL_OUTCOME_HANDLE sasl_outcome, amqp_binary additional_data_value);

/* terminus-durability */

	typedef uint32_t terminus_durability;

	#define amqpvalue_create_terminus_durability amqpvalue_create_uint
	#define amqpvalue_get_terminus_durability amqpvalue_get_uint

	#define terminus_durability_none 0
	#define terminus_durability_configuration 1
	#define terminus_durability_unsettled_state 2

/* terminus-expiry-policy */

	typedef const char* terminus_expiry_policy;

	#define amqpvalue_create_terminus_expiry_policy amqpvalue_create_symbol
	#define amqpvalue_get_terminus_expiry_policy amqpvalue_get_symbol

	#define terminus_expiry_policy_link_detach link_detach
	#define terminus_expiry_policy_session_end session_end
	#define terminus_expiry_policy_connection_close connection_close
	#define terminus_expiry_policy_never never

/* node-properties */

	typedef fields node_properties;

	#define amqpvalue_create_node_properties amqpvalue_create_fields
	#define amqpvalue_get_node_properties amqpvalue_get_fields


/* filter-set */

	typedef AMQP_VALUE filter_set;

	#define amqpvalue_create_filter_set amqpvalue_clone
	#define amqpvalue_get_filter_set amqpvalue_get_map


/* source */

	typedef void* SOURCE_HANDLE;

	extern SOURCE_HANDLE source_create(void);
	extern SOURCE_HANDLE source_clone(SOURCE_HANDLE value);
	extern void source_destroy(SOURCE_HANDLE source);
	extern bool is_source_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_source(AMQP_VALUE value, SOURCE_HANDLE* SOURCE_handle);
	extern AMQP_VALUE amqpvalue_create_source(SOURCE_HANDLE source);

	extern int source_get_address(SOURCE_HANDLE source, AMQP_VALUE* address_value);
	extern int source_set_address(SOURCE_HANDLE source, AMQP_VALUE address_value);
	extern int source_get_durable(SOURCE_HANDLE source, terminus_durability* durable_value);
	extern int source_set_durable(SOURCE_HANDLE source, terminus_durability durable_value);
	extern int source_get_expiry_policy(SOURCE_HANDLE source, terminus_expiry_policy* expiry_policy_value);
	extern int source_set_expiry_policy(SOURCE_HANDLE source, terminus_expiry_policy expiry_policy_value);
	extern int source_get_timeout(SOURCE_HANDLE source, seconds* timeout_value);
	extern int source_set_timeout(SOURCE_HANDLE source, seconds timeout_value);
	extern int source_get_dynamic(SOURCE_HANDLE source, bool* dynamic_value);
	extern int source_set_dynamic(SOURCE_HANDLE source, bool dynamic_value);
	extern int source_get_dynamic_node_properties(SOURCE_HANDLE source, node_properties* dynamic_node_properties_value);
	extern int source_set_dynamic_node_properties(SOURCE_HANDLE source, node_properties dynamic_node_properties_value);
	extern int source_get_distribution_mode(SOURCE_HANDLE source, const char** distribution_mode_value);
	extern int source_set_distribution_mode(SOURCE_HANDLE source, const char* distribution_mode_value);
	extern int source_get_filter(SOURCE_HANDLE source, filter_set* filter_value);
	extern int source_set_filter(SOURCE_HANDLE source, filter_set filter_value);
	extern int source_get_default_outcome(SOURCE_HANDLE source, AMQP_VALUE* default_outcome_value);
	extern int source_set_default_outcome(SOURCE_HANDLE source, AMQP_VALUE default_outcome_value);
	extern int source_get_outcomes(SOURCE_HANDLE source, const char** outcomes_value);
	extern int source_set_outcomes(SOURCE_HANDLE source, const char* outcomes_value);
	extern int source_get_capabilities(SOURCE_HANDLE source, const char** capabilities_value);
	extern int source_set_capabilities(SOURCE_HANDLE source, const char* capabilities_value);

/* target */

	typedef void* TARGET_HANDLE;

	extern TARGET_HANDLE target_create(void);
	extern TARGET_HANDLE target_clone(TARGET_HANDLE value);
	extern void target_destroy(TARGET_HANDLE target);
	extern bool is_target_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_target(AMQP_VALUE value, TARGET_HANDLE* TARGET_handle);
	extern AMQP_VALUE amqpvalue_create_target(TARGET_HANDLE target);

	extern int target_get_address(TARGET_HANDLE target, AMQP_VALUE* address_value);
	extern int target_set_address(TARGET_HANDLE target, AMQP_VALUE address_value);
	extern int target_get_durable(TARGET_HANDLE target, terminus_durability* durable_value);
	extern int target_set_durable(TARGET_HANDLE target, terminus_durability durable_value);
	extern int target_get_expiry_policy(TARGET_HANDLE target, terminus_expiry_policy* expiry_policy_value);
	extern int target_set_expiry_policy(TARGET_HANDLE target, terminus_expiry_policy expiry_policy_value);
	extern int target_get_timeout(TARGET_HANDLE target, seconds* timeout_value);
	extern int target_set_timeout(TARGET_HANDLE target, seconds timeout_value);
	extern int target_get_dynamic(TARGET_HANDLE target, bool* dynamic_value);
	extern int target_set_dynamic(TARGET_HANDLE target, bool dynamic_value);
	extern int target_get_dynamic_node_properties(TARGET_HANDLE target, node_properties* dynamic_node_properties_value);
	extern int target_set_dynamic_node_properties(TARGET_HANDLE target, node_properties dynamic_node_properties_value);
	extern int target_get_capabilities(TARGET_HANDLE target, const char** capabilities_value);
	extern int target_set_capabilities(TARGET_HANDLE target, const char* capabilities_value);

/* annotations */

	typedef AMQP_VALUE annotations;

	#define amqpvalue_create_annotations amqpvalue_clone
	#define amqpvalue_get_annotations amqpvalue_get_map


/* message-id-ulong */

	typedef uint64_t message_id_ulong;

	#define amqpvalue_create_message_id_ulong amqpvalue_create_ulong
	#define amqpvalue_get_message_id_ulong amqpvalue_get_ulong


/* message-id-uuid */

	typedef uuid message_id_uuid;

	#define amqpvalue_create_message_id_uuid amqpvalue_create_uuid
	#define amqpvalue_get_message_id_uuid amqpvalue_get_uuid


/* message-id-binary */

	typedef amqp_binary message_id_binary;

	#define amqpvalue_create_message_id_binary amqpvalue_create_binary
	#define amqpvalue_get_message_id_binary amqpvalue_get_binary


/* message-id-string */

	typedef const char* message_id_string;

	#define amqpvalue_create_message_id_string amqpvalue_create_string
	#define amqpvalue_get_message_id_string amqpvalue_get_string


/* address-string */

	typedef const char* address_string;

	#define amqpvalue_create_address_string amqpvalue_create_string
	#define amqpvalue_get_address_string amqpvalue_get_string


/* header */

	typedef void* HEADER_HANDLE;

	extern HEADER_HANDLE header_create(void);
	extern HEADER_HANDLE header_clone(HEADER_HANDLE value);
	extern void header_destroy(HEADER_HANDLE header);
	extern bool is_header_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_header(AMQP_VALUE value, HEADER_HANDLE* HEADER_handle);
	extern AMQP_VALUE amqpvalue_create_header(HEADER_HANDLE header);

	extern int header_get_durable(HEADER_HANDLE header, bool* durable_value);
	extern int header_set_durable(HEADER_HANDLE header, bool durable_value);
	extern int header_get_priority(HEADER_HANDLE header, uint8_t* priority_value);
	extern int header_set_priority(HEADER_HANDLE header, uint8_t priority_value);
	extern int header_get_ttl(HEADER_HANDLE header, milliseconds* ttl_value);
	extern int header_set_ttl(HEADER_HANDLE header, milliseconds ttl_value);
	extern int header_get_first_acquirer(HEADER_HANDLE header, bool* first_acquirer_value);
	extern int header_set_first_acquirer(HEADER_HANDLE header, bool first_acquirer_value);
	extern int header_get_delivery_count(HEADER_HANDLE header, uint32_t* delivery_count_value);
	extern int header_set_delivery_count(HEADER_HANDLE header, uint32_t delivery_count_value);

/* properties */

	typedef void* PROPERTIES_HANDLE;

	extern PROPERTIES_HANDLE properties_create(void);
	extern PROPERTIES_HANDLE properties_clone(PROPERTIES_HANDLE value);
	extern void properties_destroy(PROPERTIES_HANDLE properties);
	extern bool is_properties_type_by_descriptor(AMQP_VALUE descriptor);
	extern int amqpvalue_get_properties(AMQP_VALUE value, PROPERTIES_HANDLE* PROPERTIES_handle);
	extern AMQP_VALUE amqpvalue_create_properties(PROPERTIES_HANDLE properties);

	extern int properties_get_message_id(PROPERTIES_HANDLE properties, AMQP_VALUE* message_id_value);
	extern int properties_set_message_id(PROPERTIES_HANDLE properties, AMQP_VALUE message_id_value);
	extern int properties_get_user_id(PROPERTIES_HANDLE properties, amqp_binary* user_id_value);
	extern int properties_set_user_id(PROPERTIES_HANDLE properties, amqp_binary user_id_value);
	extern int properties_get_to(PROPERTIES_HANDLE properties, AMQP_VALUE* to_value);
	extern int properties_set_to(PROPERTIES_HANDLE properties, AMQP_VALUE to_value);
	extern int properties_get_subject(PROPERTIES_HANDLE properties, const char** subject_value);
	extern int properties_set_subject(PROPERTIES_HANDLE properties, const char* subject_value);
	extern int properties_get_reply_to(PROPERTIES_HANDLE properties, AMQP_VALUE* reply_to_value);
	extern int properties_set_reply_to(PROPERTIES_HANDLE properties, AMQP_VALUE reply_to_value);
	extern int properties_get_correlation_id(PROPERTIES_HANDLE properties, AMQP_VALUE* correlation_id_value);
	extern int properties_set_correlation_id(PROPERTIES_HANDLE properties, AMQP_VALUE correlation_id_value);
	extern int properties_get_content_type(PROPERTIES_HANDLE properties, const char** content_type_value);
	extern int properties_set_content_type(PROPERTIES_HANDLE properties, const char* content_type_value);
	extern int properties_get_content_encoding(PROPERTIES_HANDLE properties, const char** content_encoding_value);
	extern int properties_set_content_encoding(PROPERTIES_HANDLE properties, const char* content_encoding_value);
	extern int properties_get_absolute_expiry_time(PROPERTIES_HANDLE properties, timestamp* absolute_expiry_time_value);
	extern int properties_set_absolute_expiry_time(PROPERTIES_HANDLE properties, timestamp absolute_expiry_time_value);
	extern int properties_get_creation_time(PROPERTIES_HANDLE properties, timestamp* creation_time_value);
	extern int properties_set_creation_time(PROPERTIES_HANDLE properties, timestamp creation_time_value);
	extern int properties_get_group_id(PROPERTIES_HANDLE properties, const char** group_id_value);
	extern int properties_set_group_id(PROPERTIES_HANDLE properties, const char* group_id_value);
	extern int properties_get_group_sequence(PROPERTIES_HANDLE properties, sequence_no* group_sequence_value);
	extern int properties_set_group_sequence(PROPERTIES_HANDLE properties, sequence_no group_sequence_value);
	extern int properties_get_reply_to_group_id(PROPERTIES_HANDLE properties, const char** reply_to_group_id_value);
	extern int properties_set_reply_to_group_id(PROPERTIES_HANDLE properties, const char* reply_to_group_id_value);


#ifdef __cplusplus
}
#endif

#endif /* AMQP_DEFINITIONS_H */
