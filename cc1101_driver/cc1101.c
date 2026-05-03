/*
 * cc1101.c
 *
 *  Created on: May 3, 2026
 *      Author: omc_2
 */

#include "cc1101.h"
#include "system.h"
#include "f28x_project.h"


uint16_t cc1101_reset_hw(void)
{
    uint16_t rx;

    cc1101_csn_high();
    system_delay_loop(50000);

    cc1101_csn_low();
    system_delay_loop(10000);
    cc1101_csn_high();
    system_delay_loop(50000);

    cc1101_csn_low();
    system_delay_loop(50000);

    if(!spia_xfer8(CC1101_SRES, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    system_delay_loop(200000);   // bunu büyüttük
    cc1101_csn_high();
    system_delay_loop(200000);

    return 1;
}

inline void cc1101_csn_low(void)
{
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;
}

inline void cc1101_csn_high(void)
{
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;
}


uint16_t cc1101_write_reg_hw(uint16_t addr, uint16_t value)
{
    volatile uint16_t rx;

    cc1101_csn_low();
    system_delay_loop(2000);

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

    system_delay_loop(2000);
    cc1101_csn_high();
    system_delay_loop(2000);

    return 1;
}

volatile uint16_t raw1 = 0;
volatile uint16_t raw2 = 0;
volatile uint16_t  rx1_lo = 0, rx1_hi = 0;
volatile uint16_t  rx2_lo = 0, rx2_hi = 0;

uint16_t cc1101_read_reg_hw(uint16_t addr, volatile uint16_t *value)
{
    cc1101_csn_low();
    system_delay_loop(5000);

    if(!spia_xfer8_debug(addr | CC1101_READ_SINGLE, &raw1, &rx1_lo, &rx1_hi))
    {
        cc1101_csn_high();
        return 0;
    }

    system_delay_loop(2000);

    if(!spia_xfer8_debug(0x00, &raw2, &rx2_lo, &rx2_hi))
    {
        cc1101_csn_high();
        return 0;
    }

    *value = rx2_lo;   // şimdilik böyle bırak

    cc1101_csn_high();
    system_delay_loop(5000);

    return 1;
}

uint16_t cc1101_strobe_hw(uint16_t cmd)
{
    volatile uint16_t rx;

    cc1101_csn_low();
    system_delay_loop(2000);

    if(!spia_xfer8(cmd, &rx))
    {
        cc1101_csn_high();
        return 0;
    }

    system_delay_loop(2000);
    cc1101_csn_high();
    system_delay_loop(2000);

    return 1;
}

uint16_t cc1101_burst_write_hw(uint16_t addr, const uint16_t *data, uint16_t len)
{
    volatile uint16_t rx;
    uint16_t i;

    cc1101_csn_low();
    system_delay_loop(2000);

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

    system_delay_loop(2000);
    cc1101_csn_high();
    system_delay_loop(2000);

    return 1;
}

uint16_t cc1101_cw_init_433_hw(void)
{
    uint16_t pa_tbl[2] = {0x60, 0x60};   // maksimuma yakın; istersen 0x60 ile başla
    uint16_t ok = 1;

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

uint16_t cc1101_start_cw_hw(void)
{
    uint16_t dummy = 0xFF;
    uint16_t ok = 1;

    ok &= cc1101_strobe_hw(CC1101_SIDLE);
    ok &= cc1101_strobe_hw(CC1101_SFTX);
    ok &= cc1101_strobe_hw(CC1101_SCAL);

    ok &= cc1101_burst_write_hw(CC1101_TXFIFO, &dummy, 1);
    ok &= cc1101_strobe_hw(CC1101_STX);

    return ok;
}

uint16_t cc1101_stop_cw_hw(void)
{
    uint16_t ok = 1;
    ok &= cc1101_strobe_hw(CC1101_SIDLE);
    ok &= cc1101_strobe_hw(CC1101_SFTX);
    return ok;
}




/*
static uint16_t cc1101_read_reg_hw(uint16_t addr, uint16_t *value)
{
    cc1101_csn_low();
    system_delay_loop(5000);

    if(!spia_xfer8(addr | CC1101_READ_SINGLE, &first_rx))
    {
        cc1101_csn_high();
        return 0;
    }

    system_delay_loop(2000);

    if(!spia_xfer8(0x00, &second_rx))
    {
        cc1101_csn_high();
        return 0;
    }

    *value = second_rx;

    cc1101_csn_high();
    system_delay_loop(1000);

    return 1;
}

*/
