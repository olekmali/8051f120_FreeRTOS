#include "fr_t3isr.h"

#include "FreeRTOS.h"
#include "queue.h"

static void (*Timer3_ISR_function)(void) = 0;

//------------------------------------------------------------------------------------
// Timer3_Init
//------------------------------------------------------------------------------------
//
// Configure Timer3 to auto-reload and generate an interrupt at interval specified by <counts>
void Timer3_Init (unsigned unsigned portLONG rate, void (*function)(void))
{
    Timer3_ISR_function = function;
    portENTER_CRITICAL();
    {
        unsigned portCHAR ucOriginalSFRPage1 = SFRPAGE;
        SFRPAGE = TMR3_PAGE;                // set SFR page
    
        TMR3CN  = 0x00;                     // Stop Timer3; Clear TF3;
        TMR3CF  = 0x00;                     // use SYSCLK/12 as timebase
    //  TMR3CF  = 0x08;                     // use SYSCLK as timebase
        RCAP3   = -(configCPU_CLOCK_HZ/12)/rate;
                                            // Init Timer3 to generate interrupts at a RATE Hz rate.
                                            // Note that timer3 is connected to configCPU_CLOCK_HZ/12
                                            // Init reload values
        TMR3    = 0xffff;                   // set to reload immediately
        EIE2   |= 0x01;                     // enable Timer3 interrupts - bit 00000001 or ET3 = 1;
        TMR3CN |= 0x04;                     // start Timer3
    
        SFRPAGE = ucOriginalSFRPage1;       // Restore SFR page
    }
    portEXIT_CRITICAL();
}

//------------------------------------------------------------------------------------
// Timer3_ISR services Timer3 interrupt
//------------------------------------------------------------------------------------
//
void Timer3_ISR (void) __interrupt 14
{
    portENTER_CRITICAL();
    {
        unsigned portCHAR ucOriginalSFRPage2 = SFRPAGE;
        SFRPAGE = TMR3_PAGE;                // set SFR page (automatic setting is globally disabled)
        TF3 = 0;                            // clear TF3
        SFRPAGE = ucOriginalSFRPage2;        // Restore SFR page
    }
    portEXIT_CRITICAL();
    (*Timer3_ISR_function)();
}
