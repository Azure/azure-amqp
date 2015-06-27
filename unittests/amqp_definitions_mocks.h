

#ifndef AMQP_DEFINITIONS_MOCKS_H
#define AMQP_DEFINITIONS_MOCKS_H

#include "amqp_definitions.h"

/* role */

/* sender-settle-mode */

/* receiver-settle-mode */

/* handle */

/* seconds */

/* milliseconds */

/* delivery-tag */

/* sequence-no */

/* delivery-number */

/* transfer-number */

/* message-format */

/* ietf-language-tag */

/* fields */

/* error */

static const ERROR_HANDLE test_error_handle = (ERROR_HANDLE)16962;
static const AMQP_VALUE test_error_amqp_value = (AMQP_VALUE)16963;
/* amqp-error */

/* connection-error */

/* session-error */

/* link-error */

/* open */

static const OPEN_HANDLE test_open_handle = (OPEN_HANDLE)16963;
static const AMQP_VALUE test_open_amqp_value = (AMQP_VALUE)16964;
/* begin */

static const BEGIN_HANDLE test_begin_handle = (BEGIN_HANDLE)16964;
static const AMQP_VALUE test_begin_amqp_value = (AMQP_VALUE)16965;
/* attach */

static const ATTACH_HANDLE test_attach_handle = (ATTACH_HANDLE)16965;
static const AMQP_VALUE test_attach_amqp_value = (AMQP_VALUE)16966;
/* flow */

static const FLOW_HANDLE test_flow_handle = (FLOW_HANDLE)16966;
static const AMQP_VALUE test_flow_amqp_value = (AMQP_VALUE)16967;
/* transfer */

static const TRANSFER_HANDLE test_transfer_handle = (TRANSFER_HANDLE)16967;
static const AMQP_VALUE test_transfer_amqp_value = (AMQP_VALUE)16968;
/* disposition */

static const DISPOSITION_HANDLE test_disposition_handle = (DISPOSITION_HANDLE)16968;
static const AMQP_VALUE test_disposition_amqp_value = (AMQP_VALUE)16969;
/* detach */

static const DETACH_HANDLE test_detach_handle = (DETACH_HANDLE)16969;
static const AMQP_VALUE test_detach_amqp_value = (AMQP_VALUE)16970;
/* end */

static const END_HANDLE test_end_handle = (END_HANDLE)16970;
static const AMQP_VALUE test_end_amqp_value = (AMQP_VALUE)16971;
/* close */

static const CLOSE_HANDLE test_close_handle = (CLOSE_HANDLE)16971;
static const AMQP_VALUE test_close_amqp_value = (AMQP_VALUE)16972;

TYPED_MOCK_CLASS(amqp_definitions_mocks, CGlobalMock)
{
public:
/* role */

/* sender-settle-mode */

/* receiver-settle-mode */

/* handle */

/* seconds */

/* milliseconds */

/* delivery-tag */

/* sequence-no */

/* delivery-number */

/* transfer-number */

/* message-format */

/* ietf-language-tag */

/* fields */

/* error */

	MOCK_STATIC_METHOD_1(,ERROR_HANDLE, error_create, uint32_t, condition_value);
	MOCK_METHOD_END(ERROR_HANDLE, test_error_handle);
	MOCK_STATIC_METHOD_1(, void, error_destroy, ERROR_HANDLE, error);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_error, ERROR_HANDLE, error);
	MOCK_METHOD_END(AMQP_VALUE, test_error_amqp_value);

	MOCK_STATIC_METHOD_2(, int, error_get_condition, ERROR_HANDLE, error, uint32_t*, condition_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, error_set_condition, ERROR_HANDLE, error, uint32_t, condition_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, error_get_description, ERROR_HANDLE, error, const char**, description_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, error_set_description, ERROR_HANDLE, error, const char*, description_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, error_get_info, ERROR_HANDLE, error, fields*, info_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, error_set_info, ERROR_HANDLE, error, fields, info_value);
	MOCK_METHOD_END(int, 0);

/* amqp-error */

/* connection-error */

/* session-error */

/* link-error */

