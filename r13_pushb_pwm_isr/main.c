/* Standard includes. */
#include <stdlib.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"

/* Timer3 interrupt service initialization. */
#include "fr_t3isr.h"

/*-----------------------------------------------------------*/

/* These below are not variables, just values that are to be placed into 
formulas or variable initialization, or a function call. If all values 
of a formula are constant and known at the compilation time then a good 
compiler replace them with a value. This allows to avoid inefficiencies 
and shortens the code for the firmware. */

#define exampleIsrRATE (200L)
// Defines how often to run your interrupt

#define examplePwmRATE (10)
// Defines the frequency of the PWM signal

#define examplePwmLEVELS (10)
// Defines the PWM resolution

#define examplePwmINIT (examplePwmLEVELS>>1)
// Defines the initial PWM level: 0-0%, examplePwmLEVELS-100%

// comment out to remove 1Hz diagnostic blinking 
#define debug_1Hz_flashing 1

// comment out to allow the iddle task to run
#define prevent_iddle_task 1

/*-----------------------------------------------------------*/

typedef struct InterprocessMessageDefinition
{
    unsigned portSHORT isr_ticks_on;
    unsigned portSHORT isr_ticks_total;
    // .. place more data items here as necessary
} xMESSAGE;

/*-----------------------------------------------------------*/

void   mainInitComm( void );
void   mainInitTasks( void );
static void userRead( void *pvParameters );
#ifdef prevent_iddle_task
static void userNotiddle( void *pvParameters );
#endif
void   myInterruptHandler( void );

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    mainInitComm();

    Timer3_Init( exampleIsrRATE , &myInterruptHandler );
    // Function void mainInitComm( void ) will be run from an interrupt at frequency 10KHz)
    // Minimum frequency for 100MHz clock is 128Hz, function must be very short to execute

    mainInitTasks();

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control
    of the scheduler. */
}

/*-----------------------------------------------------------*/

static xMESSAGE xSharedVar;

/*-----------------------------------------------------------*/

void   mainInitComm( void )
{
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
    xTaskCreate( userRead,     ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, ( xTaskHandle * ) NULL );
#ifdef prevent_iddle_task
    xTaskCreate( userNotiddle, ( signed portCHAR * ) "BUSY",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
#endif
}

/*-----------------------------------------------------------*/

static void userRead( void *pvParameters )
{
    portSHORT       value = examplePwmINIT;
    portBASE_TYPE   changed = pdTRUE;
    xMESSAGE  message = { (examplePwmINIT*exampleIsrRATE)/(examplePwmRATE*examplePwmLEVELS), (exampleIsrRATE/examplePwmRATE) };
                // see also how to initialize more complex structures
    portBASE_TYPE   b1_state;
    portBASE_TYPE   b2_state;

    ( void ) pvParameters;

    b1_state   = bioGetAB4_SW1();
    b2_state   = bioGetAB4_SW2();
    for (;;)
    {
        if ( bioGetAB4_SW1() != b1_state )
        {
            b1_state = bioGetAB4_SW1();
            if (b1_state && value>0)
            {
                --value;
                changed = pdTRUE;
            }
        }

        if ( bioGetAB4_SW2() != b2_state )
        {
            b2_state = bioGetAB4_SW2();
            if (b2_state && value<examplePwmLEVELS)
            {
                ++value;
                changed = pdTRUE;
            }
        }

        if (changed)
        {
            message.isr_ticks_on = ( ((unsigned portLONG)value)*exampleIsrRATE ) / (examplePwmRATE*examplePwmLEVELS);
            // message.isr_ticks_total remains unchenged
            changed = pdFALSE;

            taskENTER_CRITICAL();
            {
                memcpy( &xSharedVar, &message, sizeof(xMESSAGE) );
            }
            taskEXIT_CRITICAL();
        
            if ( ( 0 == value ) || ( 10 == value ) )
                bioSetAB4_LED2(bioLED_ON);
            else
                bioSetAB4_LED2(bioLED_OFF);
        }

        vTaskDelay( (50 / portTICK_RATE_MS) );
    }
}

/*-----------------------------------------------------------*/

#ifdef prevent_iddle_task
static void userNotiddle( void *pvParameters )
{
    ( void ) pvParameters;
    for (;;) ; // keep the OS busy when nothing else runs to prevent continuous yielding from the iddle task
}
#endif

/*-----------------------------------------------------------*/

// This function is called within an interrupt - it must be as short as possible
// and must not call on any tasks that might cause any unnecessary delay
void myInterruptHandler( void ) 
{
#ifdef debug_1Hz_flashing
    static unsigned portSHORT countDiag = 0;
#endif
    static unsigned portSHORT countIntr = 0;
    static xMESSAGE  state = { (examplePwmINIT*exampleIsrRATE/examplePwmRATE/examplePwmLEVELS) , (exampleIsrRATE/examplePwmRATE) };
                // see also how to initialize more complex structures
                /* Variables above must retain values from isr to isr */

    if (countIntr < state.isr_ticks_on)
    {
        bioSetLED(bioLED_ON);
    } else {
        bioSetLED(bioLED_OFF);
    }

    ++countIntr;
    if ( countIntr >= state.isr_ticks_total )
    {
        countIntr = 0;

        // Update the PWM at counter roll over to ensure smooth change of PWM signal
        // use of CRITICAL is somewhat redundant here as we have only one priority of interrupts set up
        taskENTER_CRITICAL();
        {
            memcpy( (void*) &state, (void*) &xSharedVar, sizeof(xMESSAGE) );
        }
        taskEXIT_CRITICAL();
    }

#ifdef debug_1Hz_flashing
    // 1Hz diagnostic blinking
    if (countDiag < (exampleIsrRATE>>1))
    {
        bioSetAB4_LED1(bioLED_ON);
    } else {
        bioSetAB4_LED1(bioLED_OFF);
    }

    ++countDiag;
    if ( countDiag >= exampleIsrRATE )
    {
        countDiag = 0;
    }
#endif

}
