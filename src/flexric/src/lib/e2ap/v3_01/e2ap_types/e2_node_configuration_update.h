/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONFIGURATION_UPDATE_H
#define E2_NODE_CONFIGURATION_UPDATE_H 

#include "common/e2ap_node_component_config_update.h"
#include <stdbool.h>

typedef struct {
  e2_node_component_config_update_t* comp_conf_update_list;
  size_t len_ccul;
} e2_node_configuration_update_t;

bool eq_node_configuration_update(const e2_node_configuration_update_t* m0, const e2_node_configuration_update_t* m1);

#endif
