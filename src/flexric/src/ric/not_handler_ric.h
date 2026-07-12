/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef NOTIFICATION_HANDLER_RIC_H
#define NOTIFICATION_HANDLER_RIC_H

#include "near_ric.h"

void notification_handle_ric(near_ric_t* ric, sctp_msg_t const* msg);

#endif

