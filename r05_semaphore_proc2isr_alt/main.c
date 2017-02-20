/* Standard includes. */
#include <stdlib.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"

/* Timer3 interrupt service initialization. */
#include "fr_t3isr.h"

/*-----------------------------------------------------------*/

typedef struct InterprocessSharedDataDefinition
{
    unsigned portCHAR value;
    // .. place more data items here as necessary
} xSHARED;

/*-----------------------------------------------------------*/

void   mainInitComm();
static void userRead( void *pvParameters );
void   myInterruptHandler(void);

/*-----------------------------------------------------------*/

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    mainInitComm();

    Timer3_Init( 200 , &myInterruptHandler );
    // Function void mainInitComm(void) will be run from an interrupt at frequency 200Hz)
    // Minimum frequency for 100MHz clock is 128Hz, function must be very short to execute

    xTaskCreate( userRead, ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control
    of the scheduler. */
}

/*-----------------------------------------------------------*/

static xSemaphoreHandle     xMySemaphore;
static xSHARED              xSharedVariable;

/*-----------------------------------------------------------*/

void   mainInitComm()
{
    vSemaphoreCreateBinary( xMySemaphore );
    xSharedVariable.value = 1;
}

/*-----------------------------------------------------------*/

static void userRead( void *pvParameters )
{
    portCHAR        state;
    portCHAR        message;

    if ( 0 == xMySemaphore ) return; // not initialized properly

    ( void ) pvParameters;

    state   = bioGetSW2();
    message = 1;
    for (;;)
    {
        if ( bioGetSW2() != state )
        {
            state = bioGetSW2();
            if (state)
            {

                if( pdTRUE == xSemaphoreTake( xMySemaphore, ( 50 / portTICK_RATE_MS) ) )
                {
                    // access shared variables here
                    message = xSharedVariable.value;
                    // ...
                    message = !message;
                    // ...
                    xSharedVariable.value = message;

                    // release the variables as soon as possible...
                    xSemaphoreGive( xMySemaphore );

                    // ... and continue processing data as necessary
                } else {
                    // deal with inability to obtain the access within specified period of time
                    // as necessary deal with inability to obtain the access within specified period of time
                    bioSetAB4_LED2(bioLED_ON);
                }
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
void myInterruptHandler(void) 
{
    static portSHORT count = 0;
    static xSHARED   state = {1}; // see also how to initialize more complex structures

    static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;    

    if( pdTRUE == xSemaphoreTake( xMySemaphore, 0) )
    {
        // There was something int he queue and we can import data from the received message
        memcpy( (void*)state, (void*)xSharedVariable, sizeof(xSHARED) );
        xSemaphoreGiveFromISR( xMySemaphore, &xHigherPriorityTaskWoken );
        if ( state.value )
        {
            bioSetAB4_LED1(bioLED_ON);
        } else {
            bioSetAB4_LED1(bioLED_OFF);
        }
    }

    if( xHigherPriorityTaskWoken != pdFALSE )
    {
        // We should switch context so the ISR returns to a different task.
        taskYIELD ();
    }

    count++;
    if (count >=100)
    {
        count = 0;
        if ( state.value || bioGetLED() )
            bioToggleLED();
    }

}
