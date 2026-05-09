#include "f28x_project.h"
#include "system.h"
#include "cc1101.h"
#include <stdint.h>




volatile uint16_t cfg_ok = 0;
volatile uint16_t tx_ok  = 0;
volatile uint16_t rst_ok  = 0;


static const uint16_t tx_payload[] =
{
    0x55, 0xAA, 0x55, 0xAA,
    'T', 'I', '-', 'C', 'C', '1', '1', '0', '1'
};


volatile uint16_t dbg_iocfg0   = 0;
volatile uint16_t dbg_pktctrl0 = 0;
volatile uint16_t dbg_pktlen   = 0;
volatile uint16_t dbg_mcsm1    = 0;

volatile uint16_t dbg_marcstate = 0;
volatile uint16_t dbg_txbytes   = 0;
volatile uint16_t dbg_timeout_stage = 0;


void main(void)
{
    system_min_init();
    spi_cs_gpio34_init();
    spia_gpio_init();
    spia_init();          // CLK_PHASE = 1 olan çalışan sürüm
    cc1101_gdo0_init();
    rst_ok = cc1101_reset_hw();

    if(rst_ok)
    {
        cfg_ok = cc1101_packet_init_hw_crc_gdo0();

        cc1101_read_reg_hw(CC1101_IOCFG0, &dbg_iocfg0);
        cc1101_read_reg_hw(CC1101_PKTCTRL0, &dbg_pktctrl0);
        cc1101_read_reg_hw(CC1101_PKTLEN, &dbg_pktlen);
        cc1101_read_reg_hw(CC1101_MCSM1, &dbg_mcsm1);
    }

    //gdo0'clock yaptım
    //cc1101_write_reg_hw(CC1101_IOCFG0, 0x3F);
    while(1)
    {

        if(cfg_ok)
        {
            tx_ok = cc1101_send_packet_hw_gdo0(tx_payload, sizeof(tx_payload));
        }

        system_delay_loop(800000);


    }
}
