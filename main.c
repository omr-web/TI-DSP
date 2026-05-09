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

void main(void)
{
    system_min_init();
    spi_cs_gpio34_init();
    spia_gpio_init();
    spia_init();          // CLK_PHASE = 1 olan çalışan sürüm

    rst_ok = cc1101_reset_hw();

    if(rst_ok)
    {
        cfg_ok = cc1101_packet_init_hw();
    }

    while(1)
    {
        if(cfg_ok)
        {
            tx_ok = cc1101_send_packet_hw(tx_payload, sizeof(tx_payload));
        }

        system_delay_loop(800000);
    }
}
