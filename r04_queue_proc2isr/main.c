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

typedef struct InterprocessMessageDefinition
{
    unsigned portCHAR value;
    // .. place more data items here as necessary
} xMESSAGE;

/*-----------------------------------------------------------*/

void   mainInitComm( void );
void   mainInitTasks( void );
static void userRead( void *pvParameters );
void   myInterruptHandler( void );

/*-----------------------------------------------------------*/

/* These below are not variables, just values that are to be placed into 
formulas or variable initialization, or a function call. If all values 
of a formula are constant and known at the compilation time then a good 
compiler replace them with a value. This allows to avoid inefficiencies 
and shortens the code for the firmware. */

#define exampleIsrRATE (200L)
// Defines how often to run your interrupt

#define exampleIsrCOMM (exampleIsrRATE/100)
// Defines how often attempt to communicate with OS from within the interrupt
// If you want to communicate each time, do not expect to be able to service
// that interrupt more than a few thousand times per second.

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    mainInitComm();

    Timer3_Init( exampleIsrRATE , &myInterruptHandler );
    // Function void mainInitComm( void ) will be run from an interrupt at frequency 200Hz)
    // Minimum frequency for 100MHz clock is 128Hz, function must be very short to execute

    mainInitTasks();

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control
    of the scheduler. */
}

/*-----------------------------------------------------------*/

static xQueueHandle xComQueue;

/*-----------------------------------------------------------*/

void   mainInitComm( void )
{
    xComQueue = xQueueCreate( (unsigned portBASE_TYPE ) 5, ( unsigned portBASE_TYPE ) sizeof( xMESSAGE ) );
    vQueueAddToRegistry(xComQueue, ( signed portCHAR * ) "QUEUE"); // optional name registration
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
    xTaskCreate( userRead, ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void userRead( void *pvParameters )
{
    portCHAR        state;
    xMESSAGE        message;
    portBASE_TYPE   status;

    if ( 0 == xComQueue ) return; // not initialized properly

    ( void ) pvParameters;

    state   = bioGetSW2();
    message.value = 1;
    for (;;)
    {
        if ( bioGetSW2() != state )
        {
            state = bioGetSW2();
            if (state)
            {
                message.value = !message.value;
                status = xQueueSend( xComQueue, (void*) &message, 0);
                if ( status != pdTRUE )
                    bioSetAB4_LED2(bioLED_ON);
            }
        }
        // taskYIELD();
        // this taskYIELD() could be optional if both are true:
        // 1)  RTOS is preemptive, and
        // 2)  userRead tasks has lower priority than userExec
        vTaskDelay( (10 / portTICK_RATE_MS) );
        // vTaskDelay will allow all lower priority tasks to run
        // while this task sleeps. 
        // Also, short sleep will serve as button debouncing feature
    }
}

/*-----------------------------------------------------------*/

// This function is called within an interrupt - it must be as short as possible
// and must not call on any tasks that might cause any unnecessary delay
void myInterruptHandler( void ) 
{
    static unsigned portSHORT countIntr = 0;
    static unsigned portSHORT countComm = 0;
    static xMESSAGE  state = {1}; // see also how to initialize more complex structures
    static xMESSAGE  message; 
                /* we use static so that the memory is not allocated and gone
                   each time  the interrupt is serviced as xMESSAGE may take 
                   a larger chunk of memory in some of your applications */
    
    static portBASE_TYPE xTaskWokenByReceive = pdFALSE;
    static portBASE_TYPE changed = pdTRUE;

    ++countComm;
    if ( countComm >= exampleIsrCOMM ) {
        countComm = 0;

        if ( xQueueReceiveFromISR( xComQueue, (void*) &message, &xTaskWokenByReceive ) )
        {
            // There was something in the queue and we can import data from the received message
            memcpy( (void*)state, (void*)message, sizeof(xMESSAGE) );
            changed = pdTRUE;
        }
        if( xTaskWokenByReceive != pdFALSE )
        {
            // We should switch context so the ISR returns to a different task.
            taskYIELD ();
        }
    }

    if (changed)
    {
        changed = pdFALSE;
        if ( state.value )
        {
            bioSetAB4_LED1(bioLED_ON);
        } else {
            bioSetAB4_LED1(bioLED_OFF);
        }
    }

    ++countIntr;
    if ( countIntr >= (exampleIsrRATE>>1) )
    {
        countIntr = 0;
        if ( state.value || bioGetLED() )
            bioToggleLED();
    }

}
