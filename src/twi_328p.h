#ifndef TWI_328P_H
#define TWI_328P_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ATmega328P TWI (I2C) blocking master driver.
 * - Uses hardware TWI registers (TWBR/TWSR/TWCR/TWDR).
 * - Provides low-level primitives + simple register helpers.
 *
 * Addressing:
 * - Pass 7-bit slave address (0..127) to API functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================
 * Configuration / timeouts
 * ========================= */

/* Timeout loop counter for waiting TWINT. Adjust for your CPU speed and bus conditions. */
#ifndef TWI_328P_TIMEOUT
#define TWI_328P_TIMEOUT  (uint16_t)60000
#endif

/* =========================
 * Error / status codes
 * ========================= */

typedef enum
{
    TWI_OK = 0,

    /* General / software errors */
    TWI_ERR_TIMEOUT,
    TWI_ERR_INVALID_ARG,

    /* Bus / arbitration */
    TWI_ERR_ARB_LOST,
    TWI_ERR_BUS_ERROR,

    /* Slave responses */
    TWI_ERR_ADDR_NACK,
    TWI_ERR_DATA_NACK,

    /* Unexpected TWI state */
    TWI_ERR_UNEXPECTED_STATUS
} twi_result_t;

/* =========================
 * Public API
 * ========================= */

/*
 * Initialize TWI hardware in master mode.
 *
 * cpu_hz: CPU clock frequency in Hz (e.g. 16000000UL).
 * scl_hz: desired SCL frequency in Hz (e.g. 100000UL or 400000UL).
 *
 * Notes:
 * - This function sets prescaler to 1 by default.
 * - TWBR is computed for prescaler=1. If TWBR underflows/overflows, you should
 *   choose a lower scl_hz or adjust prescaler logic.
 */
twi_result_t twi328p_init(uint32_t cpu_hz, uint32_t scl_hz);

/*
 * Deinitialize TWI hardware (disable TWI).
 */
void twi_deinit(void);

/*
 * Send START condition (or repeated START).
 */
twi_result_t twi_start(void);

/*
 * Send STOP condition.
 * Note: STOP does not set TWINT; hardware releases the bus asynchronously.
 */
void twi328p_stop(void);

/*
 * Send SLA+W or SLA+R.
 * addr7: 7-bit slave address.
 * read:  false => SLA+W, true => SLA+R
 */
twi_result_t twi_send_address(uint8_t addr7, bool read);

/*
 * Write one data byte.
 */
twi_result_t twi_write_byte(uint8_t data);

/*
 * Read one data byte and return it via *data.
 * ack: true  => send ACK (request more bytes)
 *      false => send NACK (last byte)
 */
twi_result_t twi_read_byte(uint8_t *data, bool ack);

/* =========================
 * High-level helpers
 * ========================= */

/*
 * Write single 8-bit register:
 *   START -> SLA+W -> reg -> data -> STOP
 */
twi_result_t twi_write_reg(uint8_t addr7, uint8_t reg, uint8_t data);

/*
 * Read single 8-bit register:
 *   START -> SLA+W -> reg -> RESTART -> SLA+R -> read NACK -> STOP
 */
twi_result_t twi_read_reg(uint8_t addr7, uint8_t reg, uint8_t *data);

/*
 * Write N bytes starting at register address:
 *   START -> SLA+W -> reg -> data[0..len-1] -> STOP
 */
twi_result_t twi_write_regs(uint8_t addr7, uint8_t start_reg, const uint8_t *buf, uint16_t len);

/*
 * Read N bytes starting at register address:
 *   START -> SLA+W -> reg -> RESTART -> SLA+R -> read ACK...ACK, last NACK -> STOP
 */
twi_result_t twi_read_regs(uint8_t addr7, uint8_t start_reg, uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* TWI_328P_H */
////
//exaple usage:
// #include "twi_328p.h"
// ...
// twi_result_t r = twi328p_init(16000000UL, 100000UL); // Init TWI at 100kHz with 16MHz CPU
// if (r != TWI_OK) { /* handle error */ }
// ...
// uint8_t data = 0;
// r = twi_read_reg(0x40, 0x01, &data); // Read register 0x01 from slave at address 0x40
// if (r != TWI_OK) { /* handle error */ }
// ...
// uint8_t value = 0x55;
// uint8 addr7 = 0x40;
// uint8 reg = 0x02;
// r = twi_write_reg(addr7, reg, value); // Write 0x55 to register 0x02 of slave at address 0x40
// if (r != TWI_OK) { /* handle error */ }
////
// End of example usage