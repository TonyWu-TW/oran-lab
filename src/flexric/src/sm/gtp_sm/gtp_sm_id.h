/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef GTP_SERVICE_MODEL_ID_H
#define GTP_SERVICE_MODEL_ID_H 


/*
 * Service Model ID needed for the agent as well as for the ric to ensure that they match. 
 */

#include <stdint.h>

static
const uint16_t SM_GTP_ID = 148; 

__attribute__((unused)) static
const char* SM_GTP_STR = "GTP_STATS_V0"; 

static
const uint16_t SM_GTP_REV = 1; 

__attribute__((unused)) static
const char SM_GTP_SHORT_NAME[] = "E2SM-GTP";

//iso(0) identified-organization(0)
//dod(0) internet(0) private(0)
//enterprise(0) 53148 e2(0)
// version1 (1) e2sm(148) e2sm-RC-
// IEs (0)

__attribute__((unused)) static
const char SM_GTP_OID[] = "0.0.0.0.0.0.0.0.1.148.0"; 

__attribute__((unused)) static
const char SM_GTP_DESCRIPTION[] = "GTP Service Model";

#endif

