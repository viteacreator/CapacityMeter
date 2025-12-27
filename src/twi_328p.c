#include "twi_328p.h"

#include <avr/io.h>
#include <util/twi.h>

/* =========================
 * Internal helpers
 * ========================= */

/* Return masked TWI status (prescaler bits removed). */
static inline uint8_t twi_status(void)
{
    return (uint8_t)(TWSR & 0xF8);
}

/* Wait for TWINT with a simple timeout. */
static twi_result_t twi_wait_twint(void)
{
    uint16_t to = TWI_328P_TIMEOUT;
    while (!(TWCR & (1u << TWINT)))
    {
        if (--to == 0u)
        {
            return TWI_ERR_TIMEOUT;
        }
    }
    return TWI_OK;
}

/* Map arbitration/bus errors that may occur in multiple steps. */
static twi_result_t twi_map_common_errors(uint8_t st)
{
    if (st == TW_MT_ARB_LOST || st == TW_MR_ARB_LOST)
    {
        return TWI_ERR_ARB_LOST;
    }
    if (st == TW_BUS_ERROR)
    {
        return TWI_ERR_BUS_ERROR;
    }
    return TWI_ERR_UNEXPECTED_STATUS;
}

/* Compute TWBR for prescaler=1. Returns false if out of range. */
static bool twi_compute_twbr(uint32_t cpu_hz, uint32_t scl_hz, uint8_t *twbr_out)
{
    /* From datasheet: SCL = F_CPU / (16 + 2*TWBR*prescaler) */
    /* prescaler = 1 => SCL = F_CPU / (16 + 2*TWBR) */
    if (scl_hz == 0u || cpu_hz == 0u)
    {
        return false;
    }

    /* Ensure cpu_hz / scl_hz >= 16 to keep TWBR non-negative. */
    uint32_t div = cpu_hz / scl_hz;
    if (div < 16u)
    {
        return false;
    }

    uint32_t twbr = (div - 16u) / 2u;
    if (twbr > 255u)
    {
        return false;
    }

    *twbr_out = (uint8_t)twbr;
    return true;
}

/* =========================
 * Public API
 * ========================= */

twi_result_t twi328p_init(uint32_t cpu_hz, uint32_t scl_hz)
{
    uint8_t twbr = 0;

    if (!twi_compute_twbr(cpu_hz, scl_hz, &twbr))
    {
        return TWI_ERR_INVALID_ARG;
    }

    /* Prescaler = 1 (TWPS1..0 = 0). */
    TWSR &= (uint8_t)~((1u << TWPS0) | (1u << TWPS1));

    /* Bit rate. */
    TWBR = twbr;

    /*
     * Enable TWI.
     * Note: Internal pull-ups on SDA/SCL are NOT controlled by TWI; they are GPIO pull-ups.
     * If you want weak internal pull-ups, set DDRC bits to input and PORTC bits to 1 on PC4/PC5.
     */
    TWCR = (1u << TWEN);

    return TWI_OK;
}

void twi_deinit(void)
{
    /* Disable TWI. */
    TWCR = 0;
}

twi_result_t twi_start(void)
{
    /* Send START condition. */
    TWCR = (1u << TWINT) | (1u << TWSTA) | (1u << TWEN);

    twi_result_t r = twi_wait_twint();
    if (r != TWI_OK)
    {
        return r;
    }

    uint8_t st = twi_status();
    if (st == TW_START || st == TW_REP_START)
    {
        return TWI_OK;
    }

    return twi_map_common_errors(st);
}

void twi328p_stop(void)
{
    /* Send STOP condition. */
    TWCR = (1u << TWINT) | (1u << TWEN) | (1u << TWSTO);

    /*
     * Optional: wait for STOP to be transmitted (TWSTO cleared).
     * Not strictly required for typical use, but can help in some edge cases.
     */
    /* while (TWCR & (1u << TWSTO)) { } */
}

twi_result_t twi_send_address(uint8_t addr7, bool read)
{
    if (addr7 > 0x7Fu)
    {
        return TWI_ERR_INVALID_ARG;
    }

    uint8_t sla = (uint8_t)(addr7 << 1);
    if (read)
    {
        sla |= 1u; /* R */
    }

    TWDR = sla;
    TWCR = (1u << TWINT) | (1u << TWEN);

    twi_result_t r = twi_wait_twint();
    if (r != TWI_OK)
    {
        return r;
    }

    uint8_t st = twi_status();

    if (!read)
    {
        /* Master Transmitter mode: SLA+W */
        if (st == TW_MT_SLA_ACK)
        {
            return TWI_OK;
        }
        if (st == TW_MT_SLA_NACK)
        {
            return TWI_ERR_ADDR_NACK;
        }
        return twi_map_common_errors(st);
    }
    else
    {
        /* Master Receiver mode: SLA+R */
        if (st == TW_MR_SLA_ACK)
        {
            return TWI_OK;
        }
        if (st == TW_MR_SLA_NACK)
        {
            return TWI_ERR_ADDR_NACK;
        }
        return twi_map_common_errors(st);
    }
}

