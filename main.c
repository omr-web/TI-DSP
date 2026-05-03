#include "f28x_project.h"
#include "system.h"
#include "cc1101.h"
#include <stdint.h>




volatile uint16_t cfg_ok = 0;
volatile uint16_t tx_ok  = 0;
volatile uint16_t rst_ok  = 0;


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
