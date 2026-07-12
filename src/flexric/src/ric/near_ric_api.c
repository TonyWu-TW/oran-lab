/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "near_ric_api.h"
#include "near_ric.h"  // for control_service_near_ric, free_near_ric, init_
#include "../util/conf_file.h"
#include <assert.h>    // for assert
#include <pthread.h>   // for pthread_create, pthread_join, pthread_t
#include <stddef.h>    // for NULL
#include <stdio.h>

/*
#include "near_ric_api.h"
#include "near_ric.h"
#include "e2ap_ric.h"
#include "../sm/sm_ric.h"

#include <assert.h>
#include <stdio.h>
#include <pthread.h>
*/


static
near_ric_t* ric = NULL;

static
pthread_t t_near_ric;

static
void* static_start_near_ric(void* a)
{
  (void)a;
  // Blocking call
  start_near_ric(ric);
  return NULL;
}



void init_near_ric_api(fr_args_t const* args)
{
  assert(ric == NULL);

  ric = init_near_ric(args);
  assert(ric != NULL && "Memory exhausted");

  // Spawn a new thread for the ric
  int const rc = pthread_create(&t_near_ric, NULL, static_start_near_ric, ric);
  assert(rc == 0);
}

void stop_near_ric_api()
{
  assert(ric != NULL);
  free_near_ric(ric);
  int const rc = pthread_join(t_near_ric, NULL);
  assert(rc  == 0);
}


e2_nodes_api_t e2_nodes_near_ric_api(void)
{
  assert(ric != NULL);

  seq_arr_t arr = conn_e2_nodes(ric); 

  size_t const sz = seq_size(&arr);
  e2_nodes_api_t ans = {.len = sz};  

 if(ans.len > 0){
  ans.n = calloc(ans.len, sizeof(e2_node_t)); 
  assert(ans.n != NULL && "Memory exhausted");
 }
/*
 for(size_t i = 0; i < sz; ++i){
    e2_node_t* n = (e2_node_t*)seq_at(&arr, i);  
    ans.n[i] = cp_e2_node(n);
 }
*/

 void* it = seq_front(&arr);
 void* end = seq_end(&arr);

 size_t i = 0;
 while(it != end){
  e2_node_t* n = (e2_node_t*)it;  
  ans.n[i] = cp_e2_node(n);
  ++i;
  it = seq_next(&arr, it);
 }

 assert(i == seq_size(&arr) && "Size mismatch while copying \n");
 
  seq_free(&arr, NULL);
  return ans;
}


void free_e2_nodes_api(e2_nodes_api_t* src)
{
  assert(src != NULL);

  for(size_t i = 0; i < src->len; ++i){
    free_e2_node(&src->n[i]);
  }

  free(src->n);
}

uint16_t report_service_near_ric_api(global_e2_node_id_t const* id, uint16_t ran_func_id, void* cmd)
{
  assert(ric != NULL);
  assert(ran_func_id != 0 && "Reserved SM ID");  
  assert(cmd != NULL);

  return report_service_near_ric(ric, id, ran_func_id, cmd);
}

void rm_report_service_near_ric_api(global_e2_node_id_t const* id, uint16_t ran_func_id, uint16_t act_id)
{
  assert(ric != NULL);
  assert(act_id != 0 && "Reserved SM ID");  

  return rm_report_service_near_ric(ric, id, ran_func_id, act_id);
}

void control_service_near_ric_api(global_e2_node_id_t const* id, uint16_t ran_func_id, void* cmd)
{
  assert(ric!= NULL);
  assert(ran_func_id != 0 && "Reserved SM ID");  
  assert(cmd != NULL);

  return control_service_near_ric(ric, id, ran_func_id, cmd);
}

void load_sm_near_ric_api(const char* file_path)
{
  assert(ric!= NULL);
  assert(file_path != NULL); 
  return load_sm_near_ric(ric, file_path); 
}