/* open */

	MOCK_STATIC_METHOD_1(,OPEN_HANDLE, open_create, const char*, container_id_value);
	MOCK_METHOD_END(OPEN_HANDLE, test_open_handle);
	MOCK_STATIC_METHOD_1(, void, open_destroy, OPEN_HANDLE, open);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_open, OPEN_HANDLE, open);
	MOCK_METHOD_END(AMQP_VALUE, test_open_amqp_value);

	MOCK_STATIC_METHOD_2(, int, open_get_container_id, OPEN_HANDLE, open, const char**, container_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_container_id, OPEN_HANDLE, open, const char*, container_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_hostname, OPEN_HANDLE, open, const char**, hostname_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_hostname, OPEN_HANDLE, open, const char*, hostname_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_max_frame_size, OPEN_HANDLE, open, uint32_t*, max_frame_size_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_max_frame_size, OPEN_HANDLE, open, uint32_t, max_frame_size_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_channel_max, OPEN_HANDLE, open, uint16_t*, channel_max_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_channel_max, OPEN_HANDLE, open, uint16_t, channel_max_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_idle_time_out, OPEN_HANDLE, open, milliseconds*, idle_time_out_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_idle_time_out, OPEN_HANDLE, open, milliseconds, idle_time_out_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_outgoing_locales, OPEN_HANDLE, open, ietf_language_tag*, outgoing_locales_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_outgoing_locales, OPEN_HANDLE, open, ietf_language_tag, outgoing_locales_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_incoming_locales, OPEN_HANDLE, open, ietf_language_tag*, incoming_locales_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_incoming_locales, OPEN_HANDLE, open, ietf_language_tag, incoming_locales_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_offered_capabilities, OPEN_HANDLE, open, uint32_t*, offered_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_offered_capabilities, OPEN_HANDLE, open, uint32_t, offered_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_desired_capabilities, OPEN_HANDLE, open, uint32_t*, desired_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_desired_capabilities, OPEN_HANDLE, open, uint32_t, desired_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_get_properties, OPEN_HANDLE, open, fields*, properties_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, open_set_properties, OPEN_HANDLE, open, fields, properties_value);
	MOCK_METHOD_END(int, 0);

/* begin */

	MOCK_STATIC_METHOD_3(,BEGIN_HANDLE, begin_create, transfer_number, next_outgoing_id_value, uint32_t, incoming_window_value, uint32_t, outgoing_window_value);
	MOCK_METHOD_END(BEGIN_HANDLE, test_begin_handle);
	MOCK_STATIC_METHOD_1(, void, begin_destroy, BEGIN_HANDLE, begin);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_begin, BEGIN_HANDLE, begin);
	MOCK_METHOD_END(AMQP_VALUE, test_begin_amqp_value);

	MOCK_STATIC_METHOD_2(, int, begin_get_remote_channel, BEGIN_HANDLE, begin, uint16_t*, remote_channel_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_remote_channel, BEGIN_HANDLE, begin, uint16_t, remote_channel_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_next_outgoing_id, BEGIN_HANDLE, begin, transfer_number*, next_outgoing_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_next_outgoing_id, BEGIN_HANDLE, begin, transfer_number, next_outgoing_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_incoming_window, BEGIN_HANDLE, begin, uint32_t*, incoming_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_incoming_window, BEGIN_HANDLE, begin, uint32_t, incoming_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_outgoing_window, BEGIN_HANDLE, begin, uint32_t*, outgoing_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_outgoing_window, BEGIN_HANDLE, begin, uint32_t, outgoing_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_handle_max, BEGIN_HANDLE, begin, handle*, handle_max_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_handle_max, BEGIN_HANDLE, begin, handle, handle_max_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_offered_capabilities, BEGIN_HANDLE, begin, uint32_t*, offered_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_offered_capabilities, BEGIN_HANDLE, begin, uint32_t, offered_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_desired_capabilities, BEGIN_HANDLE, begin, uint32_t*, desired_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_desired_capabilities, BEGIN_HANDLE, begin, uint32_t, desired_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_get_properties, BEGIN_HANDLE, begin, fields*, properties_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, begin_set_properties, BEGIN_HANDLE, begin, fields, properties_value);
	MOCK_METHOD_END(int, 0);

