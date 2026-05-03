#include "f28x_project.h"


#define CC1101_PKTLEN 0x06


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

/* CC1101 */
#define CC1101_IOCFG2        0x00
#define CC1101_SRES          0x30
#define CC1101_READ_SINGLE   0x80

volatile u16 spi_timeout = 0;
volatile u16 rst_ok = 0;
volatile u16 read_ok = 0;
volatile u8  reg0 = 0x00;

volatile u16 raw1 = 0;
volatile u16 raw2 = 0;
volatile u8  rx1_lo = 0, rx1_hi = 0;
volatile u8  rx2_lo = 0, rx2_hi = 0;


volatile u8 test_wr = 0x12;
volatile u8 test_rd = 0x00;
volatile u16 wr_ok = 0;
volatile u16 rd_ok = 0;
volatile u16 test_ok = 0;

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



static void spia_init(void)
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




static u16 spia_xfer8(u8 tx, volatile u8 *rx)
{
    volatile u32 timeout = 1000000UL;
    volatile u16 rxd;

    rxd = SpiaRegs.SPIRXBUF;
    (void)rxd;

    SpiaRegs.SPITXBUF = ((u16)tx) << 8;

    while((SpiaRegs.SPISTS.bit.INT_FLAG == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if(timeout == 0U)
    {
        spi_timeout = 1;
        return 0;
    }

    spi_timeout = 0;
    rxd = SpiaRegs.SPIRXBUF;
    *rx = (u8)(rxd & 0x00FF);

    return 1;
}


static u16 spia_xfer8_debug(u8 tx, volatile u16 *raw, volatile u8 *rx_lo, volatile u8 *rx_hi)
{
    volatile u32 timeout = 1000000UL;
    volatile u16 rxd;

    rxd = SpiaRegs.SPIRXBUF;
    (void)rxd;

    SpiaRegs.SPITXBUF = ((u16)tx) << 8;

    while((SpiaRegs.SPISTS.bit.INT_FLAG == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if(timeout == 0U)
    {
        spi_timeout = 1;
        return 0;
    }

    spi_timeout = 0;
    rxd = SpiaRegs.SPIRXBUF;

    *raw   = rxd;
    *rx_lo = (u8)(rxd & 0x00FF);
    *rx_hi = (u8)((rxd >> 8) & 0x00FF);

    return 1;
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

static inline void cc1101_csn_low(void)
{
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;
}

static inline void cc1101_csn_high(void)
{
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;
}

static u16 cc1101_reset_hw(void)
{
    u8 rx;

    cc1101_csn_high();
    my_delay_loop(50000);

    cc1101_csn_low();
    my_delay_loop(10000);
    cc1101_csn_high();
    my_delay_loop(50000);

    cc1101_csn_low();
    my_delay_loop(50000);

    if(!spia_xfer8(CC1101_SRES, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    my_delay_loop(200000);   // bunu büyüttük
    cc1101_csn_high();
    my_delay_loop(200000);

    return 1;
}

volatile u8 first_rx  = 0;
volatile u8 second_rx = 0;

/*
static u16 cc1101_read_reg_hw(u8 addr, u8 *value)
{
    cc1101_csn_low();
    my_delay_loop(5000);

    if(!spia_xfer8(addr | CC1101_READ_SINGLE, &first_rx))
    {
        cc1101_csn_high();
        return 0;
    }

    my_delay_loop(2000);

    if(!spia_xfer8(0x00, &second_rx))
    {
        cc1101_csn_high();
        return 0;
    }

    *value = second_rx;

    cc1101_csn_high();
    my_delay_loop(1000);

    return 1;
}

*/

static u16 cc1101_write_reg_hw(u8 addr, u8 value)
{
    volatile u8 rx;

    cc1101_csn_low();
    my_delay_loop(2000);

    if(!spia_xfer8(addr, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    if(!spia_xfer8(value, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    my_delay_loop(2000);
    cc1101_csn_high();
    my_delay_loop(2000);

    return 1;
}

static u16 cc1101_read_reg_hw(u8 addr, volatile u8 *value)
{
    cc1101_csn_low();
    my_delay_loop(5000);

    if(!spia_xfer8_debug(addr | CC1101_READ_SINGLE, &raw1, &rx1_lo, &rx1_hi))
    {
        cc1101_csn_high();
        return 0;
    }

    my_delay_loop(2000);

    if(!spia_xfer8_debug(0x00, &raw2, &rx2_lo, &rx2_hi))
    {
        cc1101_csn_high();
        return 0;
    }

    *value = rx2_lo;   // şimdilik böyle bırak

    cc1101_csn_high();
    my_delay_loop(5000);

    return 1;
}
void main(void)
{
    system_min_init();
    spi_cs_gpio34_init();
    spia_gpio_init();
    spia_init();

    rst_ok = cc1101_reset_hw();

    read_ok = cc1101_read_reg_hw(CC1101_IOCFG2, &reg0);

    wr_ok = cc1101_write_reg_hw(CC1101_PKTLEN, test_wr);
    rd_ok = cc1101_read_reg_hw(CC1101_PKTLEN, &test_rd);

    if((reg0 == 0x29) && (wr_ok == 1) && (rd_ok == 1) && (test_rd == test_wr))
    {
        test_ok = 1;
    }
    else
    {
        test_ok = 0;
    }
}
