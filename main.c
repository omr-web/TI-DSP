
#include "f28x_project.h"

static void my_delay_loop(volatile Uint32 count)
{
    while(count--)
    {
        asm(" NOP");
    }
}

static void gpio34_init(void)
{
    EALLOW;

    // GPIO34 -> normal GPIO
    GpioCtrlRegs.GPBGMUX1.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBMUX1.bit.GPIO34  = 0;

    // pull-up enable
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 0;

    // qualification
    GpioCtrlRegs.GPBQSEL1.bit.GPIO34 = 0;

    // başlangıçta LOW
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;

    // output
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;

    EDIS;
}

void main(void)
{

    //asm(" ESTOP0");

   // InitSysCtrl();


    EALLOW;
    WdRegs.WDCR.all = 0x0068;   // watchdog kapat
    EDIS;

    gpio34_init();


    while(1)
    {
        GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
        my_delay_loop(500000);
    }
}
