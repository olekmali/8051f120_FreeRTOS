
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "fr_serial.h"

/*-----------------------------------------------------------*/

static portBASE_TYPE        vSerialTaskMultithreadingSafe = pdFALSE;
static xSemaphoreHandle     xSerialPutSemaphore;
static xSemaphoreHandle     xSerialGetSemaphore;

void vSerialSetThreadSafe( void )
{
    if ( pdTRUE != vSerialTaskMultithreadingSafe )
    {
        vSemaphoreCreateBinary( xSerialPutSemaphore );
        vSemaphoreCreateBinary( xSerialGetSemaphore );
        vSerialTaskMultithreadingSafe = pdTRUE;
    }
}


/*-----------------------------------------------------------*/

portBASE_TYPE vSerialPutStringBlocking ( xComPortHandle pxPort, const portCHAR * pcString )
{
    /* There is only one port supported. */
    ( void ) pxPort;

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        while ( pdFALSE == xSemaphoreTake( xSerialPutSemaphore, portMAX_DELAY ) ) 
            ;

    while ( *pcString != '\0' )
    {
        if ( *pcString == '\n')
        {
            if (    xSerialPutChar( pxPort, '\r', portMAX_DELAY ) != pdPASS ||
                    xSerialPutChar( pxPort, '\n', portMAX_DELAY ) != pdPASS )
            {
                if ( pdTRUE == vSerialTaskMultithreadingSafe )
                    xSemaphoreGive( xSerialPutSemaphore );
                return(pdFALSE);
            }
        } else {
            if( xSerialPutChar( pxPort, *pcString, portMAX_DELAY ) != pdPASS )
            {
                if ( pdTRUE == vSerialTaskMultithreadingSafe )
                    xSemaphoreGive( xSerialPutSemaphore );
                return(pdFALSE);
            }
        }
        ++pcString;
    }

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        xSemaphoreGive( xSerialPutSemaphore );
    return(pdTRUE);
}

/*-----------------------------------------------------------*/

portBASE_TYPE  vSerialPutBinaryBlocking ( xComPortHandle pxPort, const void * pbObject, unsigned portSHORT uLength )
{
    unsigned portCHAR * byte = (unsigned portCHAR *) pbObject;
    unsigned portCHAR * bend = byte+uLength;

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        while ( pdFALSE == xSemaphoreTake( xSerialPutSemaphore, portMAX_DELAY ) ) 
            ;

    /* There is only one port supported. */
    ( void ) pxPort;

    while (byte<bend)
    {
        if( xSerialPutChar( pxPort, *byte, portMAX_DELAY ) != pdPASS )
        {
            if ( pdTRUE == vSerialTaskMultithreadingSafe )
                xSemaphoreGive( xSerialPutSemaphore );
            return(pdFALSE);
        }
        ++byte;
    }

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        xSemaphoreGive( xSerialPutSemaphore );
    return(pdTRUE);
}

/*-----------------------------------------------------------*/

portBASE_TYPE vSerialGetStringBlocking ( xComPortHandle pxPort, portCHAR * pcString, unsigned portSHORT usStringMaxLength )
{

    unsigned portCHAR  cByteRxed;
    unsigned portSHORT count = 0;
    usStringMaxLength--; // to make room for final \0

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
    {
        while ( pdFALSE == xSemaphoreTake( xSerialGetSemaphore, portMAX_DELAY ) ) 
            ;
        while ( pdFALSE == xSemaphoreTake( xSerialPutSemaphore, portMAX_DELAY ) ) 
            ;
    }
 
    /* There is only one port supported. */
    ( void ) pxPort;

    while (1) {
        if ( !xSerialGetChar( pxPort, &cByteRxed, portMAX_DELAY ) )
        {
            // timeout occurred, return early
            *pcString=0;
            if ( pdTRUE == vSerialTaskMultithreadingSafe )
            {
                xSemaphoreGive( xSerialPutSemaphore );
                xSemaphoreGive( xSerialGetSemaphore );
            }
            return(pdFALSE);
        }
        switch(cByteRxed) {
            case '\b': // backspace
                break;
            case '\n':
            case '\r': // CR or LF
                xSerialPutChar( pxPort, '\r', portMAX_DELAY );            // <-- remove if NOECHO desired
                xSerialPutChar( pxPort, '\n', portMAX_DELAY );            // <-- remove if NOECHO desired
                *pcString=0;
                if ( pdTRUE == vSerialTaskMultithreadingSafe )
                {
                    xSemaphoreGive( xSerialPutSemaphore );
                    xSemaphoreGive( xSerialGetSemaphore );
                }
                return(pdTRUE);
            default:
                if (count<usStringMaxLength) {
                    *pcString++=cByteRxed;
                    count++;
                    xSerialPutChar( pxPort, cByteRxed, portMAX_DELAY ); // <-- remove if NOECHO desired
                }
                else                                                    // <-- remove if NOECHO desired
                {                                                       // <-- remove if NOECHO desired
                    xSerialPutChar( pxPort, '\a', portMAX_DELAY );      // <-- remove if NOECHO desired
                }                                                       // <-- remove if NOECHO desired
                break;
        }
    }
}

