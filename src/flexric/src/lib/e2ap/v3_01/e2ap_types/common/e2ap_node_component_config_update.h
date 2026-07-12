/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_NODE_COMPONENT_CONFIG_UPDATE_H
#define E2AP_NODE_COMPONENT_CONFIG_UPDATE_H

#include "../../../../../util/byte_array.h"
#include <stdint.h>

typedef enum {
	E2_NODE_COMPONENT_CONFIG_UPDATE_NOTHING,	
	E2_NODE_COMPONENT_CONFIG_UPDATE_GNB_CONFIG_UPDATE,
	E2_NODE_COMPONENT_CONFIG_UPDATE_EN_GNB_CONFIG_UPDATE,
	E2_NODE_COMPONENT_CONFIG_UPDATE_NG_ENB_CONFIG_UPDATE,
	E2_NODE_COMPONENT_CONFIG_UPDATE_ENB_CONFIG_UPDATE
} e2_node_component_config_update_present_e;

typedef enum {
	E2_NODE_COMPONENT_ID_E2_NODE_COMPONENT_TYPE_GNB_CU_UP,
	E2_NODE_COMPONENT_ID_E2_NODE_COMPONENT_TYPE_GNB_DU
} e2_node_component_id_present_e;


typedef enum {
 	E2_NODE_COMPONENT_INTERFACE_TYPE_NG	= 0,
	E2_NODE_COMPONENT_INTERFACE_TYPE_XN	= 1,
	E2_NODE_COMPONENT_INTERFACE_TYPE_E1	= 2,
	E2_NODE_COMPONENT_INTERFACE_TYPE_F1	= 3,
	E2_NODE_COMPONENT_INTERFACE_TYPE_W1	= 4,
	E2_NODE_COMPONENT_INTERFACE_TYPE_S1	= 5,
	E2_NODE_COMPONENT_INTERFACE_TYPE_X2	= 6
} e2_node_component_type_e;

typedef struct {
  e2_node_component_type_e e2_node_component_type;
  e2_node_component_id_present_e* id_present; // optional
  union {
    uint64_t gnb_cu_up_id;
    uint64_t gnb_du_id;
  };
  e2_node_component_config_update_present_e update_present;
  union {
    struct {
      byte_array_t* ngap_gnb_cu_cp;
      byte_array_t* xnap_gnb_cu_cp;
      byte_array_t* e1ap_gnb_cu_cp;
      byte_array_t* f1ap_gnb_cu_cp;
    } gnb;
    struct {
      byte_array_t* x2ap_en_gnb;
    } en_gnb;
    struct {
      byte_array_t* ngap_ng_enb;
      byte_array_t* xnap_ng_enb;
    } ng_enb;
    struct {
      byte_array_t* s1ap_enb;
      byte_array_t* x2ap_enb;
    } enb;
  };
} e2_node_component_config_update_t;

e2_node_component_config_update_t cp_e2_node_component_config_update(const e2_node_component_config_update_t* src);

void free_e2_node_component_config_update(e2_node_component_config_update_t* src);


bool eq_e2_node_component_config_update(const e2_node_component_config_update_t* m0, const e2_node_component_config_update_t* m1);

#endif

