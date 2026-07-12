/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONFIGURATION_UPDATE_ACK_ITEM_H
#define E2_NODE_CONFIGURATION_UPDATE_ACK_ITEM_H 

#include "e2ap_cause.h"
#include "e2ap_node_component_config_update.h"

typedef struct{
  e2_node_component_type_e e2_node_component_type;
  e2_node_component_id_present_e* id_present; // optional
  union {
    uint64_t gnb_cu_up_id;
    uint64_t gnb_du_id;
  };

 	long	 update_outcome;
	cause_t *failure_cause;	/* OPTIONAL */
} e2_node_component_config_update_ack_item_t;

#endif


