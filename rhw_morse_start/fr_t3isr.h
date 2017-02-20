#ifndef FR_T3ISR_H
#define FR_T3ISR_H

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Note: requirted:         1 <= sysclock/12/rate   <= 65535
// Note: if frequency matters:   sysclock/12/rate   should be integer
// Note: in real life applicatrions keep ^^^^^^^^   as low as practical

void Timer3_Init(unsigned portLONG rate, void (*function)(void));
unsigned char Timer3_getParam(void);

void Timer3_ISR (void) __interrupt 14; 

#endif
