/*
 * system.c
 *
 *  Created on: May 3, 2026
 *      Author: omc_2
 */

#include "system.h"
#include "f28x_project.h"

void system_min_init(void)
{
    EALLOW;

    //WdRegs.WDCR.all = 0x0068;   // watchdog off

    WdRegs.WDCR.bit.WDCHK = 5; // 101 watch enable/disable etmek icin yazilmali
    WdRegs.WDCR.bit.WDDIS = 1; // watchdog disable

    EDIS;
}

void system_delay_loop(volatile uint32_t count)
{
    while(count--)
    {
        asm(" NOP");
    }
}

void spia_init(void)
{
    EALLOW;

    // SPIA peripheral clock
    // Header'ında isim SPIA ise onu kullan
    CpuSysRegs.PCLKCR8.bit.SPI_A = 1;

    EDIS;

    // FIFO tamamen kapalı
    //SpiaRegs.SPIFFTX.bit.SPIFFENA    = 0;
  //    SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;
   //   SpiaRegs.SPIFFCT.all             = 0x0000;

    // Reset altında konfigüre et
    // 0x001F = SPISWRESET=0, CLKPOL=0 SPICHAR=15 (16-bit)
    SpiaRegs.SPICCR.all = 0x0000;
    SpiaRegs.SPICCR.bit.SPISWRESET  = 0;
    SpiaRegs.SPICCR.bit.CLKPOLARITY = 0;
    SpiaRegs.SPICCR.bit.SPICHAR     = 7;
    SpiaRegs.SPICCR.bit.SPILBK      = 0;

    // 0x0007 = SPIINTENA=1, TALK=1, MASTER=1, CLK_PHASE=0
    SpiaRegs.SPICTL.all = 0x0000;
    //SpiaRegs.SPICTL.all = 0x0007;
    SpiaRegs.SPICTL.all = 0x000F;
    // Yavaş başla
    SpiaRegs.SPIBRR.all = 0x0063;

    // Debug'da durunca SPI bozulmasın
    SpiaRegs.SPIPRI.bit.FREE = 1;

    // Resetten çıkar
    // 0x009F = SPISWRESET=1, CLKPOL=0, SPILBK=0, SPICHAR=15
    SpiaRegs.SPICCR.bit.SPISWRESET = 1;
}

uint16_t spia_xfer8(uint16_t tx, volatile uint16_t *rx)
{
    volatile uint32_t timeout = 1000000UL;
    volatile uint16_t rxd;

    rxd = SpiaRegs.SPIRXBUF;
    (void)rxd;

    SpiaRegs.SPITXBUF = ((uint16_t)tx) << 8;

    while((SpiaRegs.SPISTS.bit.INT_FLAG == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if(timeout == 0U)
    {
        return 0;
    }

    rxd = SpiaRegs.SPIRXBUF;
    *rx = (uint16_t)(rxd & 0x00FF);

    return 1;
}

uint16_t spia_xfer8_debug(uint16_t tx, volatile uint16_t *raw, volatile uint16_t *rx_lo, volatile uint16_t *rx_hi)
{
    volatile uint32_t timeout = 1000000UL;
    volatile uint16_t rxd;

    rxd = SpiaRegs.SPIRXBUF;
    (void)rxd;

    SpiaRegs.SPITXBUF = ((uint16_t)tx) << 8;

    while((SpiaRegs.SPISTS.bit.INT_FLAG == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if(timeout == 0U)
    {
        return 0;
    }

    rxd = SpiaRegs.SPIRXBUF;

    *raw   = rxd;
    *rx_lo = (uint16_t)(rxd & 0x00FF);
    *rx_hi = (uint16_t)((rxd >> 8) & 0x00FF);

    return 1;
}

void spia_gpio_init(void)
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

void spi_cs_gpio34_init(void)
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



