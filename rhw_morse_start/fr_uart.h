#ifndef FR_UART_H
#define FR_UART_H

typedef void * xComPortHandle;

typedef enum
{
    serCOM1,
    serCOM2,
    serCOM3,
    serCOM4,
    serCOM5,
    serCOM6,
    serCOM7,
    serCOM8
} eCOMPort;

typedef enum
{
    serNO_PARITY,
    serODD_PARITY,
    serEVEN_PARITY,
    serMARK_PARITY,
    serSPACE_PARITY
} eParity;

typedef enum
{
    serSTOP_1,
    serSTOP_2
} eStopBits;

typedef enum
{
    serBITS_5,
    serBITS_6,
    serBITS_7,
    serBITS_8
} eDataBits;

typedef enum
{
    ser50       =    50,
    ser75       =    75,
    ser110      =   110,
    ser134      =   134,
    ser150      =   150,
    ser200      =   200,
    ser300      =   300,
    ser600      =   600,
    ser1200     =  1200,
    ser1800     =  1800,
    ser2400     =  2400,
    ser4800     =  4800,
    ser9600     =  9600,
    ser19200    = 19200,
    ser38400    = 38400,
    ser57600    = 57600,
    ser115200   = 115200
} eBaud;

#define serNO_BLOCK             ( ( portTickType ) 0 )

xComPortHandle xSerialPortInitMinimal(  unsigned portLONG ulWantedBaud, 
                                        unsigned portBASE_TYPE uxInQueueLength, 
                                        unsigned portBASE_TYPE uxOutQueueLength );

portBASE_TYPE  xSerialGetChar   ( xComPortHandle pxPort,       portCHAR *pcRxedChar, portTickType xBlockTime );
portBASE_TYPE  xSerialPutChar   ( xComPortHandle pxPort,       portCHAR cOutChar,    portTickType xBlockTime );
portBASE_TYPE  vSerialPutFlush  ( xComPortHandle pxPort, portTickType xBlockTime );

void           vUart1_ISR       ( void ) __interrupt 20;
void           xSerialClose     ( xComPortHandle xPort );

#endif
