/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_TNL_INFORMATION_H
#define E2AP_TNL_INFORMATION_H

#include "util/byte_array.h"

typedef struct
{
  byte_array_t tnl_addr;
  byte_array_t* tnl_port; // optional
} e2ap_tnl_information_t;


#endif

