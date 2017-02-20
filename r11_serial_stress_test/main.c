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
#define mainCOM_TEST_BUFF_IN_SIZE   ( ( unsigned portBASE_TYPE ) 5 )
#define mainCOM_TEST_BUFF_OUT_SIZE  ( ( unsigned portBASE_TYPE ) 5 )

#define mainCOM_TEST_MAX_CHUNK      ( ( unsigned portBASE_TYPE ) 11 )
#define mainCOM_TEST_WAIT1          ( ( unsigned portBASE_TYPE ) 15 )
#define mainCOM_TEST_WAIT2          ( ( unsigned portBASE_TYPE ) 16 )


/*-----------------------------------------------------------*/

typedef struct Send_param_t {
    unsigned char   num;
    char            chr;
};

/* Handle to the com port used by both tasks. */
static xComPortHandle xPort = NULL;

void   mainInitTasks( void );
static void userSend1( void *pvParameters );
static void userSend2( void *pvParameters );
static void userSendC( void *pvParameters );
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
    vSerialSetThreadSafe(); // <-- comment this line to see what happens when the library ceases to be thread safe

    mainInitTasks();

    /* Finally kick off the scheduler.  This function should never return. */
    vTaskStartScheduler();

    /* Should never reach here as the tasks will now be executing under control of the scheduler. */
}

/*-----------------------------------------------------------*/

void   mainInitTasks( void )
{
    static __code const struct Send_param_t S1 = {10, '+'};
    static __code const struct Send_param_t S2 = {10, '-'};
    static __code const struct Send_param_t S3 = {10, '*'};
    xTaskCreate( userSend1,     ( signed portCHAR * ) "SEND11", configMINIMAL_STACK_SIZE, &S1,  tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( userSend1,     ( signed portCHAR * ) "SEND12", configMINIMAL_STACK_SIZE, &S2,  tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( userSend2,     ( signed portCHAR * ) "SEND2",  configMINIMAL_STACK_SIZE, &S3,  tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
//  xTaskCreate( userSendC,     ( signed portCHAR * ) "SENDC",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( userShowAlive, ( signed portCHAR * ) "ALIVE",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void userSend1( void *pvParameters )
{
    char buffer[mainCOM_TEST_MAX_CHUNK];
    unsigned char n;
    struct Send_param_t* params = (Send_param_t*) pvParameters;
    if ( params->num > (mainCOM_TEST_MAX_CHUNK-1) )
        params->num = (mainCOM_TEST_MAX_CHUNK-1);
    for(n=0; n<params->num; n++) buffer[n]= params->chr;
    buffer[n]='\0';
    
    for (;;)
    {
        vSerialPutStringBlocking(xPort, buffer);
        // taskYIELD();
        vTaskDelay( (mainCOM_TEST_WAIT1 / portTICK_RATE_MS) );
    }
}

static void userSend2( void *pvParameters )
{
    char buffer[mainCOM_TEST_MAX_CHUNK];
    unsigned char n;
    struct Send_param_t* params = pvParameters;
    if ( params->num > (mainCOM_TEST_MAX_CHUNK-1) )
        params->num = (mainCOM_TEST_MAX_CHUNK-1);
    for(n=0; n<params->num; n++) buffer[n]= params->chr;
    buffer[n]='\0';
    
    for (;;)
    {
        vSerialPutStringBlocking(xPort, buffer);
        // taskYIELD();
        vTaskDelay( (mainCOM_TEST_WAIT2 / portTICK_RATE_MS) );
    }
}

static void userSendC( void *pvParameters )
{
    ( void ) pvParameters;
    for (;;)
    {
        xSerialPutChar(xPort, '.', portMAX_DELAY);
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
        
        // this is simpler wait but does not guaratee keeping the frequency
        // vTaskDelay( (500 / portTICK_RATE_MS) );
    }
}