/* attach */

	MOCK_STATIC_METHOD_3(,ATTACH_HANDLE, attach_create, const char*, name_value, handle, handle_value, role, role_value);
	MOCK_METHOD_END(ATTACH_HANDLE, test_attach_handle);
	MOCK_STATIC_METHOD_1(, void, attach_destroy, ATTACH_HANDLE, attach);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_attach, ATTACH_HANDLE, attach);
	MOCK_METHOD_END(AMQP_VALUE, test_attach_amqp_value);

	MOCK_STATIC_METHOD_2(, int, attach_get_name, ATTACH_HANDLE, attach, const char**, name_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_name, ATTACH_HANDLE, attach, const char*, name_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_handle, ATTACH_HANDLE, attach, handle*, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_handle, ATTACH_HANDLE, attach, handle, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_role, ATTACH_HANDLE, attach, role*, role_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_role, ATTACH_HANDLE, attach, role, role_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_snd_settle_mode, ATTACH_HANDLE, attach, sender_settle_mode*, snd_settle_mode_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_snd_settle_mode, ATTACH_HANDLE, attach, sender_settle_mode, snd_settle_mode_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_rcv_settle_mode, ATTACH_HANDLE, attach, receiver_settle_mode*, rcv_settle_mode_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_rcv_settle_mode, ATTACH_HANDLE, attach, receiver_settle_mode, rcv_settle_mode_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_source, ATTACH_HANDLE, attach, AMQP_VALUE*, source_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_source, ATTACH_HANDLE, attach, AMQP_VALUE, source_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_target, ATTACH_HANDLE, attach, AMQP_VALUE*, target_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_target, ATTACH_HANDLE, attach, AMQP_VALUE, target_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_unsettled, ATTACH_HANDLE, attach, AMQP_VALUE*, unsettled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_unsettled, ATTACH_HANDLE, attach, AMQP_VALUE, unsettled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_incomplete_unsettled, ATTACH_HANDLE, attach, bool*, incomplete_unsettled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_incomplete_unsettled, ATTACH_HANDLE, attach, bool, incomplete_unsettled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_initial_delivery_count, ATTACH_HANDLE, attach, sequence_no*, initial_delivery_count_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_initial_delivery_count, ATTACH_HANDLE, attach, sequence_no, initial_delivery_count_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_max_message_size, ATTACH_HANDLE, attach, uint64_t*, max_message_size_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_max_message_size, ATTACH_HANDLE, attach, uint64_t, max_message_size_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_offered_capabilities, ATTACH_HANDLE, attach, uint32_t*, offered_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_offered_capabilities, ATTACH_HANDLE, attach, uint32_t, offered_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_desired_capabilities, ATTACH_HANDLE, attach, uint32_t*, desired_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_desired_capabilities, ATTACH_HANDLE, attach, uint32_t, desired_capabilities_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_get_properties, ATTACH_HANDLE, attach, fields*, properties_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, attach_set_properties, ATTACH_HANDLE, attach, fields, properties_value);
	MOCK_METHOD_END(int, 0);

/* flow */

	MOCK_STATIC_METHOD_3(,FLOW_HANDLE, flow_create, uint32_t, incoming_window_value, transfer_number, next_outgoing_id_value, uint32_t, outgoing_window_value);
	MOCK_METHOD_END(FLOW_HANDLE, test_flow_handle);
	MOCK_STATIC_METHOD_1(, void, flow_destroy, FLOW_HANDLE, flow);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_flow, FLOW_HANDLE, flow);
	MOCK_METHOD_END(AMQP_VALUE, test_flow_amqp_value);

	MOCK_STATIC_METHOD_2(, int, flow_get_next_incoming_id, FLOW_HANDLE, flow, transfer_number*, next_incoming_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_next_incoming_id, FLOW_HANDLE, flow, transfer_number, next_incoming_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_incoming_window, FLOW_HANDLE, flow, uint32_t*, incoming_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_incoming_window, FLOW_HANDLE, flow, uint32_t, incoming_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_next_outgoing_id, FLOW_HANDLE, flow, transfer_number*, next_outgoing_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_next_outgoing_id, FLOW_HANDLE, flow, transfer_number, next_outgoing_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_outgoing_window, FLOW_HANDLE, flow, uint32_t*, outgoing_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_outgoing_window, FLOW_HANDLE, flow, uint32_t, outgoing_window_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_handle, FLOW_HANDLE, flow, handle*, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_handle, FLOW_HANDLE, flow, handle, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_delivery_count, FLOW_HANDLE, flow, sequence_no*, delivery_count_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_delivery_count, FLOW_HANDLE, flow, sequence_no, delivery_count_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_link_credit, FLOW_HANDLE, flow, uint32_t*, link_credit_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_link_credit, FLOW_HANDLE, flow, uint32_t, link_credit_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_available, FLOW_HANDLE, flow, uint32_t*, available_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_available, FLOW_HANDLE, flow, uint32_t, available_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_drain, FLOW_HANDLE, flow, bool*, drain_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_drain, FLOW_HANDLE, flow, bool, drain_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_echo, FLOW_HANDLE, flow, bool*, echo_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_echo, FLOW_HANDLE, flow, bool, echo_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_get_properties, FLOW_HANDLE, flow, fields*, properties_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, flow_set_properties, FLOW_HANDLE, flow, fields, properties_value);
	MOCK_METHOD_END(int, 0);

