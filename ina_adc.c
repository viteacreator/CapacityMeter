#include "ina_adc.h"
#include <math.h>

/* Private function declarations */
static void ina226_write_register(INA226_t *ina, uint8_t address, uint16_t data);
static uint16_t ina226_read_register(INA226_t *ina, uint8_t address);
static bool ina226_test_connection(INA226_t *ina);
static void ina226_calibrate(INA226_t *ina);

/* Initialize INA226 structure with values */
void ina226_init(INA226_t *ina, float r_shunt, float i_max, uint8_t address)
{
    ina->iic_address = address;
    ina->r_shunt = r_shunt;
    ina->i_max = i_max;
    ina->current_lsb = 0.0f;
    ina->power_lsb = 0.0f;
    ina->cal_value = 0;
}

/* Initialize I2C communication and verify connection */
bool ina226_begin(INA226_t *ina)
{
#if defined(ESP32) || defined(ESP8266)
    Wire.begin(); /* Initialize for ESP */
#else
    Wire.begin(); /* Initialize I2C bus */
#endif

    if (!ina226_test_connection(ina))
    {
        return false; /* Check presence */
    }

    ina226_calibrate(ina); /* Calculate calibration value and initialize */
    return true;           /* Return true if all OK */
}

/* Set or clear sleep mode */
void ina226_sleep(INA226_t *ina, bool state)
{
    uint16_t cfg_register = ina226_read_register(ina, INA226_CFG_REG_ADDR) & ~(0b111);
    /* Read config register and clear mode bits */

    ina226_write_register(ina, INA226_CFG_REG_ADDR,
                          cfg_register | (state ? 0b000 : 0b111));
    /* Write new config register value with selected mode */
}

/* Adjust calibration value */
void ina226_adj_calibration(INA226_t *ina, int16_t adj)
{
    ina226_set_calibration(ina, ina226_get_calibration(ina) + adj);
    /* Read and modify value */

    ina->cal_value = ina->cal_value + adj; /* Update internal variable */
}

/* Set calibration value */
void ina226_set_calibration(INA226_t *ina, uint16_t cal)
{
    ina226_write_register(ina, INA226_CAL_REG_ADDR, cal);
    /* Write value to calibration register */

    ina->cal_value = cal; /* Update internal variable */
}

/* Read calibration value */
uint16_t ina226_get_calibration(INA226_t *ina)
{
    ina->cal_value = ina226_read_register(ina, INA226_CAL_REG_ADDR);
    /* Update internal variable */

    return ina->cal_value; /* Return value */
}

/* Set built-in sample averaging */
void ina226_set_averaging(INA226_t *ina, uint8_t avg)
{
    uint16_t cfg_register = ina226_read_register(ina, INA226_CFG_REG_ADDR) & ~(0b111 << 9);
    /* Read config register, clearing AVG2-0 bits */

    ina226_write_register(ina, INA226_CFG_REG_ADDR, cfg_register | (avg << 9));
    /* Write new config register value */
}

/* Set resolution for selected channel */
void ina226_set_sample_time(INA226_t *ina, bool ch, uint8_t mode)
{
    uint16_t cfg_register = ina226_read_register(ina, INA226_CFG_REG_ADDR);
    /* Read config register */

    cfg_register &= ~((0b111) << (ch ? 6 : 3));
    /* Clear required bit group, depending on channel */

    cfg_register |= mode << (ch ? 6 : 3);
    /* Write required bit group, depending on channel */

    ina226_write_register(ina, INA226_CFG_REG_ADDR, cfg_register);
    /* Write new config register value */
}

/* Read shunt voltage */
float ina226_get_shunt_voltage(INA226_t *ina)
{
    ina226_set_calibration(ina, ina->cal_value);
    /* Force calibration update (in case of unexpected INA219 reboot) */

    int16_t value = ina226_read_register(ina, INA226_SHUNT_REG_ADDR);
    /* Read shunt voltage register */

    return value * 0.0000025f; /* LSB = 2.5uV = 0.0000025V, multiply and return */
}

/* Read bus voltage */
float ina226_get_voltage(INA226_t *ina)
{
    return ((float)ina226_get_mili_voltage(ina) * 0.001f);
    /* LSB = 4mV = 0.004V, Shift value to 12 bits and multiply */
}

/* Read bus voltage in millivolts */
uint16_t ina226_get_mili_voltage(INA226_t *ina)
{
    uint16_t value = ina226_read_register(ina, INA226_VBUS_REG_ADDR);
    return (uint16_t)(value * 1.25f); /* still INA226 scale: 1.25mV/bit */
}

/* Read current in milliamps */
float ina226_get_mili_current(INA226_t *ina)
{
    ina226_set_calibration(ina, ina->cal_value);
    /* Force calibration update (in case of unexpected INA219 reboot) */

    int16_t value = ina226_read_register(ina, INA226_CUR_REG_ADDR);
    /* Read current register */

    return (value * ina->current_lsb * 1000);
    /* LSB is calculated based on max expected current, multiply and return */
}

/* Read power */
float ina226_get_power(INA226_t *ina)
{
    ina226_set_calibration(ina, ina->cal_value);
    /* Force calibration update (in case of unexpected INA219 reboot) */

    uint16_t value = ina226_read_register(ina, INA226_POWER_REG_ADDR);
    /* Read power register */

    return value * ina->power_lsb;
    /* LSB is 25 times larger than current LSB, multiply and return */
}

/* ===== Private Function Implementations ===== */

/* Write 16-bit INA226 register */
static void ina226_write_register(INA226_t *ina, uint8_t address, uint16_t data)
{
    Wire.beginTransmission(ina->iic_address); /* Start transmission */
    Wire.write(address);                      /* Send address */
    Wire.write((uint8_t)(data >> 8));         /* Send high byte */
    Wire.write((uint8_t)(data & 0xFF));       /* Send low byte */
    Wire.endTransmission();                   /* End transmission */
}

/* Read 16-bit INA226 register */
static uint16_t ina226_read_register(INA226_t *ina, uint8_t address)
{
    Wire.beginTransmission(ina->iic_address);       /* Start transmission */
    Wire.write(address);                            /* Send address */
    Wire.endTransmission();                         /* End transmission */
    Wire.requestFrom(ina->iic_address, (uint8_t)2); /* Request 2 bytes */

    uint16_t result = ((uint16_t)Wire.read() << 8) | Wire.read();
    /* Combine high and low bytes */

    return result; /* Return result */
}

/* Check presence of device */
static bool ina226_test_connection(INA226_t *ina)
{
    Wire.beginTransmission(ina->iic_address); /* Start transmission */
    return (bool)!Wire.endTransmission();     /* End immediately, invert result */
}

/* Procedure for calculating calibration value and initialization */
static void ina226_calibrate(INA226_t *ina)
{
    ina226_write_register(ina, INA226_CFG_REG_ADDR, 0x8000);
    /* Force reset */

    ina->current_lsb = ina->i_max / 32768.0f;
    /* Calculate LSB for current (see INA219 datasheet) */

    ina->power_lsb = ina->current_lsb * 25.0f;
    /* Calculate LSB for power (see INA219 datasheet) */

    ina->cal_value = (uint16_t)(truncf(0.00512f / (ina->current_lsb * ina->r_shunt)));
    /* Calculate calibration value (see INA219 datasheet) */

    ina226_set_calibration(ina, ina->cal_value);
    /* Write standard calibration value */
}
