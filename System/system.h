/*
 * system.h
 *
 *  Created on: May 3, 2026
 *      Author: omc_2
 */

#ifndef SYSTEM_SYSTEM_H_
#define SYSTEM_SYSTEM_H_

#include <stdint.h>

void system_min_init(void);
void system_delay_loop(volatile uint32_t count);
void spia_init(void);
uint16_t spia_xfer8(uint16_t tx, volatile uint16_t *rx);
uint16_t spia_xfer8_debug(uint16_t tx, volatile uint16_t *raw, volatile uint16_t *rx_lo, volatile uint16_t *rx_hi);
void spia_gpio_init(void);
void spi_cs_gpio34_init(void);

#endif /* SYSTEM_SYSTEM_H_ */
