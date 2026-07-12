/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_GLOBAL_RIC_H
#define E2AP_GLOBAL_RIC_H

#include "e2ap_plmn.h"
#include <stdint.h>

typedef struct{
  e2ap_plmn_t plmn;

  union{
    uint32_t double_word;
    uint8_t bytes[4];  
  } near_ric_id;

} global_ric_id_t;


bool eq_global_ric_id(const global_ric_id_t* m0, const global_ric_id_t* m1);


#endif