/* transfer */

	MOCK_STATIC_METHOD_1(,TRANSFER_HANDLE, transfer_create, handle, handle_value);
	MOCK_METHOD_END(TRANSFER_HANDLE, test_transfer_handle);
	MOCK_STATIC_METHOD_1(, void, transfer_destroy, TRANSFER_HANDLE, transfer);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_transfer, TRANSFER_HANDLE, transfer);
	MOCK_METHOD_END(AMQP_VALUE, test_transfer_amqp_value);

	MOCK_STATIC_METHOD_2(, int, transfer_get_handle, TRANSFER_HANDLE, transfer, handle*, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_handle, TRANSFER_HANDLE, transfer, handle, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_delivery_id, TRANSFER_HANDLE, transfer, delivery_number*, delivery_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_delivery_id, TRANSFER_HANDLE, transfer, delivery_number, delivery_id_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_delivery_tag, TRANSFER_HANDLE, transfer, delivery_tag*, delivery_tag_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_delivery_tag, TRANSFER_HANDLE, transfer, delivery_tag, delivery_tag_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_message_format, TRANSFER_HANDLE, transfer, message_format*, message_format_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_message_format, TRANSFER_HANDLE, transfer, message_format, message_format_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_settled, TRANSFER_HANDLE, transfer, bool*, settled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_settled, TRANSFER_HANDLE, transfer, bool, settled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_more, TRANSFER_HANDLE, transfer, bool*, more_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_more, TRANSFER_HANDLE, transfer, bool, more_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_rcv_settle_mode, TRANSFER_HANDLE, transfer, receiver_settle_mode*, rcv_settle_mode_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_rcv_settle_mode, TRANSFER_HANDLE, transfer, receiver_settle_mode, rcv_settle_mode_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_state, TRANSFER_HANDLE, transfer, AMQP_VALUE*, state_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_state, TRANSFER_HANDLE, transfer, AMQP_VALUE, state_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_resume, TRANSFER_HANDLE, transfer, bool*, resume_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_resume, TRANSFER_HANDLE, transfer, bool, resume_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_aborted, TRANSFER_HANDLE, transfer, bool*, aborted_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_aborted, TRANSFER_HANDLE, transfer, bool, aborted_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_get_batchable, TRANSFER_HANDLE, transfer, bool*, batchable_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, transfer_set_batchable, TRANSFER_HANDLE, transfer, bool, batchable_value);
	MOCK_METHOD_END(int, 0);

/* disposition */

	MOCK_STATIC_METHOD_2(,DISPOSITION_HANDLE, disposition_create, role, role_value, delivery_number, first_value);
	MOCK_METHOD_END(DISPOSITION_HANDLE, test_disposition_handle);
	MOCK_STATIC_METHOD_1(, void, disposition_destroy, DISPOSITION_HANDLE, disposition);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_disposition, DISPOSITION_HANDLE, disposition);
	MOCK_METHOD_END(AMQP_VALUE, test_disposition_amqp_value);

	MOCK_STATIC_METHOD_2(, int, disposition_get_role, DISPOSITION_HANDLE, disposition, role*, role_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_set_role, DISPOSITION_HANDLE, disposition, role, role_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_get_first, DISPOSITION_HANDLE, disposition, delivery_number*, first_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_set_first, DISPOSITION_HANDLE, disposition, delivery_number, first_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_get_last, DISPOSITION_HANDLE, disposition, delivery_number*, last_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_set_last, DISPOSITION_HANDLE, disposition, delivery_number, last_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_get_settled, DISPOSITION_HANDLE, disposition, bool*, settled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_set_settled, DISPOSITION_HANDLE, disposition, bool, settled_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_get_state, DISPOSITION_HANDLE, disposition, AMQP_VALUE*, state_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_set_state, DISPOSITION_HANDLE, disposition, AMQP_VALUE, state_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_get_batchable, DISPOSITION_HANDLE, disposition, bool*, batchable_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, disposition_set_batchable, DISPOSITION_HANDLE, disposition, bool, batchable_value);
	MOCK_METHOD_END(int, 0);

