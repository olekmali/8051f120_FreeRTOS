/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

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

/* Handle to the com port used by both tasks. */
static xComPortHandle xPort = NULL;

void   mainInitTasks( void );
static void userDialog( void *pvParameters );
static void userShowAlive( void *pvParameters );

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

    mainInitTasks();

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control of the scheduler. */
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
	xTaskCreate( userDialog,    ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
	xTaskCreate( userShowAlive, ( signed portCHAR * ) "ALIV", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void userDialog( void *pvParameters )
{
    static portCHAR buffer[40];
    ( void ) pvParameters; // useless instruction to prevent warnings - unused variable warning

    vSerialPutStringBlocking(xPort, "System Ready\nPlease enter the text to be echoed\n");
    for (;;)
    {
        portBASE_TYPE   dataOk;
        vSerialPutStringBlocking(xPort, "> ");
        dataOk = vSerialEdtStringBlocking(xPort, buffer, sizeof(buffer) );
        if ( pdPASS == dataOk )
        {
            portBASE_TYPE   allSentOk;
            vSerialPutStringBlocking(xPort, "E:"   );   // we assume that we do not send too much data
            vSerialPutStringBlocking(xPort, buffer );   // so that there is always enough room in the queue
            vSerialPutStringBlocking(xPort, "\n"   );   // otherwise, we have to check if pdTRUE is returned
            allSentOk = vSerialPutFlush( xPort, 50 / portTICK_RATE_MS );    // flush - waits until all printed but only up to N ticks
                // Blocking of printing itself occurs only until there is room in the buffer, not until the data is actually sent out
                // Note, at 9600 bps it takes about 1ms to transmit one character. 
                //      one should take it into account when waiting till all is sent
            if ( pdPASS == allSentOk )
            {
                // all is fine, nothing to do
            } else {
                /* 
                    we are not sure what else we could do in this simple program
                    - light up an error LED indicator?
                    - set a global error code variable for diagnostics purposes?
                    - log the malfunction is some kind of error log in flash memory?
                */
            }
        } else {
            // we do not expect to arrive here at all as reading blocks forever if necessary
            vSerialPutStringBlocking(xPort, "Unexpected failure encountered!\a\n\n" );
        }
    }
}

static void userShowAlive( void *pvParameters )
{
    portTickType xLastWakeTime = xTaskGetTickCount();
	( void ) pvParameters;
    for (;;)
    {
        bioToggleLED();
        
        // this wait will compensate for time spent by the current task in suspend mode 
        vTaskDelayUntil( &xLastWakeTime, (500 / portTICK_RATE_MS) );
        
        // this is simpler wait but does not guarantee keeping the frequency
        // vTaskDelay( (500 / portTICK_RATE_MS) );
    }
}
