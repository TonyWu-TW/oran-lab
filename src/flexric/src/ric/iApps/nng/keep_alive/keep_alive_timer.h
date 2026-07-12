/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef KEEP_ALIVE_TIMER_H
#define KEEP_ALIVE_TIMER_H 

#include <stdint.h>

//#include "../msgs/xapp_msgs.h"

/*
 * Naive timer that expires every exp_time_ms and calls the function fun(void* data) 
 */



void init_keep_alive_timer(uint32_t exp_time_ms, void (*fun)(void*), void* data);

void stop_keep_alive_timer(void);


#endif

