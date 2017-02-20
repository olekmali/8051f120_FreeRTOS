/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "fr_init.h"

/* Constants to set the clock frequency. */
#define initSELECT_INTERNAL_OSC     ( ( unsigned portCHAR ) 0x80 )
#define initDIVIDE_CLOCK_BY_1       ( ( unsigned portCHAR ) 0x03 )
#define initPLL_USES_INTERNAL_OSC   ( ( unsigned portCHAR ) 0x04 )
#define initFLASH_READ_TIMING       ( ( unsigned portCHAR ) 0x30 )
#define initPLL_POWER_ON            ( ( unsigned portCHAR ) 0x01 )
#define initPLL_NO_PREDIVIDE        ( ( unsigned portCHAR ) 0x01 )
#define initPLL_FILTER              ( ( unsigned portCHAR ) 0x01 )
#define initPLL_MULTIPLICATION      ( ( unsigned portCHAR ) 0x04 )
#define initENABLE_PLL              ( ( unsigned portCHAR ) 0x02 )
#define initPLL_LOCKED              ( ( unsigned portCHAR ) 0x10 )
#define initSELECT_PLL_AS_SOURCE    ( ( unsigned portCHAR ) 0x02 )

/* We want the Cygnal to act as much as possible as a standard 8052. */
#define initAUTO_SFR_OFF            ( ( unsigned portCHAR ) 0 )

/* Constants required to setup the IO pins for serial comms. */
#define initENABLE_COMS             ( ( unsigned portCHAR ) 0x04 )
#define initCOMS_LINES_TO_PUSH_PULL ( ( unsigned portCHAR ) 0x03 )

/*
 * Setup the Cygnal microcontroller for its fastest operation.
 */
void initSetupSystemClock( void );

/*-----------------------------------------------------------*/

/*
 * Setup the hardware prior to using the scheduler.  Most of the Cygnal
 * specific initialisation is performed here leaving standard 8052 setup
 * only in the driver code.
 */
void initSetupHardware( void )
{
    unsigned portCHAR ucOriginalSFRPage;

    /* Remember the SFR page before it is changed so it can get set back
    before the function exits. */
    ucOriginalSFRPage = SFRPAGE;

    /* Setup the SFR page to access the config SFR's. */
    SFRPAGE = CONFIG_PAGE;

    /* Don't allow the microcontroller to automatically switch SFR page, as the
    SFR page is not stored as part of the task context. */
    SFRPGCN = initAUTO_SFR_OFF;

    // Disable watchdog timer
    WDTCN = 0xde;
    WDTCN = 0xad;

    P0MDOUT |= 0x01;                // Set TX1 pin to push-pull
    P1MDOUT |= 0x40;                // Set P1.6(TB_LED) to push-pull
//  P2MDOUT |= 0x00;
//  P3MDOUT |= 0x00;

    // all pins used by the external memory interface are in push-pull mode
    // including /WR (P4.7) and /RD (P4.6) but the reset (P4.5) is open-drain
    P4MDOUT =  0xC0;
    P4 = 0xC0;                      // /WR, /RD, are high, RESET is low

    // You may want to set P4.3 (AB4_LED1) and P4.4 (AB4_LED2) to push-pull
    // You may want to set P4.1 (AB4_SW1)  and P4.2 (AM4_SW2)  to open-drain
    P4MDOUT =  0xD8;
    // You may want to prevent permanent on on AB4 switches by not pulling them down
    P4 = 0xC6;                      // /WR, /RD, SW1, SW2 are high, RESET is low,

    P5MDOUT =  0xFF;                // P5, P6 contain the address lines
    P6MDOUT =  0xFF;                // P5, P6 contain the address lines
    P7MDOUT =  0xFF;                // P7 contains the data lines
    P5 = 0xFF;                      // P5, P6 contain the address lines
    P6 = 0xFF;                      // P5, P6 contain the address lines
    P7 = 0xFF;                      // P7 contains the data lines

    TCON &= ~0x01;                  // Make /INT0 level triggered

    XBR0 = 0x80;                    // Enable CP0, Close PCA0 I/O, Close UART0
    XBR1 = 0x04;                    // Enable INT0 input pin, this puts /INT0 on P0.3.
    XBR2 = 0x44;                    // Enable crossbar and weak pull-up, Enable UART1

    /* Setup a fast system clock. */
    initSetupSystemClock();

    /* Return the SFR page. */
    SFRPAGE = ucOriginalSFRPage;
}
/*-----------------------------------------------------------*/

void initSetupSystemClock( void )
{
    volatile unsigned portSHORT usWait;
    const unsigned portSHORT usWaitTime = ( unsigned portSHORT ) 0x2ff;
    unsigned portCHAR ucOriginalSFRPage;

    /* Remember the SFR page so we can set it back at the end. */
    ucOriginalSFRPage = SFRPAGE;
    SFRPAGE = CONFIG_PAGE;

    /* Use the internal oscillator set to its fasted frequency. */
    OSCICN = initSELECT_INTERNAL_OSC | initDIVIDE_CLOCK_BY_1;

    /* Ensure the clock is stable. */
    for( usWait = 0; usWait < usWaitTime; usWait++ );

    /* Setup the clock source for the PLL. */
    PLL0CN &= ~initPLL_USES_INTERNAL_OSC;

    /* Change the read timing for the flash ready for the fast clock. */
    SFRPAGE = LEGACY_PAGE;
    FLSCL |= initFLASH_READ_TIMING;

    /* Turn on the PLL power. */
    SFRPAGE = CONFIG_PAGE;
    PLL0CN |= initPLL_POWER_ON;

    /* Don't predivide the clock. */
    PLL0DIV = initPLL_NO_PREDIVIDE;

    /* Set filter for fastest clock. */
    PLL0FLT = initPLL_FILTER;
    PLL0MUL = initPLL_MULTIPLICATION;

    /* Ensure the clock is stable. */
    for( usWait = 0; usWait < usWaitTime; usWait++ );

    /* Enable the PLL and wait for it to lock. */
    PLL0CN |= initENABLE_PLL;
    for( usWait = 0; usWait < usWaitTime; usWait++ )
    {
        if( PLL0CN & initPLL_LOCKED )
        {
            break;
        }
    }

    /* Select the PLL as the clock source. */
    CLKSEL |= initSELECT_PLL_AS_SOURCE;

    /* Return the SFR back to its original value. */
    SFRPAGE = ucOriginalSFRPage;
}
