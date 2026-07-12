/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_CONNECTION_UPDATE_H
#define E2AP_CONNECTION_UPDATE_H 

#include "e2ap_tnl_information.h"
#include "e2ap_tnl_usage.h"

typedef struct{
  e2ap_tnl_information_t info;
  e2ap_tnl_usage_e usage;  
} e2_connection_update_item_t;

#endif
