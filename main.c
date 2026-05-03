#include "f28x_project.h"


#define CC1101_PKTLEN 0x06


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

/* CC1101 */
#define CC1101_IOCFG2        0x00
#define CC1101_SRES          0x30
#define CC1101_READ_SINGLE   0x80
#define CC1101_IOCFG0        0x02
#define CC1101_FIFOTHR       0x03
#define CC1101_SYNC1         0x04
#define CC1101_SYNC0         0x05
#define CC1101_PKTLEN        0x06
#define CC1101_PKTCTRL1      0x07
#define CC1101_PKTCTRL0      0x08
#define CC1101_ADDR          0x09
#define CC1101_CHANNR        0x0A
#define CC1101_FSCTRL1       0x0B
#define CC1101_FSCTRL0       0x0C
#define CC1101_FREQ2         0x0D
#define CC1101_FREQ1         0x0E
#define CC1101_FREQ0         0x0F
#define CC1101_MDMCFG4       0x10
#define CC1101_MDMCFG3       0x11
#define CC1101_MDMCFG2       0x12
#define CC1101_MDMCFG1       0x13
#define CC1101_MDMCFG0       0x14
#define CC1101_DEVIATN       0x15
#define CC1101_MCSM1         0x17
#define CC1101_MCSM0         0x18
#define CC1101_FOCCFG        0x19
#define CC1101_BSCFG         0x1A
#define CC1101_AGCCTRL2      0x1B
#define CC1101_AGCCTRL1      0x1C
#define CC1101_AGCCTRL0      0x1D
#define CC1101_FREND1        0x21
#define CC1101_FREND0        0x22
#define CC1101_FSCAL3        0x23
#define CC1101_FSCAL2        0x24
#define CC1101_FSCAL1        0x25
#define CC1101_FSCAL0        0x26
#define CC1101_TEST2         0x2C
#define CC1101_TEST1         0x2D
#define CC1101_TEST0         0x2E
#define CC1101_PATABLE       0x3E
#define CC1101_TXFIFO        0x3F

#define CC1101_WRITE_BURST   0x40
#define CC1101_SFSTXON       0x31
#define CC1101_SCAL          0x33
#define CC1101_STX           0x35
#define CC1101_SIDLE         0x36
#define CC1101_SFTX          0x3B

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

    //WdRegs.WDCR.all = 0x0068;   // watchdog off

    WdRegs.WDCR.bit.WDCHK = 5; // 101 watch enable/disable etmek icin yazilmali
    WdRegs.WDCR.bit.WDDIS = 1; // watchdog disable

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

