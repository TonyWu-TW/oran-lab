/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef SERVICE_MODEL_STRING_PARSER_H
#define SERVICE_MODEL_STRING_PARSER_H 

#include <stddef.h>                            // for size_t
#include <stdint.h>                            // for int64_t
#include "../../sm/mac_sm/ie/mac_data_ie.h"
#include "../../sm/rlc_sm/ie/rlc_data_ie.h"
#include "../../sm/pdcp_sm/ie/pdcp_data_ie.h"
#include "../../sm/slice_sm/ie/slice_data_ie.h"
#include "../../sm/gtp_sm/ie/gtp_data_ie.h"
#include "../../sm/kpm_sm/kpm_data_ie_wrapper.h"

void to_string_mac_ue_stats(mac_ue_stats_impl_t* stats, int64_t tstamp, char* out, size_t out_len);

void to_string_rlc_rb(rlc_radio_bearer_stats_t* rlc, int64_t tstamp, char* out, size_t out_len);

void to_string_pdcp_rb(pdcp_radio_bearer_stats_t* pdcp, int64_t tstamp, char* out, size_t out_len);

void to_string_slice(slice_ind_msg_t const* slice, int64_t tstamp, char* out, size_t out_len);

void to_string_gtp_ngu(gtp_ngu_t_stats_t const* gtp, int64_t tstamp, char* out, size_t out_len);

void to_string_kpm_measRecord(meas_record_lst_t const* measRecord, size_t idx, char*out, size_t out_len);

void to_string_kpm_labelInfo(label_info_lst_t const* labelInfo, size_t idx, char*out, size_t out_len);

#endif
