/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_TIME_TO_WAIT_H
#define E2AP_TIME_TO_WAIT_H 

#include <stdbool.h>

typedef enum {
	TIMETOWAIT_V1S	= 0,
	TIMETOWAIT_V2S	= 1,
	TIMETOWAIT_V5S	= 2,
	TIMETOWAIT_V10S	= 3,
	TIMETOWAIT_V20S	= 4,
	TIMETOWAIT_V60S	= 5
	/*
	 * Enumeration is extensible
	 */
} e2ap_time_to_wait_e;

bool eq_time_to_wait(const e2ap_time_to_wait_e* m0, const e2ap_time_to_wait_e* m1);



#endif

