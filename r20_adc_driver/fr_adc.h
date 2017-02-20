#ifndef FR_ADC_H
#define FR_ADC_H

#include "FreeRTOS.h"

#define         ADC0_MAX    (0xFFF0)
#define         VREF          (2400)

#define         ADC0_CH_TEMP     (8)
#define         ADC0_CH_MAX      (8)
#define         ADC0_GAIN_HALF  (64)
#define         ADC0_GAIN_ONE    (1)
#define         ADC0_GAIN_MAX   (16)

#define         ADC0_SEQ_MAX    (9)

void                bioADC0init();
void                bioADC0shutdown();

portBASE_TYPE       bioADC0read( 
                        const unsigned portBASE_TYPE    inputs[],   // sequence of inputs to read
                        const unsigned portBASE_TYPE    gains[],    // sequence of gains for inputs to read
                        unsigned portBASE_TYPE          count,      // how many items in the sequence
                        unsigned portSHORT              values[],   // array to store the results
                        portTickType                    xBlockTime  // maximum time to wait for the results
                    );

void                ADC0_ISR (void) __interrupt 15;

#endif
