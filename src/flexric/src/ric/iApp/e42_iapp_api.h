
/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
      
#ifndef E42_IAPP_API_H
#define E42_IAPP_API_H 

#include "../../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../../lib/e2ap/e2ap_ran_function_wrapper.h"
#include "../../lib/e2ap/type_defs_wrapper.h"    
#include "near_ric_if.h"

#include <stdint.h>
#include <stddef.h>

typedef struct near_ric_s near_ric_t;

void init_iapp_api(const char* addr, near_ric_if_t ric);
  
void stop_iapp_api(void);     

#ifdef E2AP_V1 
void add_e2_node_iapp_api_v1(global_e2_node_id_t* id, size_t len, ran_function_t const ran_func[len]);
#elif defined(E2AP_V2) || defined(E2AP_V3)
void add_e2_node_iapp_api(global_e2_node_id_t* id, size_t len, ran_function_t const ran_func[len],  size_t len_cca, e2ap_node_component_config_add_t const* cca);
#else
  static_assert(0!=0, "Unknown E2Ap version");
#endif

void rm_e2_node_iapp_api(global_e2_node_id_t* id);

void notify_msg_iapp_api(e2ap_msg_t const* msg);

#endif