portBASE_TYPE vSerialGetStringNoEchoBlocking ( xComPortHandle pxPort, portCHAR * pcString, unsigned portSHORT usStringMaxLength )
{

    unsigned portCHAR  cByteRxed;
    unsigned portSHORT count = 0;
    usStringMaxLength--; // to make room for final \0

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        while ( pdFALSE == xSemaphoreTake( xSerialGetSemaphore, portMAX_DELAY ) ) 
            ;

    /* There is only one port supported. */
    ( void ) pxPort;

    while (1) {
        if ( !xSerialGetChar( pxPort, &cByteRxed, portMAX_DELAY ) )
        {
            // timeout occurred, return early
            *pcString=0;
            if ( pdTRUE == vSerialTaskMultithreadingSafe )
                xSemaphoreGive( xSerialGetSemaphore );
            return(pdFALSE);
        }
        switch(cByteRxed) {
            case '\n':
            case '\r': // CR or LF
                *pcString=0;
                if ( pdTRUE == vSerialTaskMultithreadingSafe )
                    xSemaphoreGive( xSerialGetSemaphore );
                return(pdTRUE);
            default:
                if (count<usStringMaxLength) {
                    *pcString++=cByteRxed;
                    count++;
                }
                break;
        }
    }
}

/*-----------------------------------------------------------*/

portBASE_TYPE vSerialEdtStringBlocking ( xComPortHandle pxPort, portCHAR * pcString, unsigned portSHORT usStringMaxLength )
{

    unsigned portCHAR  cByteRxed;
    unsigned portSHORT count = 0;
    usStringMaxLength--; // to make room for final \0

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
    {
        while ( pdFALSE == xSemaphoreTake( xSerialGetSemaphore, portMAX_DELAY ) ) 
            ;
        while ( pdFALSE == xSemaphoreTake( xSerialPutSemaphore, portMAX_DELAY ) ) 
            ;
    }

    /* There is only one port supported. */
    ( void ) pxPort;

    while (1) {
        if ( !xSerialGetChar( pxPort, &cByteRxed, portMAX_DELAY ) )
        {
            // timeout occurred, return early
            *pcString=0;
            if ( pdTRUE == vSerialTaskMultithreadingSafe )
            {
                xSemaphoreGive( xSerialPutSemaphore );
                xSemaphoreGive( xSerialGetSemaphore );
            }
            return(pdFALSE);
        }
        switch(cByteRxed) {
            case '\b': // backspace
                if (count) {
                    xSerialPutChar( pxPort, '\b', portMAX_DELAY );
                    xSerialPutChar( pxPort, ' ',  portMAX_DELAY );
                    xSerialPutChar( pxPort, '\b', portMAX_DELAY );
                    pcString--;
                    count--;
                }
                else
                {
                    xSerialPutChar( pxPort, '\a', portMAX_DELAY );
                }
                break;
            case '\n':
            case '\r': // CR or LF
                xSerialPutChar( pxPort, '\r', portMAX_DELAY );
                xSerialPutChar( pxPort, '\n', portMAX_DELAY );
                *pcString=0;
                if ( pdTRUE == vSerialTaskMultithreadingSafe )
                {
                    xSemaphoreGive( xSerialPutSemaphore );
                    xSemaphoreGive( xSerialGetSemaphore );
                }
                return(pdTRUE);
            default:
                if (count<usStringMaxLength) {
                    *pcString++=cByteRxed;
                    count++;
                    xSerialPutChar( pxPort, cByteRxed, portMAX_DELAY );
                }
                else
                {
                    xSerialPutChar( pxPort, '\a', portMAX_DELAY );
                }
                break;
        }
    }
}

/*-----------------------------------------------------------*/

portBASE_TYPE  vSerialGetBinaryBlocking ( xComPortHandle pxPort, void * pbObject, unsigned portSHORT uLength )
{
    unsigned portCHAR * byte = (unsigned portCHAR *) pbObject;
    unsigned portCHAR * bend = byte+uLength;

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        while ( pdFALSE == xSemaphoreTake( xSerialGetSemaphore, portMAX_DELAY ) ) 
            ;

    /* There is only one port supported. */
    ( void ) pxPort;

    while (byte<bend)
    {
        if( !xSerialGetChar( pxPort, byte, portMAX_DELAY ) )
        {
            if ( pdTRUE == vSerialTaskMultithreadingSafe )
                xSemaphoreGive( xSerialGetSemaphore );
            return(pdFALSE);
        }
        ++byte;
    }

    if ( pdTRUE == vSerialTaskMultithreadingSafe )
        xSemaphoreGive( xSerialGetSemaphore );
    return(pdTRUE);
}
/*-----------------------------------------------------------*/
