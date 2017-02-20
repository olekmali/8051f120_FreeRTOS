/* Standard includes. */
#include <stdlib.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"

/* Timer3 interrupt service initialization. */
#include "fr_t3isr.h"

/* Buffered serial port implementation. */
#include "fr_serial.h"

/*-----------------------------------------------------------*/

static __code const struct
{
    const char  letter;
    __code const char *morse;
} cxTABLE[] =
{
    { '0', "----- " },
    { '1', ".---- " },
    { '2', "..--- " },
    { '3', "...-- " },
    { '4', "....- " },
    { '5', "..... " },
    { '6', "-.... " },
    { '7', "--... " },
    { '8', "---.. " },
    { '9', "----. " },
    { 'A', ".- " },
    { 'B', "-... " },
    { 'C', "-.-. " },
    { 'D', "-.. " },
    { 'E', ". " },
    { 'F', "..-. " },
    { 'G', "--. " },
    { 'H', ".... " },
    { 'I', ".. " },
    { 'J', ".--- " },
    { 'K', ".-.- " },
    { 'L', ".-.. " },
    { 'M', "-- " },
    { 'N', "-. " },
    { 'O', "--- " },
    { 'P', ".--. " },
    { 'Q', "--.- " },
    { 'R', ".-. " },
    { 'S', "... " },
    { 'T', "- " },
    { 'U', "..- " },
    { 'V', "...- " },
    { 'W', ".-- " },
    { 'X', "-..- " },
    { 'Y', "-.-- " },
    { 'Z', "--.. " },
    { ' ', "    " },
    { '.', ".-.-.- " },
    { ',', "--..-- " },
    { '?', "..--.. " },
};
/*
   1. short mark, dot or 'dit' (·)      - one unit long
   2. longer mark, dash or 'dah' (-)    - three units long
   3. intra-character gap (between the dots and dashes within a character) - one unit long
   4. short gap (between letters)       - three units long
   5. medium gap (between words)        - seven units long
*/

/*-----------------------------------------------------------*/

/* Baud rate used by the serial port tasks. */
#define mainCOM_TEST_BAUD_RATE      ( ( unsigned portLONG ) 9600 )
#define mainCOM_TEST_BUFF_IN_SIZE   ( ( unsigned portBASE_TYPE ) 10 )
#define mainCOM_TEST_BUFF_OUT_SIZE  ( ( unsigned portBASE_TYPE ) 81 )

/*-----------------------------------------------------------*/

static xComPortHandle xPort = NULL;

/*-----------------------------------------------------------*/

#define mainMSG_BUFF_SIZE           ( ( unsigned portBASE_TYPE ) 10 )

typedef struct InterprocessMessageDefinition
{
    unsigned portCHAR value;
    // .. place more data items here as necessary
} xMESSAGE;

/*-----------------------------------------------------------*/

void   mainInitComm( void );
void   mainInitTasks( void );
static void userDialog( void *pvParameters );
void   myInterruptHandler( void );

/*-----------------------------------------------------------*/

void main( void )
{
    initSetupHardware();

    xPort = xSerialPortInitMinimal( mainCOM_TEST_BAUD_RATE, mainCOM_TEST_BUFF_IN_SIZE, mainCOM_TEST_BUFF_OUT_SIZE );
    vSerialSetThreadSafe();

    mainInitComm();


/* HW HINT: Enable is needed for this homework. Otherwise, delete

    Timer3_Init( 200 , &myInterruptHandler ); // HW HINT: pick a different frequency if desired
        // Minimum frequency for 100MHz clock is 128Hz, function must be very short to execute
*/


    mainInitTasks();

    vTaskStartScheduler();
}

/*-----------------------------------------------------------*/

static xQueueHandle xComQueue;

/*-----------------------------------------------------------*/

void   mainInitComm( void )
{
    xComQueue = xQueueCreate( (unsigned portBASE_TYPE ) mainMSG_BUFF_SIZE, ( unsigned portBASE_TYPE ) sizeof( xMESSAGE ) );
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
    xTaskCreate( userDialog,    ( signed portCHAR * ) "READ",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
//  xTaskCreate( sendMorse,     ( signed portCHAR * ) "MORS",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void userDialog( void *pvParameters )
{
    static xMESSAGE message_to_send_buffer = {0};
    portBASE_TYPE   dataOk;
    static unsigned portCHAR character;

    ( void ) pvParameters;
    if ( pdFAIL == xComQueue ) return; // not initialized properly

    vSerialPutStringBlocking(xPort, "Please enter the text to be encoded:\n");
    for (;;)
    {
        vSerialPutStringBlocking(xPort, "> ");
        dataOk = xSerialGetChar(xPort, &character, portMAX_DELAY);
        if ( pdPASS == dataOk )
        {
            xSerialPutChar(xPort, character, portMAX_DELAY);
            // HW HINT: send one or more messages either to the other task or to the interrupt depending on your design
            // ...
        } else {
            // HW HINT: do you need this else branch at all?
        }
    }
}

/*-----------------------------------------------------------*/

static void sendMorse( void *pvParameters )
{
    static xMESSAGE message_received_buffer = {0};
    portBASE_TYPE   dataOk;

    ( void ) pvParameters;
    if ( 0 == xComQueue ) return; // not initialized properly

    for (;;)
    {
        vTaskDelay( (100 / portTICK_RATE_MS) ); // perhaps this should be a single time unit for Morse signal encoding
    }
}

/*-----------------------------------------------------------*/

/* HW HINT: Enable is needed for this homework. Otherwise, delete
// This function is called within an interrupt - it must be as short as possible
// and must not call on any task that might cause any unnecessary delay
void myInterruptHandler( void ) 
{
    static portBASE_TYPE xTaskWokenByReceive = pdFALSE;
    static xMESSAGE  received_message = {0}; // HW HINT: you may need a different initialization
    static portSHORT count = 0;

    count++;
    
//  if ( xQueueReceiveFromISR( xComQueue, (void*) &message_buffer, &xTaskWokenByReceive ) )
//  {
//  }



    if( xTaskWokenByReceive != pdFALSE )
    {
        taskYIELD ();
    }
}
*/
