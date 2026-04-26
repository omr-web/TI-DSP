#include "f28x_project.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

/* =========================
   CC1101 sabitleri
   ========================= */
#define CC1101_PKTLEN       0x06
#define CC1101_SRES         0x30

#define CC1101_READ_SINGLE  0x80
#define CC1101_WRITE_BURST  0x40

/* =========================
   Debug değişkenleri
   ========================= */
volatile u8  test_wr = 0x12;
volatile u8  test_rd = 0x00;
volatile u16 test_ok = 0;
volatile u16 ready_ok = 0;

/* =========================
   Delay
   ========================= */
static void my_delay_loop(volatile u32 count)
{
    while(count--)
    {
        asm(" NOP");
    }
}

/* =========================
   Minimum system init
   ========================= */
static void system_min_init(void)
{
    EALLOW;
    WdRegs.WDCR.all = 0x0068;   // watchdog off
    EDIS;
}

/* =========================
   GPIO init
   GPIO16 = MOSI (SI)
   GPIO17 = MISO (SO/GDO1)
   GPIO18 = SCLK
   GPIO34 = CSN
   ========================= */
static void cc1101_gpio_init_swspi(void)
{
    EALLOW;

    /* GPIO16 -> output (MOSI) */
    GpioCtrlRegs.GPAGMUX2.bit.GPIO16 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO16  = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO16   = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO16   = 1;
    GpioDataRegs.GPACLEAR.bit.GPIO16 = 1;

    /* GPIO17 -> input (MISO / SO / GDO1) */
    GpioCtrlRegs.GPAGMUX2.bit.GPIO17 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO17  = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO17   = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO17   = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;

    /* GPIO18 -> output (SCLK) */
    GpioCtrlRegs.GPAGMUX2.bit.GPIO18 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18  = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO18   = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO18   = 1;
    GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;

    /* GPIO34 -> output (CSN) */
    GpioCtrlRegs.GPBGMUX1.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBMUX1.bit.GPIO34  = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO34   = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO34   = 1;
    GpioDataRegs.GPBSET.bit.GPIO34   = 1;   // idle high

    EDIS;
}

/* =========================
   Pin yardımcıları
   ========================= */
static inline void cc_csn_low(void)
{
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;
}

static inline void cc_csn_high(void)
{
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;
}

static inline void cc_sclk_low(void)
{
    GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;
}

static inline void cc_sclk_high(void)
{
    GpioDataRegs.GPASET.bit.GPIO18 = 1;
}

static inline void cc_mosi_low(void)
{
    GpioDataRegs.GPACLEAR.bit.GPIO16 = 1;
}

static inline void cc_mosi_high(void)
{
    GpioDataRegs.GPASET.bit.GPIO16 = 1;
}

static inline u16 cc_miso_read(void)
{
    return GpioDataRegs.GPADAT.bit.GPIO17;
}

/* =========================
   Kısa SPI yarım periyot gecikmesi
   Çok hızlı olursa bunu büyüt
   ========================= */
static inline void spi_half_delay(void)
{
    my_delay_loop(20);
}

/* =========================
   CC1101 ready bekle
   SO/GDO1 low olmalı
   ========================= */
static u16 cc1101_wait_ready(void)
{
    volatile u32 timeout = 200000UL;

    while((cc_miso_read() == 1U) && (timeout > 0U))
    {
        timeout--;
    }

    if(timeout == 0U)
    {
        return 0;
    }

    return 1;
}

/* =========================
   Software SPI xfer (Mode 0 mantığı)
   MOSI data -> rising edge'te örnekle
   ========================= */
static u8 swspi_xfer8(u8 tx)
{
    u8 rx = 0;
    u16 i;

    for(i = 0; i < 8; i++)
    {
        /* MSB first */
        if(tx & 0x80)
            cc_mosi_high();
        else
            cc_mosi_low();

        spi_half_delay();

        cc_sclk_high();
        spi_half_delay();

        rx <<= 1;
        if(cc_miso_read())
            rx |= 0x01;

        cc_sclk_low();
        spi_half_delay();

        tx <<= 1;
    }

    return rx;
}

/* =========================
   CC1101 reset
   ========================= */
static u16 cc1101_reset_sw(void)
{
    cc_csn_high();
    cc_sclk_low();
    cc_mosi_low();
    my_delay_loop(50000);

    cc_csn_low();
    my_delay_loop(10000);
    cc_csn_high();
    my_delay_loop(50000);

    cc_csn_low();

    if(!cc1101_wait_ready())
    {
        cc_csn_high();
        return 0;
    }

    swspi_xfer8(CC1101_SRES);

    if(!cc1101_wait_ready())
    {
        cc_csn_high();
        return 0;
    }

    cc_csn_high();
    my_delay_loop(50000);

    return 1;
}

/* =========================
   Register write
   ========================= */
static u16 cc1101_write_reg_sw(u8 addr, u8 value)
{
    cc_csn_low();

    if(!cc1101_wait_ready())
    {
        cc_csn_high();
        return 0;
    }

    swspi_xfer8(addr);
    swspi_xfer8(value);

    cc_csn_high();
    return 1;
}

/* =========================
   Register read
   ========================= */
static u8 cc1101_read_reg_sw(u8 addr)
{
    u8 value;

    cc_csn_low();

    if(!cc1101_wait_ready())
    {
        cc_csn_high();
        return 0xFF;
    }

    swspi_xfer8(addr | CC1101_READ_SINGLE);
    value = swspi_xfer8(0x00);

    cc_csn_high();

    return value;
}

/* =========================
   main
   ========================= */
void main(void)
{
    system_min_init();
    cc1101_gpio_init_swspi();

    ready_ok = cc1101_reset_sw();

    if(ready_ok == 1)
    {
        cc1101_write_reg_sw(CC1101_PKTLEN, test_wr);
        test_rd = cc1101_read_reg_sw(CC1101_PKTLEN);

        if(test_rd == test_wr)
            test_ok = 1;
        else
            test_ok = 0;
    }
    else
    {
        test_ok = 0;
    }

    while(1)
    {
    }
}