static u16 cc1101_strobe_hw(u8 cmd)
{
    volatile u8 rx;

    cc1101_csn_low();
    my_delay_loop(2000);

    if(!spia_xfer8(cmd, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    my_delay_loop(2000);
    cc1101_csn_high();
    my_delay_loop(2000);

    return 1;
}

static u16 cc1101_burst_write_hw(u8 addr, const u8 *data, u16 len)
{
    volatile u8 rx;
    u16 i;

    cc1101_csn_low();
    my_delay_loop(2000);

    if(!spia_xfer8(addr | CC1101_WRITE_BURST, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    for(i = 0; i < len; i++)
    {
        if(!spia_xfer8(data[i], &rx))
        {
            cc1101_csn_high();
            return 0;
        }
    }

    my_delay_loop(2000);
    cc1101_csn_high();
    my_delay_loop(2000);

    return 1;
}

static u16 cc1101_cw_init_433_hw(void)
{
    u8 pa_tbl[2] = {0x60, 0x60};   // maksimuma yakın; istersen 0x60 ile başla
    u16 ok = 1;

    /* 433.920 MHz */
    ok &= cc1101_write_reg_hw(CC1101_FSCTRL1, 0x06);
    ok &= cc1101_write_reg_hw(CC1101_FSCTRL0, 0x00);
    ok &= cc1101_write_reg_hw(CC1101_FREQ2, 0x10);
    ok &= cc1101_write_reg_hw(CC1101_FREQ1, 0xB0);
    ok &=cc1101_write_reg_hw(CC1101_FREQ0, 0xC9);

    /* CW / unmodulated ayarları */
    ok &= cc1101_write_reg_hw(CC1101_PKTLEN,   0xFF);
    ok &= cc1101_write_reg_hw(CC1101_PKTCTRL1, 0x04);
    ok &= cc1101_write_reg_hw(CC1101_PKTCTRL0, 0x12);
    ok &= cc1101_write_reg_hw(CC1101_MDMCFG4,  0xF5);
    ok &= cc1101_write_reg_hw(CC1101_MDMCFG3,  0x83);
    ok &= cc1101_write_reg_hw(CC1101_MDMCFG2,  0x30);
    ok &= cc1101_write_reg_hw(CC1101_MDMCFG1,  0x22);
    ok &= cc1101_write_reg_hw(CC1101_MDMCFG0,  0xF8);
    ok &= cc1101_write_reg_hw(CC1101_DEVIATN,  0x31);

    ok &= cc1101_write_reg_hw(CC1101_MCSM1,    0x30);
    ok &= cc1101_write_reg_hw(CC1101_MCSM0,    0x18);

    ok &= cc1101_write_reg_hw(CC1101_FOCCFG,   0x16);
    ok &= cc1101_write_reg_hw(CC1101_BSCFG,    0x6C);
    ok &= cc1101_write_reg_hw(CC1101_AGCCTRL2, 0x03);
    ok &= cc1101_write_reg_hw(CC1101_AGCCTRL1, 0x40);
    ok &= cc1101_write_reg_hw(CC1101_AGCCTRL0, 0x91);

    ok &= cc1101_write_reg_hw(CC1101_FREND1,   0x56);
    ok &= cc1101_write_reg_hw(CC1101_FREND0,   0x11);

    ok &= cc1101_write_reg_hw(CC1101_FSCAL3,   0xE9);
    ok &= cc1101_write_reg_hw(CC1101_FSCAL2,   0x2A);
    ok &= cc1101_write_reg_hw(CC1101_FSCAL1,   0x00);
    ok &= cc1101_write_reg_hw(CC1101_FSCAL0,   0x1F);

    ok &= cc1101_write_reg_hw(CC1101_TEST2,    0x81);
    ok &= cc1101_write_reg_hw(CC1101_TEST1,    0x35);
    ok &= cc1101_write_reg_hw(CC1101_TEST0,    0x09);

    /* PATABLE[0] ve [1] aynı */
    ok &= cc1101_burst_write_hw(CC1101_PATABLE, pa_tbl, 2);

    return ok;
}

static u16 cc1101_start_cw_hw(void)
{
    u8 dummy = 0xFF;
    u16 ok = 1;

    ok &= cc1101_strobe_hw(CC1101_SIDLE);
    ok &= cc1101_strobe_hw(CC1101_SFTX);
    ok &= cc1101_strobe_hw(CC1101_SCAL);

    ok &= cc1101_burst_write_hw(CC1101_TXFIFO, &dummy, 1);
    ok &= cc1101_strobe_hw(CC1101_STX);

    return ok;
}

static u16 cc1101_stop_cw_hw(void)
{
    u16 ok = 1;
    ok &= cc1101_strobe_hw(CC1101_SIDLE);
    ok &= cc1101_strobe_hw(CC1101_SFTX);
    return ok;
}


volatile u16 cfg_ok = 0;
volatile u16 tx_ok  = 0;

void main(void)
{
    system_min_init();
    spi_cs_gpio34_init();
    spia_gpio_init();
    spia_init();          // CLK_PHASE = 1 olan çalışan sürüm

    rst_ok = cc1101_reset_hw();

    if(rst_ok)
    {
        cfg_ok = cc1101_cw_init_433_hw();
    }

    if(cfg_ok)
    {
        tx_ok = cc1101_start_cw_hw();
    }

    while(1)
    {
    }
}
