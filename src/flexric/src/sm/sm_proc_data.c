/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "sm_proc_data.h"

#include <assert.h>
#include <stdlib.h>

void free_sm_subs_data(sm_subs_data_t* data)
{
  assert(data != NULL);

  if(data->action_def != NULL){
    assert(data->len_ad != 0);
    free(data->action_def);
  }

  if(data->event_trigger != NULL){
    assert(data->len_et != 0);
    free(data->event_trigger);
  }
}

void free_sm_ind_data(sm_ind_data_t* data)
{
  assert(data != NULL);

  if(data->ind_hdr != NULL){
    assert(data->len_hdr != 0);
    free(data->ind_hdr);
  }

  if(data->ind_msg != NULL){
    assert(data->len_msg != 0);
    free(data->ind_msg);
  }

  if(data->call_process_id != NULL){
    assert(data->len_cpid != 0);
    free(data->call_process_id);
  }
}

void free_exp_ind_data(exp_ind_data_t* exp)
{
  assert(exp != NULL);
  free_sm_ind_data(&exp->data);
}

void free_sm_ctrl_req_data(sm_ctrl_req_data_t* data)
{
  assert(data != NULL);

  if(data->ctrl_hdr != NULL){
    assert(data->len_hdr != 0);
    free(data->ctrl_hdr);
  }

  if(data->ctrl_msg != NULL){
    assert(data->len_msg != 0);
    free(data->ctrl_msg);
  }
}

void free_sm_ctrl_out_data(sm_ctrl_out_data_t* data)
{
  assert(data != NULL);

  if(data->len_out > 0){
    assert(data->ctrl_out != NULL);
    free(data->ctrl_out);
  }
}

void free_sm_e2_setup(sm_e2_setup_data_t* data)
{
  assert(data != NULL);

  if(data->ran_fun_def != NULL){
    assert(data->len_rfd != 0);
    free(data->ran_fun_def);
  }
  //free_ran_function(&data->rf);
} 

void free_sm_ric_service_update(sm_ric_service_update_data_t* data)
{
  assert(data != NULL);
  
  if(data->ran_fun_def != NULL){
    assert(data->len_rfd != 0);
    free(data->ran_fun_def);
  }

}

#ifdef E2AP_V3

void free_sm_ric_query_data(sm_ric_query_data_t* data)
{
  assert(data != NULL);

  if(data->query_hdr != NULL){
    assert(data->len_hdr != 0);
    free(data->query_hdr);
  }

  if(data->query_msg != NULL){
    assert(data->len_msg != 0);
    free(data->query_msg);
  }
}

void free_sm_ric_query_out_data(sm_ric_query_out_data_t* data)
{
  assert(data != NULL);

  if(data->query_out != NULL){
    assert(data->len_out != 0);
    free(data->query_out);
  }
}

void free_sm_sub_mod_data(sm_sub_mod_data_t* data)
{
  assert(data != NULL);
  
  if(data->mod != NULL){
    assert(data->len != 0);
    free(data->mod);
  }
}

#endif // E2AP_V3

