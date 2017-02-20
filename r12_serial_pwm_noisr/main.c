/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"

/* Buffered serial port implementation. */
#include "fr_serial.h"


/* Baud rate used by the serial port tasks. */
#define mainCOM_TEST_BAUD_RATE      ( ( unsigned portLONG ) 9600 )
#define mainCOM_TEST_BUFF_IN_SIZE   ( ( unsigned portBASE_TYPE ) 10 )
#define mainCOM_TEST_BUFF_OUT_SIZE  ( ( unsigned portBASE_TYPE ) 81 )

/*-----------------------------------------------------------*/

typedef struct InterprocessMessageDefinition
{
    unsigned portCHAR time_on;
    unsigned portCHAR time_off;
    // .. place more data items here as necessary
} xMESSAGE;

/*-----------------------------------------------------------*/

static xComPortHandle xPort = NULL;
static xQueueHandle xComQueue;

/*-----------------------------------------------------------*/

void   mainInitComm( void );
void   mainInitTasks( void );
static void taskComm( void *pvParameters );
static void taskWork( void *pvParameters );

/*-----------------------------------------------------------*/

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    xPort = xSerialPortInitMinimal( mainCOM_TEST_BAUD_RATE, mainCOM_TEST_BUFF_IN_SIZE, mainCOM_TEST_BUFF_OUT_SIZE );
    vSerialSetThreadSafe();

    mainInitComm();

    mainInitTasks();

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control of the scheduler. */
}

/*-----------------------------------------------------------*/

void   mainInitComm( void )
{
    xComQueue = xQueueCreate( (unsigned portBASE_TYPE ) 2, ( unsigned portBASE_TYPE ) sizeof( xMESSAGE ) );
    vQueueAddToRegistry(xComQueue, ( signed portCHAR * ) "Q_C2W"); // optional name registration
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
	xTaskCreate( taskComm, ( signed portCHAR * ) "COMM", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
	xTaskCreate( taskWork, ( signed portCHAR * ) "WORK", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void taskComm( void *pvParameters )
{
    static __code const portCHAR sHelpMessage[] = "System Ready\nPlease enter:\np <value> for PWM 0..100%\nt <value> for period 10+ms\ns - stop, r -reset, ? for help\n";

    // static - outside of switched environment, automatic - on stack
    static __xdata portCHAR buffer[10];
    // static __data  - watch out for stack starting point but faster context switch with less stack to move
    // static __xdata - less heap for tasks (we use only 6k out of 8k anyway) but faster context switch with less stack to move
    // static unspecified - large memory model makes __xdata the default location

    portBASE_TYPE   dataOk;
    xMESSAGE        message;
    portBASE_TYPE   status;
    portSHORT       valP=50;    // PWM %
    portSHORT       valT=10;    // PWM period in ms, 100Hz 

	( void ) pvParameters;

    vSerialPutStringBlocking(xPort, "System Ready\n");
    vSerialPutStringBlocking(xPort, sHelpMessage);
    for (;;)
    {
        status = pdFALSE;
        vSerialPutStringBlocking(xPort, "> ");
        dataOk = vSerialGetStringBlocking(xPort, buffer, sizeof(buffer));
        if ( pdPASS == dataOk )
        {
            if ('p' == buffer[0]) {
                valP = atoi(&buffer[1]);
                if (valP<0) valP=0; else if (valP>100) valP=100;   // range limiter
                status = pdTRUE;
            } else if ('t' == buffer[0]) {
                valT = atoi(&buffer[1]);
                if (valT<10) valT=10; else if (valT>500) valT=500; // range limiter
                status = pdTRUE;
            } else  if ('s' == buffer[0]) {
                valP = 0;
                status = pdTRUE;
            } else  if ('r' == buffer[0]) {
                valP = 50;
                valT = 10; // 100Hz
                status = pdTRUE;
            } else  if ('h' == buffer[0] || '?' == buffer[0]) {
                vSerialPutStringBlocking(xPort, sHelpMessage);
            }
            if ( pdTRUE == status) {
                message.time_on =((portLONG)valT)*valP/100; // forced to be computed on long int
                message.time_off=valT-message.time_on;
                status = xQueueSend( xComQueue, (void*) &message, 0);
                if ( status != pdTRUE )
                    vSerialPutStringBlocking(xPort, "-ERR queue full\n");
                else
                    vSerialPutStringBlocking(xPort, "+OK data accepted\n");
            } else {
                vSerialPutStringBlocking(xPort, "+OK no change\n");
            }
        }
        else
        {
                message.time_on = 0;
                message.time_off= valT;
                xQueueSend( xComQueue, (void*) &message, (1000/portTICK_RATE_MS));
                vSerialPutStringBlocking(xPort, "-ERR timeout - PWM sleep mode activated\n");
        }
    }
}

/*-----------------------------------------------------------*/

static void taskWork( void *pvParameters )
{
    portTickType xLastWakeTime = xTaskGetTickCount();
    xMESSAGE        message;
    portBASE_TYPE   status;

    unsigned portCHAR time_on=0, time_off=10;

	( void ) pvParameters;

    bioSetLED(bioLED_OFF);
    for (;;)
    {
        bioSetLED(bioLED_ON);
        vTaskDelayUntil( &xLastWakeTime, time_on );  // this can be used for up to about 600 ticks only
        bioSetLED(bioLED_OFF);
        vTaskDelayUntil( &xLastWakeTime, time_off ); // this can be used for up to about 600 ticks only
        if ( uxQueueMessagesWaiting(xComQueue) )
        {
            status = xQueueReceive( xComQueue, (void*) &message, 0 );
            // we do not want to wait if message cannot be retrieved
            if ( pdTRUE == status)
            {
                time_on = message.time_on  / portTICK_RATE_MS;
                time_off= message.time_off / portTICK_RATE_MS;
            }            
        }

        
    }
}
