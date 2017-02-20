#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "fr_adc.h"

#include "C8051F120.h"

/*-----------------------------------------------------------*/

static xSemaphoreHandle xADC0DataReadySemaphore = 0;
static xSemaphoreHandle xADC0MeasuringSemaphore = 0;
static unsigned portBASE_TYPE xADC0inputs[ADC0_SEQ_MAX];
static unsigned portBASE_TYPE xADC0encoded_gains[ADC0_SEQ_MAX];
static unsigned portBASE_TYPE xADC0count;
static unsigned portSHORT     xADC0values[ADC0_SEQ_MAX];
static unsigned portBASE_TYPE xADC0current;

/*-----------------------------------------------------------*/

void bioADC0init()
{
    unsigned portCHAR ucOriginalSFRPage;

    if ( 0 == xADC0DataReadySemaphore )
    {
        vSemaphoreCreateBinary( xADC0DataReadySemaphore );
        vSemaphoreCreateBinary( xADC0MeasuringSemaphore );
    }
    xSemaphoreTake( xADC0DataReadySemaphore, portMAX_DELAY );

    portENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = ADC0_PAGE;                // set SFR page
        // ADC0 Control
        ADC0CN = 0x41;                      // ADC0 disabled; LOW POWER tracking mode; data is left-justified
                                            // 0x41 ADC0 conversions are initiated manually
                                            // ----00-- ADC0 conversions started manually by setting AD0BUSY=1
                                            // ----01-- ADC0 conversions are initiated on overflow of Timer3
                                            // ----11-- ADC0 conversions are initiated on overflow of Timer2
                                            // 0x01 ADC0 data is left-justified
                                            // 0x40 ADC0 in low power tracking mode - adds 1.5us wait before conversion starts
                                            // 0x40 IS NEEDED in this case because we will change the multiplexer source
                                            //      just before requesting the next reading - need to have setting time

        // Conversion speed and input gain
        ADC0CF = (configCPU_CLOCK_HZ / 2500000) << 3; // ddddd--- ADC0 conversion clock at 2.5 MHz
        ADC0CF |= 0x00;                     // -----ddd ADC) internal gain (PGA): 0-1, 1-2, 2-4, 3-8, 4-16, 6-0.5

        // Voltage Source and mode
        REF0CN = 0x07;                      // enable: 0x01 on-chip VREF, 0x02 VREF output buffer for ADC and DAC, and 0x04 temp sensor
        AMX0CF = 0x00;                      // 0x00 Select 8 independent inputs, 0x0F select 4 differential pairs
        AMX0SL = 0x00;                      // Select Channel 0 as ADC mux output,
                                            // When AMX0CF = 0x00 then AMX0SL = channel number, 8 for TEMP sensor

        EIE2 |= 0x02;                       // enable ADC interrupts when the conversion is complete
        AD0EN = 1;                          // enable ADC
        // The scheduler will enable global interrupts when ready

        SFRPAGE = ucOriginalSFRPage;
    }
    portEXIT_CRITICAL();
}


/*-----------------------------------------------------------*/

void bioADC0shutdown()
{
    unsigned portCHAR ucOriginalSFRPage;
    xSemaphoreGive( xADC0DataReadySemaphore );
    portENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = ADC0_PAGE;
        ADC0CN = 0x00;
        ADC0CF = 0x00;
        EIE2 &= ~0x02;
        SFRPAGE = ucOriginalSFRPage;
    }
    portEXIT_CRITICAL();
}



/*-----------------------------------------------------------*/

static unsigned portBASE_TYPE ComputeADC0Gain( unsigned portBASE_TYPE newGain )
{
    unsigned portBASE_TYPE encodedGain=0;

    while(newGain>1)
    {
        encodedGain++; newGain = newGain>>1;
    }
    return( encodedGain );
}



/*-----------------------------------------------------------*/

portBASE_TYPE bioADC0read( 
                  const unsigned portBASE_TYPE  inputs[],   // sequence of inputs to read
                  const unsigned portBASE_TYPE  gains[],    // sequence of gains for inputs to read
                  unsigned portBASE_TYPE        count,      // how many items in the sequence
                  unsigned portSHORT            values[],   // array to store the results
                  portTickType                  xBlockTime  // maximum time to wait for the results
              )
{
    unsigned portCHAR ucOriginalSFRPage;
    unsigned portBASE_TYPE i;
    portBASE_TYPE success;

    if ( xSemaphoreTake( xADC0MeasuringSemaphore, xBlockTime ) != pdTRUE )
    {
        return pdFALSE;
    }

    if ( ADC0_SEQ_MAX>count ) {
        xADC0count = count;
    } else {
        xADC0count = ADC0_SEQ_MAX;
    }

    for (i=0; i<xADC0count; ++i)
    {
        xADC0inputs[i] = inputs[i];
        xADC0encoded_gains[i]  = ComputeADC0Gain( gains[i] );
    }

    portENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;

        xADC0current= 0;
        SFRPAGE = ADC0_PAGE;
        AMX0SL  = xADC0inputs[0];   // first channel number to read
        ADC0CF  = (ADC0CF & 0xF8) | ( xADC0encoded_gains[0] & 0x07); // set the corresponding input gain
        AD0INT  = 0;                // enable the next conversion detection or interrupt
        AD0BUSY = 1;                // take the channel measurement, start conversion

        SFRPAGE = ucOriginalSFRPage;
    }
    portEXIT_CRITICAL();

    success = xSemaphoreTake( xADC0DataReadySemaphore, xBlockTime );
    if ( pdTRUE==success )
    {
        for (i=0; i<xADC0count; ++i)
        {
            values[i] = xADC0values[i];
        }
    }
    xSemaphoreGive( xADC0MeasuringSemaphore );
    return( success );
}


/*-----------------------------------------------------------*/

// ADC0 end-of-conversion ISR
void ADC0_ISR (void) __interrupt 15
{
    static signed portBASE_TYPE pxHigherPriorityTaskWoken;

    portENTER_CRITICAL();
    {
        unsigned portCHAR effectiveChannel;
        unsigned portCHAR ucOriginalSFRPage;
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = ADC0_PAGE;

        effectiveChannel = xADC0current>>1;
        xADC0values[effectiveChannel] = ADC0;
        AD0INT  = 0;                            // enable the next conversion detection or interrupt

        ++xADC0current;
        effectiveChannel = xADC0current>>1;
        if ( effectiveChannel<xADC0count )
        {
            AMX0SL  = xADC0inputs[effectiveChannel];// first/next channel number to read
            ADC0CF  = (ADC0CF & 0xF8) | ( xADC0encoded_gains[effectiveChannel] & 0x07); // set the corresponding input gain
            AD0BUSY = 1;                        // take the channel measurement, start conversion
        } else {
            xSemaphoreGiveFromISR( xADC0DataReadySemaphore, &pxHigherPriorityTaskWoken );
        }
        SFRPAGE = ucOriginalSFRPage;
    }
    portEXIT_CRITICAL();

    if( pdFALSE != pxHigherPriorityTaskWoken )
    {
        // We should switch context so the ISR returns to a task that has just been awoken
        // Which most likely is the task that called bioADC0read which waited for this semaphore
        taskYIELD();
    }
}
