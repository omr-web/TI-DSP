/*
 * cc1101.h
 *
 *  Created on: May 3, 2026
 *      Author: omc_2
 */

#ifndef CC1101_DRIVER_CC1101_H_
#define CC1101_DRIVER_CC1101_H_

#include <stdint.h>

/* CC1101 */
#define CC1101_PKTLEN 0x06
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

inline void cc1101_csn_low(void);
inline void cc1101_csn_high(void);
uint16_t cc1101_reset_hw(void);
uint16_t cc1101_stop_cw_hw(void);
uint16_t cc1101_start_cw_hw(void);
uint16_t cc1101_cw_init_433_hw(void);
uint16_t cc1101_burst_write_hw(uint16_t addr, const uint16_t *data, uint16_t len);
uint16_t cc1101_strobe_hw(uint16_t cmd);
uint16_t cc1101_read_reg_hw(uint16_t addr, volatile uint16_t *value);
uint16_t cc1101_write_reg_hw(uint16_t addr, uint16_t value);
#endif /* CC1101_DRIVER_CC1101_H_ */