twi_result_t twi_write_byte(uint8_t data)
{
    TWDR = data;
    TWCR = (1u << TWINT) | (1u << TWEN);

    twi_result_t r = twi_wait_twint();
    if (r != TWI_OK)
    {
        return r;
    }

    uint8_t st = twi_status();
    if (st == TW_MT_DATA_ACK)
    {
        return TWI_OK;
    }
    if (st == TW_MT_DATA_NACK)
    {
        return TWI_ERR_DATA_NACK;
    }

    return twi_map_common_errors(st);
}

twi_result_t twi_read_byte(uint8_t *data, bool ack)
{
    if (data == 0)
    {
        return TWI_ERR_INVALID_ARG;
    }

    if (ack)
    {
        /* Receive byte and return ACK (request next byte). */
        TWCR = (1u << TWINT) | (1u << TWEN) | (1u << TWEA);
    }
    else
    {
        /* Receive last byte and return NACK. */
        TWCR = (1u << TWINT) | (1u << TWEN);
    }

    twi_result_t r = twi_wait_twint();
    if (r != TWI_OK)
    {
        return r;
    }

    uint8_t st = twi_status();

    if (ack)
    {
        if (st != TW_MR_DATA_ACK)
        {
            return twi_map_common_errors(st);
        }
    }
    else
    {
        if (st != TW_MR_DATA_NACK)
        {
            return twi_map_common_errors(st);
        }
    }

    *data = TWDR;
    return TWI_OK;
}

/* =========================
 * High-level helpers
 * ========================= */

twi_result_t twi_write_reg(uint8_t addr7, uint8_t reg, uint8_t data)
{
    twi_result_t r;

    r = twi_start();
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_send_address(addr7, false);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_write_byte(reg);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_write_byte(data);
    twi328p_stop();
    return r;
}

twi_result_t twi_read_reg(uint8_t addr7, uint8_t reg, uint8_t *data)
{
    twi_result_t r;

    if (data == 0)
    {
        return TWI_ERR_INVALID_ARG;
    }

    r = twi_start();
    if (r != TWI_OK) { twi328p_stop(); return r; }

    /* Write register pointer */
    r = twi_send_address(addr7, false);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_write_byte(reg);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    /* Repeated START, then read */
    r = twi_start();
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_send_address(addr7, true);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    /* Single byte read => NACK */
    r = twi_read_byte(data, false);
    twi328p_stop();
    return r;
}

twi_result_t twi_write_regs(uint8_t addr7, uint8_t start_reg, const uint8_t *buf, uint16_t len)
{
    twi_result_t r;

    if ((len > 0u) && (buf == 0))
    {
        return TWI_ERR_INVALID_ARG;
    }

    r = twi_start();
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_send_address(addr7, false);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_write_byte(start_reg);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    for (uint16_t i = 0; i < len; i++)
    {
        r = twi_write_byte(buf[i]);
        if (r != TWI_OK) { twi328p_stop(); return r; }
    }

    twi328p_stop();
    return TWI_OK;
}

twi_result_t twi_read_regs(uint8_t addr7, uint8_t start_reg, uint8_t *buf, uint16_t len)
{
    twi_result_t r;

    if ((len > 0u) && (buf == 0))
    {
        return TWI_ERR_INVALID_ARG;
    }

    if (len == 0u)
    {
        return TWI_OK;
    }

    r = twi_start();
    if (r != TWI_OK) { twi328p_stop(); return r; }

    /* Write register pointer */
    r = twi_send_address(addr7, false);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_write_byte(start_reg);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    /* Repeated START, then read */
    r = twi_start();
    if (r != TWI_OK) { twi328p_stop(); return r; }

    r = twi_send_address(addr7, true);
    if (r != TWI_OK) { twi328p_stop(); return r; }

    for (uint16_t i = 0; i < len; i++)
    {
        bool ack = (i < (len - 1u)); /* ACK all but last byte */
        r = twi_read_byte(&buf[i], ack);
        if (r != TWI_OK) { twi328p_stop(); return r; }
    }

    twi328p_stop();
    return TWI_OK;
}
