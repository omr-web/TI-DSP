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

void main(void)
{
    system_min_init();
    spia_init_internal_loopback();

    while(1)
    {
        rx_data = spia_xfer16_loopback(tx_data);

        // Beklenen: rx_data == tx_data
        if(rx_data != tx_data)
        {
            asm(" ESTOP0");
        }

        tx_data++;
        my_delay_loop(200000);
    }
}
