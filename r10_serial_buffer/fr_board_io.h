#ifndef FR_BOARD_IO_H
#define FR_BOARD_IO_H

#include "FreeRTOS.h"

#define bioLED_OFF (0)
#define bioLED_ON  (1)

#define bioGetLED()     (LED)
#define bioSetLED(x)    (LED=(x))
#define bioToggleLED()  (LED=!LED)
#define bioGetSW2()     (!SW2)

void            bioSetAB4_LED1(portBASE_TYPE newval);
void            bioSetAB4_LED2(portBASE_TYPE newval);

portBASE_TYPE   bioGetAB4_SW1();
portBASE_TYPE   bioGetAB4_SW2();

#endif
