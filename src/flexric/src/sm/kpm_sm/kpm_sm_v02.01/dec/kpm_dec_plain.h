/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
/*
 * Decoding interface for plain text encoding Information Elements in SM-KPM
 */
#ifndef KPM_DECODING_PLAIN_H
#define KPM_DECODING_PLAIN_H

#include <stddef.h>
#include "../ie/kpm_data_ie.h"


kpm_event_trigger_t kpm_dec_event_trigger_plain(size_t len, uint8_t const ev_tr[len]);
kpm_action_def_t kpm_dec_action_def_plain(size_t len, uint8_t const * action_def);
#endif