/* detach */

	MOCK_STATIC_METHOD_1(,DETACH_HANDLE, detach_create, handle, handle_value);
	MOCK_METHOD_END(DETACH_HANDLE, test_detach_handle);
	MOCK_STATIC_METHOD_1(, void, detach_destroy, DETACH_HANDLE, detach);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_detach, DETACH_HANDLE, detach);
	MOCK_METHOD_END(AMQP_VALUE, test_detach_amqp_value);

	MOCK_STATIC_METHOD_2(, int, detach_get_handle, DETACH_HANDLE, detach, handle*, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, detach_set_handle, DETACH_HANDLE, detach, handle, handle_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, detach_get_closed, DETACH_HANDLE, detach, bool*, closed_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, detach_set_closed, DETACH_HANDLE, detach, bool, closed_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, detach_get_error, DETACH_HANDLE, detach, ERROR_HANDLE*, error_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, detach_set_error, DETACH_HANDLE, detach, ERROR_HANDLE, error_value);
	MOCK_METHOD_END(int, 0);

/* end */

	MOCK_STATIC_METHOD_0(,END_HANDLE, end_create);
	MOCK_METHOD_END(END_HANDLE, test_end_handle);
	MOCK_STATIC_METHOD_1(, void, end_destroy, END_HANDLE, end);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_end, END_HANDLE, end);
	MOCK_METHOD_END(AMQP_VALUE, test_end_amqp_value);

	MOCK_STATIC_METHOD_2(, int, end_get_error, END_HANDLE, end, ERROR_HANDLE*, error_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, end_set_error, END_HANDLE, end, ERROR_HANDLE, error_value);
	MOCK_METHOD_END(int, 0);

/* close */

	MOCK_STATIC_METHOD_0(,CLOSE_HANDLE, close_create);
	MOCK_METHOD_END(CLOSE_HANDLE, test_close_handle);
	MOCK_STATIC_METHOD_1(, void, close_destroy, CLOSE_HANDLE, close);
	MOCK_VOID_METHOD_END();
	MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_close, CLOSE_HANDLE, close);
	MOCK_METHOD_END(AMQP_VALUE, test_close_amqp_value);

	MOCK_STATIC_METHOD_2(, int, close_get_error, CLOSE_HANDLE, close, ERROR_HANDLE*, error_value);
	MOCK_METHOD_END(int, 0);
	MOCK_STATIC_METHOD_2(, int, close_set_error, CLOSE_HANDLE, close, ERROR_HANDLE, error_value);
	MOCK_METHOD_END(int, 0);

};

/* role */

/* sender-settle-mode */

/* receiver-settle-mode */

/* handle */

/* seconds */

/* milliseconds */

/* delivery-tag */

/* sequence-no */

/* delivery-number */

/* transfer-number */

/* message-format */

/* ietf-language-tag */

/* fields */

/* error */

	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, ,ERROR_HANDLE, error_create, uint32_t, condition_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, error_destroy, ERROR_HANDLE, error);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_error, ERROR_HANDLE, error);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, error_get_condition, ERROR_HANDLE, error, uint32_t*, condition_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, error_set_condition, ERROR_HANDLE, error, uint32_t, condition_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, error_get_description, ERROR_HANDLE, error, const char**, description_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, error_set_description, ERROR_HANDLE, error, const char*, description_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, error_get_info, ERROR_HANDLE, error, fields*, info_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, error_set_info, ERROR_HANDLE, error, fields, info_value);

/* amqp-error */

/* connection-error */

/* session-error */

/* link-error */

