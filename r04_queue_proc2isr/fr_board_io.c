#include "fr_board_io.h"
#include "task.h"

void bioSetAB4_LED1(portBASE_TYPE newval)
{
    unsigned portCHAR ucOriginalSFRPage;
    taskENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = CONFIG_PAGE;
        AB4_LED1=newval;
        SFRPAGE = ucOriginalSFRPage;
    }
    taskEXIT_CRITICAL();
}

void bioSetAB4_LED2(portBASE_TYPE newval)
{
    unsigned portCHAR ucOriginalSFRPage;
    taskENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = CONFIG_PAGE;
        AB4_LED2=newval;
        SFRPAGE = ucOriginalSFRPage;    // must be restored before any task switching might take place
    }
    taskEXIT_CRITICAL();
}

portBASE_TYPE bioGetAB4_SW1()
{
    unsigned portCHAR   ucOriginalSFRPage;
    portBASE_TYPE       value;
    taskENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = CONFIG_PAGE;
        value=!AB4_SW1;
        SFRPAGE = ucOriginalSFRPage;
    }
    taskEXIT_CRITICAL();
    return(value);
}

portBASE_TYPE bioGetAB4_SW2()
{
    unsigned portCHAR   ucOriginalSFRPage;
    portBASE_TYPE       value;
    taskENTER_CRITICAL();
    {
        ucOriginalSFRPage = SFRPAGE;
        SFRPAGE = CONFIG_PAGE;
        value=!AB4_SW2;
        SFRPAGE = ucOriginalSFRPage;
    }
    taskEXIT_CRITICAL();
    return(value);
}

