/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"


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
static void userExec( void *pvParameters );

/*-----------------------------------------------------------*/

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    mainInitComm();

    mainInitTasks();

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control
    of the scheduler. */
}

/*-----------------------------------------------------------*/

static xQueueHandle xComQueue;

/*-----------------------------------------------------------*/

void mainInitComm( void )
{
    xComQueue = xQueueCreate( (unsigned portBASE_TYPE ) 5, ( unsigned portBASE_TYPE ) sizeof( xMESSAGE ) );
    vQueueAddToRegistry(xComQueue, ( signed portCHAR * ) "QUEUE"); // optional name registration
}

/*-----------------------------------------------------------*/

void mainInitTasks( void )
{
    xTaskCreate( userRead, ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( userExec, ( signed portCHAR * ) "EXEC",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void userRead( void *pvParameters )
{
    portCHAR        state;
    portCHAR        toggle;
    xMESSAGE        message;
    portBASE_TYPE   status;

    if ( 0 == xComQueue ) return; // not initialized properly

    ( void ) pvParameters;

    state  = bioGetSW2();
    toggle = 1;
    for (;;)
    {
        portCHAR newstate = bioGetSW2();
        if ( newstate != state )
        {
            state = newstate;
            if (state)
            {
                toggle = !toggle;

                message.value = toggle;
                status = xQueueSend( xComQueue, (void*) &message, 0);
                if ( status != pdTRUE )
                    bioSetAB4_LED2(bioLED_ON);
            }
        }
        // taskYIELD();
        // this taskYIELD() could be optional if both are true:
        // 1)  RTOS is pre-emptive, and
        // 2)  userRead task has lower priority than userExec
        vTaskDelay( (10 / portTICK_RATE_MS) );
        // vTaskDelay will allow all lower priority tasks to run
        // while this task sleeps. 
        // Also, short sleep will serve as button debouncing feature
    }
}

/*-----------------------------------------------------------*/

static void userExec( void *pvParameters )
{
    xMESSAGE        message;
    portBASE_TYPE   status;

    ( void ) pvParameters;

    if ( 0 == xComQueue ) return; // not initialized properly

    bioSetLED(bioLED_ON);
    for (;;)
    {
        status = xQueueReceive( xComQueue, (void*) &message, portMAX_DELAY );
        // we will sleep while waiting for incoming information up to the specified time
        if ( pdTRUE == status)
        {
            if (message.value)  bioSetLED(bioLED_ON);
            else                bioSetLED(bioLED_OFF);
        }
        else
        {
            // nothing received, the queue reading timed out
            // that means that no data was sent during the last portMAX_DELAY time ticks
            // perform some other periodical maintenance tasks as necessary
        }
    }
}
