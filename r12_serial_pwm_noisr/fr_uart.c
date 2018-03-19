/* BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR DEMO PURPOSES */
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "fr_uart.h"

static xQueueHandle xRxedChars;
static xQueueHandle xCharsForTx;

__data static unsigned portBASE_TYPE uxTxEmpty;

/*-----------------------------------------------------------*/

/* Contents of this function may be (C)SiliconLabs as it is taken from their demo program */
xComPortHandle xSerialPortInitMinimal( unsigned portLONG ulWantedBaud, unsigned portBASE_TYPE uxInQueueLength, unsigned portBASE_TYPE uxOutQueueLength )
{
    unsigned portCHAR ucOriginalSFRPage = SFRPAGE;
    const unsigned portLONG sysclkoverbaud = configCPU_CLOCK_HZ/ulWantedBaud;

    portENTER_CRITICAL();
    {
        uxTxEmpty = pdTRUE;

        /* Create the queues used by the com test task. */
        xRxedChars  = xQueueCreate( uxInQueueLength,  ( unsigned portBASE_TYPE ) sizeof( portCHAR ) );
        xCharsForTx = xQueueCreate( uxOutQueueLength, ( unsigned portBASE_TYPE ) sizeof( portCHAR ) );

        /* Setup the control register for standard n, 8, 1 - variable baud rate. */
        SFRPAGE = UART1_PAGE;
        SCON1   = 0x10;                 // SCON1: mode 0, 8-bit UART, enable RX

        SFRPAGE = TIMER01_PAGE;

        /* Set timer one for desired mode of operation. */
        TMOD   &= ~0xF0;
        TMOD   |=  0x20;                // TMOD: timer 1, mode 2, 8-bit reload

        /* Set the reload and start values for the time. */
        // Legacy Note: Since Timer 0 is used by the TCP/IP Library and forces the 
        // shared T0/T1 prescaler to sysclk/48, Timer 1 may only be clocked 
        // from sysclk or sysclk/48
        
        // If reload value is less than 8-bits, select sysclk
        // as Timer 1 baud rate generator
        if (sysclkoverbaud>>9 < 1) 
        {
            TH1 = -(sysclkoverbaud>>1);
            CKCON |= 0x10;              // T1M = 1; SCA1:0 = xx
        
        // Otherwise, select sysclk/48 prescaler.
        }
        else
        {
            // Adjust for truncation in special case
            // Note: Additional cases may be required if the system clock is changed.
            if ((ulWantedBaud == 115200) && (configCPU_CLOCK_HZ == 98000000))
                TH1 = -((sysclkoverbaud/2/48)+1); 
            else
                TH1 = -(sysclkoverbaud/2/48);
            
            CKCON &= ~0x13;             // Clear all T1 related bits
            CKCON |=  0x02;             // T1M = 0; SCA1:0 = 10
        }

        TL1 = TH1;                      // initialize Timer1
        TR1 = 1;                        // start Timer1  

        SFRPAGE = UART1_PAGE;
        TI1 = 1;                        // Indicate TX1 ready

        EIE2 |= 0x40;                   // Enable the serial port interrupts for UART1

        SFRPAGE = ucOriginalSFRPage;
    }
    portEXIT_CRITICAL();

    /* Unlike some ports, this serial code does not allow for more than one
    com port.  We therefore don't return a pointer to a port structure and can
    instead just return NULL. */
    return NULL;
}

/*-----------------------------------------------------------*/

/* Contents of this function may be GNU-(C) by FreeRTOS.org as it is taken from their demo program of FreeRTOS */
void vUart1_ISR( void ) __interrupt 20
{
    static unsigned portCHAR ucOriginalSFRPage;
    static portCHAR cChar;
    static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* 8051 port interrupt routines MUST be placed within a critical section
    if taskYIELD() is used within the ISR! */

    portENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = UART1_PAGE;
        if( RI1 )
        {
            /* Get the character and post it on the queue of Rxed characters.
            If the post causes a task to wake force a context switch if the woken task
            has a higher priority than the task we have interrupted. */
            cChar = SBUF;
            RI1 = 0;

            xQueueSendFromISR( xRxedChars, &cChar, &xHigherPriorityTaskWoken );
        }

        if( TI1 )
        {
            if( xQueueReceiveFromISR( xCharsForTx, &cChar, &xHigherPriorityTaskWoken ) == ( portBASE_TYPE ) pdTRUE )
                SBUF = cChar;           // Send the next character queued for Tx
            else
                uxTxEmpty = pdTRUE;     // Queue empty, nothing to send

            TI1 = 0;
        }

        SFRPAGE = ucOriginalSFRPage;    // must be restored before any task switching might take place

        if( xHigherPriorityTaskWoken )
        {
            portYIELD();
        }
    }
    portEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

/* Contents of this function may be GNU-(C) by FreeRTOS.org as it is taken from their demo program of FreeRTOS */
portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, portCHAR *pcRxedChar, portTickType xBlockTime )
{
    /* There is only one port supported. */
    ( void ) pxPort;

    /* Get the next character from the buffer.  Return false if no characters
    are available, or arrive before xBlockTime expires. */
    if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
        return ( portBASE_TYPE ) pdTRUE;
    else
        return ( portBASE_TYPE ) pdFALSE;
}
/*-----------------------------------------------------------*/

/* Contents of this function may be GNU-(C) by FreeRTOS.org as it is taken from their demo program of FreeRTOS */
portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, portCHAR cOutChar, portTickType xBlockTime )
{
    unsigned portCHAR ucOriginalSFRPage;
    portBASE_TYPE xReturn;

    /* There is only one port supported. */
    ( void ) pxPort;

    portENTER_CRITICAL();
    {
        if( uxTxEmpty == pdTRUE )
        {
            ucOriginalSFRPage = SFRPAGE;
            SFRPAGE = UART1_PAGE;
            SBUF = cOutChar;
            SFRPAGE = ucOriginalSFRPage;
            uxTxEmpty = pdFALSE;
            xReturn = ( portBASE_TYPE ) pdTRUE;
        }
        else
        {
            xReturn = xQueueSend( xCharsForTx, &cOutChar, xBlockTime );
        }
    }
    portEXIT_CRITICAL();

    return xReturn;
}
/*-----------------------------------------------------------*/

portBASE_TYPE  vSerialPutFlush  ( xComPortHandle pxPort, portTickType xBlockTime )
{
    /* There is only one port supported. */
    ( void ) pxPort;

    while ( xBlockTime>0 && uxTxEmpty == pdTRUE )
    {
        vTaskDelay(1);
        if(portMAX_DELAY!=xBlockTime) --xBlockTime;
    }
    return(xBlockTime>0);
}

/*-----------------------------------------------------------*/

void xSerialClose( xComPortHandle xPort )
{
    unsigned portCHAR ucOriginalSFRPage;

    /* Not implemented in this port. */
    ( void ) xPort;

    portENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;

        // Disable Timer1
        SFRPAGE = TIMER01_PAGE;
        TR1 = 0;                        // Stop Timer1
        TMOD = 0x00;                    // Restore the TMOD register to its reset value
        CKCON = 0x00;                   // Restore the CKCON register to its reset value

        // Disable UART1
        SFRPAGE = UART1_PAGE;
        SCON1 = 0x00;                   // Disable UART1
        EIE2 &= !0x40;                  // Disable the serial port interrupts for UART1

        SFRPAGE = ucOriginalSFRPage;
    }
    portEXIT_CRITICAL();

}
/*-----------------------------------------------------------*/