/* open */

	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, ,OPEN_HANDLE, open_create, const char*, container_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, open_destroy, OPEN_HANDLE, open);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_open, OPEN_HANDLE, open);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_container_id, OPEN_HANDLE, open, const char**, container_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_container_id, OPEN_HANDLE, open, const char*, container_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_hostname, OPEN_HANDLE, open, const char**, hostname_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_hostname, OPEN_HANDLE, open, const char*, hostname_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_max_frame_size, OPEN_HANDLE, open, uint32_t*, max_frame_size_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_max_frame_size, OPEN_HANDLE, open, uint32_t, max_frame_size_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_channel_max, OPEN_HANDLE, open, uint16_t*, channel_max_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_channel_max, OPEN_HANDLE, open, uint16_t, channel_max_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_idle_time_out, OPEN_HANDLE, open, milliseconds*, idle_time_out_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_idle_time_out, OPEN_HANDLE, open, milliseconds, idle_time_out_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_outgoing_locales, OPEN_HANDLE, open, ietf_language_tag*, outgoing_locales_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_outgoing_locales, OPEN_HANDLE, open, ietf_language_tag, outgoing_locales_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_incoming_locales, OPEN_HANDLE, open, ietf_language_tag*, incoming_locales_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_incoming_locales, OPEN_HANDLE, open, ietf_language_tag, incoming_locales_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_offered_capabilities, OPEN_HANDLE, open, uint32_t*, offered_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_offered_capabilities, OPEN_HANDLE, open, uint32_t, offered_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_desired_capabilities, OPEN_HANDLE, open, uint32_t*, desired_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_desired_capabilities, OPEN_HANDLE, open, uint32_t, desired_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_get_properties, OPEN_HANDLE, open, fields*, properties_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, open_set_properties, OPEN_HANDLE, open, fields, properties_value);

/* begin */

	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_definitions_mocks, ,BEGIN_HANDLE, begin_create, transfer_number, next_outgoing_id_value, uint32_t, incoming_window_value, uint32_t, outgoing_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, begin_destroy, BEGIN_HANDLE, begin);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_begin, BEGIN_HANDLE, begin);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_remote_channel, BEGIN_HANDLE, begin, uint16_t*, remote_channel_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_remote_channel, BEGIN_HANDLE, begin, uint16_t, remote_channel_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_next_outgoing_id, BEGIN_HANDLE, begin, transfer_number*, next_outgoing_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_next_outgoing_id, BEGIN_HANDLE, begin, transfer_number, next_outgoing_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_incoming_window, BEGIN_HANDLE, begin, uint32_t*, incoming_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_incoming_window, BEGIN_HANDLE, begin, uint32_t, incoming_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_outgoing_window, BEGIN_HANDLE, begin, uint32_t*, outgoing_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_outgoing_window, BEGIN_HANDLE, begin, uint32_t, outgoing_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_handle_max, BEGIN_HANDLE, begin, handle*, handle_max_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_handle_max, BEGIN_HANDLE, begin, handle, handle_max_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_offered_capabilities, BEGIN_HANDLE, begin, uint32_t*, offered_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_offered_capabilities, BEGIN_HANDLE, begin, uint32_t, offered_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_desired_capabilities, BEGIN_HANDLE, begin, uint32_t*, desired_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_desired_capabilities, BEGIN_HANDLE, begin, uint32_t, desired_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_get_properties, BEGIN_HANDLE, begin, fields*, properties_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, begin_set_properties, BEGIN_HANDLE, begin, fields, properties_value);

