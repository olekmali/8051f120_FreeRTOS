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
    // Note: in this scheme semaphore must be binary, and must not be mutex type
    // because it is taken by one process and released by a different processes
    xSemaphoreTake( xMySemaphore, 0); // create and immediately set it as "taken"
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
                // access shared variables here
                message = xSharedVariable.value;
                // ...
                message = !message;
                // ...
                xSharedVariable.value = message;

                // release the semaphore to indicate that a variable was updated and can be processed on the other end
                xSemaphoreGive( xMySemaphore );
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

        while ( pdFALSE == xSemaphoreTake( xMySemaphore,  portMAX_DELAY ) )
            ; 
        // This will block up to forever but only until the new were deposited on the other end
        // Since it can be taken that means new data that can be processed were deposited into the global variable

        message = xSharedVariable.value;
        // Data need to be picked up ASAP so that they are not overwritten by the other side with new data
        // We are not releasing the semaphore here. The loop will go around and wait again for indication of new data.

        // ... and continue processing data as necessary
        if (message) bioSetLED(bioLED_ON);
        else         bioSetLED(bioLED_OFF);

    }
}
