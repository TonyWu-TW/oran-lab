/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef GTP_SERVICE_MODEL_SERVER_H
#define GTP_SERVICE_MODEL_SERVER_H

#include <stdint.h>
#include "../../sm/sm_ric.h"

__attribute__ ((visibility ("default"))) 
sm_ric_t* make_gtp_sm_ric(void);

uint16_t id_gtp_sm_ric(sm_ric_t const* ); 


#endif

