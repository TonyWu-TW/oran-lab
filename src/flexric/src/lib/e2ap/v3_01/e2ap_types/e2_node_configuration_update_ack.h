/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONFIGURATION_UPDATE_ACK_H
#define E2_NODE_CONFIGURATION_UPDATE_ACK_H 

#include "common/e2ap_node_component_config_update_ack_item.h"
#include <stddef.h>

typedef struct {
  e2_node_component_config_update_ack_item_t* comp_conf_update_ack_list;
  size_t len_ccual;
} e2_node_configuration_update_ack_t;



#endif



