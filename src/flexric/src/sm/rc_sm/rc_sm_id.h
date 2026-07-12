/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RAN_CTRL_SERVICE_MODEL_ID_H
#define RAN_CTRL_SERVICE_MODEL_ID_H 


/*
 * Service Model ID needed for the agent as well as for the ric to ensure that they match. 
 */

#include <stdint.h>

static
const uint16_t SM_RC_ID = 3; 

static
const uint16_t SM_RC_REV = 1; 

#define SM_RAN_CTRL_SHORT_NAME "ORAN-E2SM-RC"
//iso(1) identified-organization(3)
//dod(6) internet(1) private(4)
//enterprise(1) 53148 e2(1)
// version1 (1) e2sm(2) e2sm-RC-
// IEs (3)

#define SM_RAN_CTRL_OID "1.3.6.1.4.1.53148.1.1.2.3"

#define SM_RAN_CTRL_DESCRIPTION "RAN Control"

#endif

