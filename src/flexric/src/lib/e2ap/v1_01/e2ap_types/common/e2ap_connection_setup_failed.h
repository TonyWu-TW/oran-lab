/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_CONNECTION_SETUP_FAILED_H
#define E2AP_CONNECTION_SETUP_FAILED_H 

#include "e2ap_cause.h"
#include "e2ap_tnl_information.h"

typedef struct{
  cause_t cause;
  e2ap_tnl_information_t info; 
} e2_connection_setup_failed_t;  


#endif

