/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifdef __cplusplus
extern "C" {
#endif


#ifndef NEAR_RIC_SERVER_API
#define NEAR_RIC_SERVER_API

#include "../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../ric/e2_node.h"
#include "../util/conf_file.h"

#include <stddef.h>
#include <stdint.h>

typedef struct{
  e2_node_t* n;  
  size_t len;
} e2_nodes_api_t;

void free_e2_nodes_api( e2_nodes_api_t* src);

void init_near_ric_api(fr_args_t const*);

void stop_near_ric_api(void);

e2_nodes_api_t e2_nodes_near_ric_api(void);

// NEAR-RT RIC services
// 4 basic Service reports defined 
// in Near-Real-time RAN Intelligent Controller
// E2 Service Model (E2SM)

uint16_t report_service_near_ric_api(global_e2_node_id_t const* id, uint16_t ran_func_id, void* cmd);

void rm_report_service_near_ric_api(global_e2_node_id_t const* id, uint16_t ran_func_id, uint16_t act_id);

void control_service_near_ric_api(global_e2_node_id_t const* id, uint16_t sm_id, void* cmd);

void insert_service_near_ric_api(uint16_t sm_id, const char* cmd);

void policy_service_near_ric_api(uint16_t sm_id, const char* cmd);

// NEAR-RT RIC support functions:
// Interface Management (E2 Setup, E2 Reset, 
// E2 Node Configuration Update, Reporting of 
// General Error Situations)
// Near-RT RIC Service Update, i.e. a E2 Node 
// initiated procedure to inform Near-RT RIC of
// changes to list of supported Near-RT RIC 
// services and mapping of services to functions.

// Plug-in functions
void load_sm_near_ric_api(const char* file_path);


// Observer pattern. Interface for subscription/publish for xApps
typedef struct{
  void (*update)(const char* data);
} subs_t;

void susbscribe_near_ric(/*global_e2_node_id_t const* id,*/ uint16_t sm_id, subs_t subscription);

void unsusbscribe_near_ric( /*global_e2_node_id_t const* id,*/ uint16_t sm_id, subs_t subscription);

// ToDo: Provide connected Agents and their ran functions 
// (note that the interface should be callable from Python, Go
// and NodeJS, at least)

#endif

#ifdef __cplusplus
}
#endif



