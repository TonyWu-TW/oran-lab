/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef PDCP_SERVICE_MODEL_ID_H
#define PDCP_SERVICE_MODEL_ID_H 


/*
 * Service Model ID needed for the agent as well as for the ric to ensure that they match. 
 */

#include <stdint.h>

static
const uint16_t SM_PDCP_ID = 144; 

__attribute__((unused)) static
const char* SM_PDCP_STR = "PDCP_STATS_V0"; 

static
const uint16_t SM_PDCP_REV = 1; 

__attribute__((unused)) static
const char SM_PDCP_SHORT_NAME[] = "E2SM-PDCP";

//iso(0) identified-organization(0)
//dod(0) internet(0) private(0)
//enterprise(0) 53148 e2(0)
// version1 (1) e2sm(144) e2sm-RC-
// IEs (0)

__attribute__((unused)) static
const char SM_PDCP_OID[] = "0.0.0.0.0.0.0.0.1.144.0"; 

__attribute__((unused)) static
const char SM_PDCP_DESCRIPTION[] = "PDCP Service Model";

#endif

