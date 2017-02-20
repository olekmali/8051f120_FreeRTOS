/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Board and Peripherals initialization includes. */
#include "fr_init.h"
#include "fr_board_io.h"

/* Buffered serial port implementation. */
#include "fr_serial.h"

/* ADC0 implementation. */
#include "fr_adc.h"

/* Baud rate used by the serial port tasks. */
#define mainCOM_TEST_BAUD_RATE      ( ( unsigned portLONG ) 9600 )
#define mainCOM_TEST_BUFF_IN_SIZE   ( ( unsigned portBASE_TYPE ) 10 )
#define mainCOM_TEST_BUFF_OUT_SIZE  ( ( unsigned portBASE_TYPE ) 81 )

/*-----------------------------------------------------------*/

/* Handle to the com port used by both tasks. */
static xComPortHandle xPort = NULL;

void   mainInitTasks( void );
static void monitorVoltage    ( void *pvParameters );
static void monitorTemperature( void *pvParameters );
static void userShowAlive     ( void *pvParameters );

/*-----------------------------------------------------------*/

/*
 * Starts all the other tasks, then starts the scheduler.
 */
void main( void )
{
    /* Initialise the hardware including the system clock, serial port and on board LED. */
    initSetupHardware();

    /* Initialize the ADC0 converter */
    bioADC0init();

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
    xTaskCreate( monitorVoltage,    ( signed portCHAR * ) "VOLT", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( monitorTemperature,( signed portCHAR * ) "TEMP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, ( xTaskHandle * ) NULL );
    xTaskCreate( userShowAlive,     ( signed portCHAR * ) "ALIV", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, ( xTaskHandle * ) NULL );
}

/*-----------------------------------------------------------*/

static void monitorVoltage( void *pvParameters )
{
    static __code const unsigned portBASE_TYPE  xADCsequence[2] = {0, 1};
    static __code const unsigned portBASE_TYPE  xADCgains[2]    = {1, 1};
    static   portCHAR   buffer[25];
    unsigned portSHORT  xADCresults[2];

    portTickType xLastWakeTime = xTaskGetTickCount();
    ( void ) pvParameters;
    
    for (;;)
    {
        if ( pdTRUE == bioADC0read( xADCsequence, xADCgains, 2, xADCresults, (2 / portTICK_RATE_MS) ) )
        {

            {
                int v0, v1;
                v0 = (unsigned long)xADCresults[0] * VREF / ADC0_MAX;
                v1 = (unsigned long)xADCresults[1] * VREF / ADC0_MAX;
                sprintf(buffer, "C1: %4dmV C2: %4dmV\n", v0, v1);
            }
            vSerialPutStringBlocking(xPort, buffer );
        } else {
            vSerialPutStringBlocking(xPort, "ERROR: Waited too long for channels 1 & 2!\a\n" );
        }

        vTaskDelayUntil( &xLastWakeTime, (500 / portTICK_RATE_MS) );
    }
}

/*-----------------------------------------------------------*/

static void monitorTemperature( void *pvParameters )
{
    static __code const unsigned portBASE_TYPE  xADCsequence[2] = {8, 8};
    static __code const unsigned portBASE_TYPE  xADCgains[2]    = {2, 2};
    // Note: the temperature sensor channel has three times longer ADC setting time - hence two measurements back to back
    static   portCHAR   buffer[25];
    unsigned portSHORT  xADCresults[2];

    portTickType xLastWakeTime = xTaskGetTickCount();
    ( void ) pvParameters;
    
    for (;;)
    {
        if ( pdTRUE == bioADC0read( xADCsequence, xADCgains, 2, xADCresults, (2 / portTICK_RATE_MS) ) )
        {

            {
                unsigned portSHORT  temperature = xADCresults[1];   // temperature in tenth of a degree C
                portSHORT           temp_int, temp_frac;    // integer and fractional portions of temperature

                // calculate temperature in tenth of a degree (note: coefficients are not really tweaked up to provide useful data)
                temperature = temperature - 42380;
                temperature = (temperature * 10) / 156;
                temp_int    = temperature / 10;
                temp_frac   = temperature - (temp_int * 10);

                sprintf(buffer, "T:%+6d.%1dC\n", temp_int, temp_frac);
            }
            vSerialPutStringBlocking(xPort, buffer );
        } else {
            vSerialPutStringBlocking(xPort, "ERROR: Waited too long for temperature!\a\n" );
        }

        vTaskDelayUntil( &xLastWakeTime, (500 / portTICK_RATE_MS) );
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