/* attach */

	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_definitions_mocks, ,ATTACH_HANDLE, attach_create, const char*, name_value, handle, handle_value, role, role_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, attach_destroy, ATTACH_HANDLE, attach);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_attach, ATTACH_HANDLE, attach);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_name, ATTACH_HANDLE, attach, const char**, name_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_name, ATTACH_HANDLE, attach, const char*, name_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_handle, ATTACH_HANDLE, attach, handle*, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_handle, ATTACH_HANDLE, attach, handle, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_role, ATTACH_HANDLE, attach, role*, role_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_role, ATTACH_HANDLE, attach, role, role_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_snd_settle_mode, ATTACH_HANDLE, attach, sender_settle_mode*, snd_settle_mode_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_snd_settle_mode, ATTACH_HANDLE, attach, sender_settle_mode, snd_settle_mode_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_rcv_settle_mode, ATTACH_HANDLE, attach, receiver_settle_mode*, rcv_settle_mode_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_rcv_settle_mode, ATTACH_HANDLE, attach, receiver_settle_mode, rcv_settle_mode_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_source, ATTACH_HANDLE, attach, AMQP_VALUE*, source_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_source, ATTACH_HANDLE, attach, AMQP_VALUE, source_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_target, ATTACH_HANDLE, attach, AMQP_VALUE*, target_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_target, ATTACH_HANDLE, attach, AMQP_VALUE, target_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_unsettled, ATTACH_HANDLE, attach, AMQP_VALUE*, unsettled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_unsettled, ATTACH_HANDLE, attach, AMQP_VALUE, unsettled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_incomplete_unsettled, ATTACH_HANDLE, attach, bool*, incomplete_unsettled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_incomplete_unsettled, ATTACH_HANDLE, attach, bool, incomplete_unsettled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_initial_delivery_count, ATTACH_HANDLE, attach, sequence_no*, initial_delivery_count_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_initial_delivery_count, ATTACH_HANDLE, attach, sequence_no, initial_delivery_count_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_max_message_size, ATTACH_HANDLE, attach, uint64_t*, max_message_size_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_max_message_size, ATTACH_HANDLE, attach, uint64_t, max_message_size_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_offered_capabilities, ATTACH_HANDLE, attach, uint32_t*, offered_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_offered_capabilities, ATTACH_HANDLE, attach, uint32_t, offered_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_desired_capabilities, ATTACH_HANDLE, attach, uint32_t*, desired_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_desired_capabilities, ATTACH_HANDLE, attach, uint32_t, desired_capabilities_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_get_properties, ATTACH_HANDLE, attach, fields*, properties_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, attach_set_properties, ATTACH_HANDLE, attach, fields, properties_value);

/* flow */

	DECLARE_GLOBAL_MOCK_METHOD_3(amqp_definitions_mocks, ,FLOW_HANDLE, flow_create, uint32_t, incoming_window_value, transfer_number, next_outgoing_id_value, uint32_t, outgoing_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, flow_destroy, FLOW_HANDLE, flow);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_flow, FLOW_HANDLE, flow);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_next_incoming_id, FLOW_HANDLE, flow, transfer_number*, next_incoming_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_next_incoming_id, FLOW_HANDLE, flow, transfer_number, next_incoming_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_incoming_window, FLOW_HANDLE, flow, uint32_t*, incoming_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_incoming_window, FLOW_HANDLE, flow, uint32_t, incoming_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_next_outgoing_id, FLOW_HANDLE, flow, transfer_number*, next_outgoing_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_next_outgoing_id, FLOW_HANDLE, flow, transfer_number, next_outgoing_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_outgoing_window, FLOW_HANDLE, flow, uint32_t*, outgoing_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_outgoing_window, FLOW_HANDLE, flow, uint32_t, outgoing_window_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_handle, FLOW_HANDLE, flow, handle*, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_handle, FLOW_HANDLE, flow, handle, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_delivery_count, FLOW_HANDLE, flow, sequence_no*, delivery_count_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_delivery_count, FLOW_HANDLE, flow, sequence_no, delivery_count_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_link_credit, FLOW_HANDLE, flow, uint32_t*, link_credit_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_link_credit, FLOW_HANDLE, flow, uint32_t, link_credit_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_available, FLOW_HANDLE, flow, uint32_t*, available_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_available, FLOW_HANDLE, flow, uint32_t, available_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_drain, FLOW_HANDLE, flow, bool*, drain_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_drain, FLOW_HANDLE, flow, bool, drain_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_echo, FLOW_HANDLE, flow, bool*, echo_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_echo, FLOW_HANDLE, flow, bool, echo_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_get_properties, FLOW_HANDLE, flow, fields*, properties_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, flow_set_properties, FLOW_HANDLE, flow, fields, properties_value);

