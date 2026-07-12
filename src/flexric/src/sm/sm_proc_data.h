/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef SM_PROCEDURES_DATA_H
#define SM_PROCEDURES_DATA_H 

#include "../lib/e2ap/e2ap_ran_function_wrapper.h"

#include <stddef.h>
#include <stdint.h>

////////////////////////////////////
// 5 ORAN E2AP procedures 
// with their 9 Information Elements 
// defined in ORAN-WG3.E2SM-v01.00.00  
///////////////////////////////////
// or
////////////////////////////////////
// E2AP_V3
// 7 ORAN E2AP procedures 
// defined in ORAN-WG3.E2SM-v03.00.00  
///////////////////////////////////


typedef struct{
  uint32_t ric_req_id;

  uint8_t* event_trigger;
  size_t len_et;

  uint8_t* action_def;
  size_t len_ad;

} sm_subs_data_t;

void free_sm_subs_data(sm_subs_data_t*);

typedef struct{

  uint8_t* ind_hdr;
  size_t len_hdr;

  uint8_t* ind_msg;
  size_t len_msg;

  uint8_t* call_process_id;
  size_t len_cpid;
   
} sm_ind_data_t;

void free_sm_ind_data(sm_ind_data_t*);

// Expected, similar to std::expected
typedef struct{
 sm_ind_data_t data;
 bool has_value;
} exp_ind_data_t;

void free_exp_ind_data( exp_ind_data_t*);

//////
//
/////

typedef struct{
  uint8_t* ctrl_hdr;
  size_t len_hdr;

  uint8_t* ctrl_msg;
  size_t len_msg;
} sm_ctrl_req_data_t;

void free_sm_ctrl_req_data(sm_ctrl_req_data_t*);

typedef struct{
  uint8_t* ctrl_out;
  size_t len_out;
} sm_ctrl_out_data_t;

void free_sm_ctrl_out_data(sm_ctrl_out_data_t*);

typedef struct{
  uint8_t* ran_fun_def;
  size_t len_rfd;
} sm_e2_setup_data_t;

void free_sm_e2_setup(sm_e2_setup_data_t*); 

typedef struct{
  uint8_t* ran_fun_def;
  size_t len_rfd;
} sm_ric_service_update_data_t;

void free_sm_ric_service_update(sm_ric_service_update_data_t*);  

#ifdef E2AP_V3

typedef struct{ 
  uint8_t* query_hdr;
  size_t len_hdr;

  uint8_t* query_msg;
  size_t len_msg;
} sm_ric_query_data_t;

void free_sm_ric_query_data(sm_ric_query_data_t*);  

typedef struct{
  uint8_t* query_out;
  size_t len_out;
} sm_ric_query_out_data_t; 

void free_sm_ric_query_out_data(sm_ric_query_out_data_t*);  

typedef struct{
  uint8_t* mod;
  size_t len;
} sm_sub_mod_data_t; 

void free_sm_sub_mod_data(sm_sub_mod_data_t*);

#endif // E2AP_V3


#endif

