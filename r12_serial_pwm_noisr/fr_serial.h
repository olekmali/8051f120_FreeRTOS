#ifndef FR_SERIAL_H
#define FR_SERIAL_H

#include "fr_uart.h"

/*
 * If you send data from more than one thread at the same time you must use vSerialTaskMultithreadingSafe
 * Note: reading data with echo also sends data back
 * Note: A binary semaphore will be created: on the very first use one for vSerialPut* and one for vSerialGet*
 */
void vSerialSetThreadSafe( void );


/*
 * Blocking - means that function will wait until all data is placed in the UART sending buffer
 * Assuming no other task attempts to send data at the same time and no control flow on data to be sent:
 *     Maximum guaranteed time spent waiting is the size of the data 
 *         times time to send one character/byte
 *     Add UART send buffer size times time to send one character/byte 
 *         to get absolute time until all data is sent out
 */
portBASE_TYPE  vSerialPutStringBlocking ( xComPortHandle pxPort, const portCHAR * pcString );
portBASE_TYPE  vSerialPutBinaryBlocking ( xComPortHandle pxPort, const void     * pbObject, unsigned portSHORT uLength );

/* 
 * Blocking - means that function will wait expected data is received 
 *            which depends on the other side of transmission
 */
portBASE_TYPE  vSerialGetStringBlocking ( xComPortHandle pxPort,       portCHAR * pcString, unsigned portSHORT usStringMaxLength );
portBASE_TYPE  vSerialGetStringNoEchoBlocking ( xComPortHandle pxPort, portCHAR * pcString, unsigned portSHORT usStringMaxLength );
portBASE_TYPE  vSerialEdtStringBlocking ( xComPortHandle pxPort,       portCHAR * pcString, unsigned portSHORT usStringMaxLength );
portBASE_TYPE  vSerialGetBinaryBlocking ( xComPortHandle pxPort,       void     * pbObject, unsigned portSHORT uLength );

#endif
