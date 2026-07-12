/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef PLMN_IDENTITY_H
#define PLMN_IDENTITY_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>



typedef struct {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t mnc_digit_len;
} e2sm_plmn_t;

void free_e2sm_plmn( e2sm_plmn_t* src); // 6.2.3.1

bool eq_e2sm_plmn(const e2sm_plmn_t* m0, const e2sm_plmn_t* m1);

e2sm_plmn_t cp_e2sm_plmn(const  e2sm_plmn_t* src);

int cmp_e2sm_plmn(const  e2sm_plmn_t* m0, const  e2sm_plmn_t* m1);

#ifdef __cplusplus
}
#endif


#endif

// done
