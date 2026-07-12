/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_GNB_ID_MIR_H
#define E2AP_GNB_ID_MIR_H 

#include <stdbool.h>
#include <stdint.h>

typedef struct{
  uint32_t nb_id;
  uint32_t unused;
} e2ap_gnb_id_t;

bool eq_e2ap_gnb_id(e2ap_gnb_id_t m0,  e2ap_gnb_id_t m1);

#endif