/* transfer */

	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, ,TRANSFER_HANDLE, transfer_create, handle, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, transfer_destroy, TRANSFER_HANDLE, transfer);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_transfer, TRANSFER_HANDLE, transfer);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_handle, TRANSFER_HANDLE, transfer, handle*, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_handle, TRANSFER_HANDLE, transfer, handle, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_delivery_id, TRANSFER_HANDLE, transfer, delivery_number*, delivery_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_delivery_id, TRANSFER_HANDLE, transfer, delivery_number, delivery_id_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_delivery_tag, TRANSFER_HANDLE, transfer, delivery_tag*, delivery_tag_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_delivery_tag, TRANSFER_HANDLE, transfer, delivery_tag, delivery_tag_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_message_format, TRANSFER_HANDLE, transfer, message_format*, message_format_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_message_format, TRANSFER_HANDLE, transfer, message_format, message_format_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_settled, TRANSFER_HANDLE, transfer, bool*, settled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_settled, TRANSFER_HANDLE, transfer, bool, settled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_more, TRANSFER_HANDLE, transfer, bool*, more_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_more, TRANSFER_HANDLE, transfer, bool, more_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_rcv_settle_mode, TRANSFER_HANDLE, transfer, receiver_settle_mode*, rcv_settle_mode_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_rcv_settle_mode, TRANSFER_HANDLE, transfer, receiver_settle_mode, rcv_settle_mode_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_state, TRANSFER_HANDLE, transfer, AMQP_VALUE*, state_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_state, TRANSFER_HANDLE, transfer, AMQP_VALUE, state_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_resume, TRANSFER_HANDLE, transfer, bool*, resume_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_resume, TRANSFER_HANDLE, transfer, bool, resume_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_aborted, TRANSFER_HANDLE, transfer, bool*, aborted_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_aborted, TRANSFER_HANDLE, transfer, bool, aborted_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_get_batchable, TRANSFER_HANDLE, transfer, bool*, batchable_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, transfer_set_batchable, TRANSFER_HANDLE, transfer, bool, batchable_value);

/* disposition */

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, ,DISPOSITION_HANDLE, disposition_create, role, role_value, delivery_number, first_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, disposition_destroy, DISPOSITION_HANDLE, disposition);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_disposition, DISPOSITION_HANDLE, disposition);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_get_role, DISPOSITION_HANDLE, disposition, role*, role_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_set_role, DISPOSITION_HANDLE, disposition, role, role_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_get_first, DISPOSITION_HANDLE, disposition, delivery_number*, first_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_set_first, DISPOSITION_HANDLE, disposition, delivery_number, first_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_get_last, DISPOSITION_HANDLE, disposition, delivery_number*, last_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_set_last, DISPOSITION_HANDLE, disposition, delivery_number, last_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_get_settled, DISPOSITION_HANDLE, disposition, bool*, settled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_set_settled, DISPOSITION_HANDLE, disposition, bool, settled_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_get_state, DISPOSITION_HANDLE, disposition, AMQP_VALUE*, state_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_set_state, DISPOSITION_HANDLE, disposition, AMQP_VALUE, state_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_get_batchable, DISPOSITION_HANDLE, disposition, bool*, batchable_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, disposition_set_batchable, DISPOSITION_HANDLE, disposition, bool, batchable_value);

/* detach */

	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, ,DETACH_HANDLE, detach_create, handle, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, detach_destroy, DETACH_HANDLE, detach);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_detach, DETACH_HANDLE, detach);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, detach_get_handle, DETACH_HANDLE, detach, handle*, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, detach_set_handle, DETACH_HANDLE, detach, handle, handle_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, detach_get_closed, DETACH_HANDLE, detach, bool*, closed_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, detach_set_closed, DETACH_HANDLE, detach, bool, closed_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, detach_get_error, DETACH_HANDLE, detach, ERROR_HANDLE*, error_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, detach_set_error, DETACH_HANDLE, detach, ERROR_HANDLE, error_value);

/* end */

	DECLARE_GLOBAL_MOCK_METHOD_0(amqp_definitions_mocks, ,END_HANDLE, end_create);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, end_destroy, END_HANDLE, end);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_end, END_HANDLE, end);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, end_get_error, END_HANDLE, end, ERROR_HANDLE*, error_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, end_set_error, END_HANDLE, end, ERROR_HANDLE, error_value);

/* close */

	DECLARE_GLOBAL_MOCK_METHOD_0(amqp_definitions_mocks, ,CLOSE_HANDLE, close_create);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, close_destroy, CLOSE_HANDLE, close);
	DECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue_create_close, CLOSE_HANDLE, close);

	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, close_get_error, CLOSE_HANDLE, close, ERROR_HANDLE*, error_value);
	DECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, close_set_error, CLOSE_HANDLE, close, ERROR_HANDLE, error_value);


#endif /* AMQP_DEFINITIONS_MOCKS_H */
