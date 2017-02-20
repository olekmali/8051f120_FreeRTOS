/* A co-routine will execute until it either blocks, yields or is 
preempted by a task. Co-routines execute cooperatively so one co-
routine cannot be preempted by another, but can be preempted by a task. 
Calls to API functions that could cause the co-routine to block can only 
be made from the co-routine function itself - not from within a function 
called by the co-routine. */

/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
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
void   mainInitCoroutines( void );
static void userRead( xCoRoutineHandle xHandle, unsigned portBASE_TYPE uxIndex );
static void userExec( xCoRoutineHandle xHandle, unsigned portBASE_TYPE uxIndex );

/*-----------------------------------------------------------*/

#define PRIORITY_0        0

/*-----------------------------------------------------------*/

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    mainInitComm();

    /* Register co-routines and tasks here */
    mainInitTasks();
    mainInitCoroutines();

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

void   mainInitTasks( void )
{
}

/*-----------------------------------------------------------*/

void mainInitCoroutines( void )
{
    xCoRoutineCreate( userRead, PRIORITY_0, 0 );
    xCoRoutineCreate( userExec, PRIORITY_0, 0 );
}

/*-----------------------------------------------------------*/

// Note! Set configUSE_IDLE_HOOK 1 within FreeRTOSConfig.h
void vApplicationIdleHook( void )
{
    vCoRoutineSchedule();
    // This will run co-routine scheduler within the idle task
    // and allow co-routines to run whenever all tasks sleep or block
}

/*-----------------------------------------------------------*/

static void userRead( xCoRoutineHandle xHandle, unsigned portBASE_TYPE uxIndex )
{
    // Variables in co-routines must be declared static if they must maintain value across a blocking call.
    static portCHAR        state;
    static xMESSAGE        message;
    static portBASE_TYPE   status;

    ( void ) uxIndex;

    if ( 0 == xComQueue ) return; // not initialized properly

    // Co-routines must begin with a call to crSTART().
    crSTART( xHandle );

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
                crQUEUE_SEND( xHandle, xComQueue, (void*) &message, 0, &status );
                if ( status != pdTRUE )
                    bioSetAB4_LED2(bioLED_ON);
            }
        }
        crDELAY( xHandle, (10 / portTICK_RATE_MS) );
    }
    // Co-routines must end with a call to crEND().
    crEND();
}

/*-----------------------------------------------------------*/

static void userExec( xCoRoutineHandle xHandle, unsigned portBASE_TYPE uxIndex )
{
    // Variables in co-routines must be declared static if they must maintain value across a blocking call.
    static xMESSAGE        message;
    static portBASE_TYPE   status;

    ( void ) uxIndex;

    if ( 0 == xComQueue ) return; // not initialized properly

    // Co-routines must begin with a call to crSTART().
    crSTART( xHandle );

    bioSetLED(bioLED_ON);
    for (;;)
    {
        crQUEUE_RECEIVE( xHandle, xComQueue, (void*) &message, portMAX_DELAY, &status );
        // we will sleep while waiting for incoming information up to the specified time
        if ( pdTRUE == status)
        {
            if (message.value)  bioSetLED(bioLED_ON);
            else                bioSetLED(bioLED_OFF);
        }
        else
        {
            // nothing received, the queue reading timed out
            // perform some other periodical maintenance tasks as necessary
        }
    }
    // Co-routines must end with a call to crEND().
    crEND();
}
