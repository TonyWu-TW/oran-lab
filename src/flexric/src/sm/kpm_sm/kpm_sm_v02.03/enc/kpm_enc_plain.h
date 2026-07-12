/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef KPM_ENCODING_PLAIN_H
#define KPM_ENCODING_PLAIN_H 

#include "../../../util/byte_array.h"
#include "../ie/kpm_data_ie/e2ap_procedures/ric_subscription.h"

typedef struct{
  // intentionally empty
} kpm_enc_plain_t;


byte_array_t kpm_enc_event_trigger_plain(kpm_event_trigger_def_t const* event_trigger);


#endif