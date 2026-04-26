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


#define CC1101_IOCFG2       0x00
#define CC1101_IOCFG1       0x01
#define CC1101_IOCFG0       0x02
#define CC1101_FIFOTHR      0x03
#define CC1101_SYNC1        0x04
#define CC1101_SYNC0        0x05
#define CC1101_PKTLEN       0x06
#define CC1101_PKTCTRL1     0x07
#define CC1101_PKTCTRL0     0x08
#define CC1101_ADDR         0x09
#define CC1101_CHANNR       0x0A
#define CC1101_FSCTRL1      0x0B
#define CC1101_FSCTRL0      0x0C
#define CC1101_FREQ2        0x0D
#define CC1101_FREQ1        0x0E
#define CC1101_FREQ0        0x0F
#define CC1101_MDMCFG4      0x10
#define CC1101_MDMCFG3      0x11
#define CC1101_MDMCFG2      0x12
#define CC1101_MDMCFG1      0x13
#define CC1101_MDMCFG0      0x14
#define CC1101_DEVIATN      0x15
#define CC1101_MCSM1        0x17
#define CC1101_MCSM0        0x18
#define CC1101_FOCCFG       0x19
#define CC1101_BSCFG        0x1A
#define CC1101_AGCCTRL2     0x1B
#define CC1101_AGCCTRL1     0x1C
#define CC1101_AGCCTRL0     0x1D
#define CC1101_FREND1       0x21
#define CC1101_FREND0       0x22
#define CC1101_FSCAL3       0x23
#define CC1101_FSCAL2       0x24
#define CC1101_FSCAL1       0x25
#define CC1101_FSCAL0       0x26
#define CC1101_TEST2        0x2C
#define CC1101_TEST1        0x2D
#define CC1101_TEST0        0x2E

#define CC1101_PATABLE      0x3E
#define CC1101_TXFIFO       0x3F

#define CC1101_SFSTXON      0x31
#define CC1101_SCAL         0x33
#define CC1101_STX          0x35
#define CC1101_SIDLE        0x36
#define CC1101_SFTX         0x3B

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

static void cc1101_strobe_sw(u8 cmd)
{
    cc_csn_low();

    if(!cc1101_wait_ready())
    {
        cc_csn_high();
        return;
    }

    swspi_xfer8(cmd);
    cc_csn_high();
}

static u16 cc1101_burst_write_sw(u8 addr, const u8 *data, u16 len)
{
    u16 i;

    cc_csn_low();

    if(!cc1101_wait_ready())
    {
        cc_csn_high();
        return 0;
    }

    swspi_xfer8(addr | CC1101_WRITE_BURST);

    for(i = 0; i < len; i++)
    {
        swspi_xfer8(data[i]);
    }

    cc_csn_high();
    return 1;
}

static void cc1101_basic_tx_init_sw(void)
{
    cc1101_write_reg_sw(CC1101_IOCFG2,   0x29);
    cc1101_write_reg_sw(CC1101_IOCFG1,   0x2E);
    cc1101_write_reg_sw(CC1101_IOCFG0,   0x2E);

    cc1101_write_reg_sw(CC1101_FIFOTHR,  0x47);

    cc1101_write_reg_sw(CC1101_SYNC1,    0xD3);
    cc1101_write_reg_sw(CC1101_SYNC0,    0x91);

    cc1101_write_reg_sw(CC1101_PKTLEN,   0x10);
    cc1101_write_reg_sw(CC1101_PKTCTRL1, 0x00);
    cc1101_write_reg_sw(CC1101_PKTCTRL0, 0x00);

    cc1101_write_reg_sw(CC1101_ADDR,     0x00);
    cc1101_write_reg_sw(CC1101_CHANNR,   0x00);

    cc1101_write_reg_sw(CC1101_FSCTRL1,  0x06);
    cc1101_write_reg_sw(CC1101_FSCTRL0,  0x00);

    // 433.92 MHz
    cc1101_write_reg_sw(CC1101_FREQ2,    0x10);
    cc1101_write_reg_sw(CC1101_FREQ1,    0xB0);
    cc1101_write_reg_sw(CC1101_FREQ0,    0x71);

    // GFSK, yaklaşık 38.4kbps
    cc1101_write_reg_sw(CC1101_MDMCFG4,  0xCA);
    cc1101_write_reg_sw(CC1101_MDMCFG3,  0x83);
    cc1101_write_reg_sw(CC1101_MDMCFG2,  0x13);
    cc1101_write_reg_sw(CC1101_MDMCFG1,  0x22);
    cc1101_write_reg_sw(CC1101_MDMCFG0,  0xF8);

    cc1101_write_reg_sw(CC1101_DEVIATN,  0x35);

    cc1101_write_reg_sw(CC1101_MCSM1,    0x00);
    cc1101_write_reg_sw(CC1101_MCSM0,    0x18);

    cc1101_write_reg_sw(CC1101_FOCCFG,   0x16);
    cc1101_write_reg_sw(CC1101_BSCFG,    0x6C);
    cc1101_write_reg_sw(CC1101_AGCCTRL2, 0x43);
    cc1101_write_reg_sw(CC1101_AGCCTRL1, 0x40);
    cc1101_write_reg_sw(CC1101_AGCCTRL0, 0x91);

    cc1101_write_reg_sw(CC1101_FREND1,   0x56);
    cc1101_write_reg_sw(CC1101_FREND0,   0x10);

    cc1101_write_reg_sw(CC1101_FSCAL3,   0xE9);
    cc1101_write_reg_sw(CC1101_FSCAL2,   0x2A);
    cc1101_write_reg_sw(CC1101_FSCAL1,   0x00);
    cc1101_write_reg_sw(CC1101_FSCAL0,   0x1F);

    cc1101_write_reg_sw(CC1101_TEST2,    0x81);
    cc1101_write_reg_sw(CC1101_TEST1,    0x35);
    cc1101_write_reg_sw(CC1101_TEST0,    0x09);

    // TX güç tablosu
    cc1101_write_reg_sw(CC1101_PATABLE,  0x60);

    cc1101_strobe_sw(CC1101_SIDLE);
    cc1101_strobe_sw(CC1101_SFTX);
    cc1101_strobe_sw(CC1101_SCAL);
}

static void cc1101_send_packet_sw(const u8 *data, u8 len)
{
    if(len == 0 || len > 64)
        return;

    cc1101_strobe_sw(CC1101_SIDLE);
    cc1101_strobe_sw(CC1101_SFTX);

    cc1101_write_reg_sw(CC1101_PKTLEN, len);
    cc1101_burst_write_sw(CC1101_TXFIFO, data, len);

    cc1101_strobe_sw(CC1101_STX);

    // paket yayınlansın diye biraz bekle
    my_delay_loop(300000);
}

static const u8 tx_payload[] =
{
    0x55, 0xAA, 0x55, 0xAA,
    0xF0, 0x0F, 0xCC, 0x33,
    'T', 'I', '-', 'C', 'C', '1', '1', '0', '1'
};

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
        cc1101_basic_tx_init_sw();

        while(1)
        {
            cc1101_send_packet_sw(tx_payload, sizeof(tx_payload));
            my_delay_loop(1500000);

            volatile u8 f2, f1, f0;

            f2 = cc1101_read_reg_sw(CC1101_FREQ2);
            f1 = cc1101_read_reg_sw(CC1101_FREQ1);
            f0 = cc1101_read_reg_sw(CC1101_FREQ0);
        }
    }

    while(1)
    {
    }
}
