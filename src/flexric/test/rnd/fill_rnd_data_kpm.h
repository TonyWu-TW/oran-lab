/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FILL_RND_DATA_KPM_V3_H
#define FILL_RND_DATA_KPM_V3_H

#include "../../src/sm/kpm_sm/kpm_data_ie_wrapper.h"

kpm_event_trigger_def_t fill_rnd_kpm_event_trigger_def(void);
  
kpm_act_def_t fill_rnd_kpm_action_def(void);

kpm_ind_hdr_t fill_rnd_kpm_ind_hdr(void);

kpm_ind_msg_t fill_rnd_kpm_ind_msg(void);

kpm_ran_function_def_t fill_rnd_kpm_ran_func_def(void);

#endif

