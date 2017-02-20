/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"


/*-----------------------------------------------------------*/

typedef struct InterprocessSharedDataDefinition
{
    unsigned portCHAR value;
    // .. place more data items here as necessary
} xSHARED;

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

static xSemaphoreHandle     xMySemaphore;
static xSHARED              xSharedVariable;

/*-----------------------------------------------------------*/

void   mainInitComm( void )
{
    vSemaphoreCreateBinary( xMySemaphore );
//  xMySemaphore = xSemaphoreCreateMutex( void ); // Mutex type semaphore can also be used in this design pattern
    xSharedVariable.value = 1;
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
    xTaskCreate( userRead, ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( userExec, ( signed portCHAR * ) "EXEC",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
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

static void userExec( void *pvParameters )
{
    portCHAR message;

    ( void ) pvParameters;
    if ( 0 == xMySemaphore ) return; // not initialized properly

    bioSetLED(bioLED_ON);
    for (;;)
    {

        if( pdTRUE == xSemaphoreTake( xMySemaphore, ( 50 / portTICK_RATE_MS) ) )
        {
            // access shared variables here
            message = xSharedVariable.value;

            // release the variables as soon as possible...
            xSemaphoreGive( xMySemaphore );

            // ... and continue processing data as necessary
            if (message) bioSetLED(bioLED_ON);
            else         bioSetLED(bioLED_OFF);

        } else {
            // as necessary deal with inability to obtain the access within specified period of time
            bioSetAB4_LED2(bioLED_ON);
        }


        vTaskDelay( (10 / portTICK_RATE_MS) );
    }
}
