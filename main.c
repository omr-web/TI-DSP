#include "f28x_project.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

volatile u16 tx_data = 0xA5A5;
volatile u16 rx_data = 0x0000;
volatile u16 spi_timeout = 0;

static void my_delay_loop(volatile u32 count)
{
    while(count--)
    {
        asm(" NOP");
    }
}

static void system_min_init(void)
{
    EALLOW;
    WdRegs.WDCR.all = 0x0068;   // watchdog off
    EDIS;
}

static void spia_init_internal_loopback(void)
{
    EALLOW;

    // SPIA peripheral clock
    // Header'ında isim SPIA ise onu kullan
    CpuSysRegs.PCLKCR8.bit.SPI_A = 1;

    EDIS;

    // FIFO tamamen kapalı
    SpiaRegs.SPIFFTX.bit.SPIFFENA    = 0;
    SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;
    SpiaRegs.SPIFFCT.all             = 0x0000;

    // Reset altında konfigüre et
    // 0x001F = SPISWRESET=0, CLKPOL=0, SPILBK=1, SPICHAR=15 (16-bit)
    SpiaRegs.SPICCR.all = 0x001F;

    //loopback i kapadim.
    SpiaRegs.SPICCR.bit.SPILBK = 0;
    // 0x0007 = SPIINTENA=1, TALK=1, MASTER=1, CLK_PHASE=0
    SpiaRegs.SPICTL.all = 0x0007;

    // Yavaş başla
    SpiaRegs.SPIBRR.all = 0x0063;

    // Debug'da durunca SPI bozulmasın
    SpiaRegs.SPIPRI.bit.FREE = 1;

    // Resetten çıkar
    // 0x009F = SPISWRESET=1, CLKPOL=0, SPILBK=1, SPICHAR=15
    SpiaRegs.SPICCR.all = 0x009F;
}

static u16 spia_xfer16_loopback(u16 tx)
{
    volatile u32 timeout = 1000000UL;

    SpiaRegs.SPITXBUF = tx;

    while((SpiaRegs.SPISTS.bit.INT_FLAG != 1) && (timeout > 0))
    {
        timeout--;
    }

    if(timeout == 0)
    {
        spi_timeout = 1;
        return 0xFFFF;
    }

    spi_timeout = 0;
    return SpiaRegs.SPIRXBUF;
}

static void spia_gpio_init(void)
{
    EALLOW;

    /* GPIO16 -> SPIA SIMO */
    GpioCtrlRegs.GPAGMUX2.bit.GPIO16 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO16  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO16   = 0;   // pull-up enable
    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3;   // async

    /* GPIO17 -> SPIA SOMI */
    GpioCtrlRegs.GPAGMUX2.bit.GPIO17 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO17  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO17   = 0;   // pull-up enable
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;   // async

    /* GPIO18 -> SPIA CLK */
    GpioCtrlRegs.GPAGMUX2.bit.GPIO18 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18  = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO18   = 0;   // pull-up enable
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3;   // async

    EDIS;
}

static void spi_cs_gpio34_init(void)
{
    EALLOW;

    GpioCtrlRegs.GPBGMUX1.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBMUX1.bit.GPIO34  = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO34   = 0;
    GpioCtrlRegs.GPBQSEL1.bit.GPIO34 = 3;
    GpioCtrlRegs.GPBDIR.bit.GPIO34   = 1;

    GpioDataRegs.GPBSET.bit.GPIO34   = 1;   // CS idle high

    EDIS;
}


void main(void)
{
    system_min_init();
    spi_cs_gpio34_init();
    spia_gpio_init();
    spia_init_internal_loopback();

    while(1)
    {
        SpiaRegs.SPITXBUF = 0xAA00;

        while(SpiaRegs.SPISTS.bit.INT_FLAG == 0)
        {
        }

        my_delay_loop(200000);
    }
}
