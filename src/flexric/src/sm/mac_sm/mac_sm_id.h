/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef MAC_SERVICE_MODEL_ID_H
#define MAC_SERVICE_MODEL_ID_H 

/*
 * Service Model ID needed for the agent as well as for the ric to ensure that they match. 
 */

#include <stdint.h>

static
const uint16_t SM_MAC_ID = 142; 

__attribute__((unused)) static
const char* SM_MAC_STR = "MAC_STATS_V0"; 

static
const uint16_t SM_MAC_REV = 1; 

__attribute__((unused)) static
const char SM_MAC_SHORT_NAME[] = "E2SM-MAC";

//iso(0) identified-organization(0)
//dod(0) internet(0) private(0)
//enterprise(0) 53148 e2(0)
// version1 (1) e2sm(142) e2sm-RC-
// IEs (0)

__attribute__((unused)) static
const char SM_MAC_OID[] = "0.0.0.0.0.0.0.0.1.142.0"; 

__attribute__((unused)) static
const char SM_MAC_DESCRIPTION[] = "MAC Service Model";

#endif

