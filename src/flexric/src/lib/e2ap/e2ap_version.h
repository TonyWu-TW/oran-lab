/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_VERSION_MIR_H
#define E2AP_VERSION_MIR_H 


typedef struct{
  int dummy;
} e2ap_v1_t;

typedef struct{
  int dummy;
} e2ap_v2_t;

typedef struct{
  int dummy;
} e2ap_v3_t;

typedef struct{
#ifdef E2AP_V1
  e2ap_v1_t type;
#elif E2AP_V2
  e2ap_v2_t type;
#elif E2AP_V3
  e2ap_v3_t type;
#else
  static_assert(0!=0, "Error. Unknown E2AP version");
#endif
} e2ap_version_t;

#endif

