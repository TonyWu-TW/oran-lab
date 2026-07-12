/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RLC_SERVICE_MODEL_ID_H
#define RLC_SERVICE_MODEL_ID_H 


/*
 * Service Model ID needed for the agent as well as for the ric to ensure that they match. 
 */

#include <stdint.h>

static
const uint16_t SM_RLC_ID = 143; 

__attribute__((unused)) static
const char* SM_RLC_STR = "RLC_STATS_V0"; 

static
const uint16_t SM_RLC_REV = 1; 

__attribute__((unused)) static
const char SM_RLC_SHORT_NAME[] = "E2SM-RLC";

//iso(0) identified-organization(0)
//dod(0) internet(0) private(0)
//enterprise(0) 53148 e2(0)
// version1 (1) e2sm(143) e2sm-RC-
// IEs (0)

__attribute__((unused)) static
const char SM_RLC_OID[] = "0.0.0.0.0.0.0.0.1.143.0"; 

__attribute__((unused)) static
const char SM_RLC_DESCRIPTION[] = "RLC Service Model";

#endif

