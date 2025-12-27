#include "ina_adc.h"
#include <math.h>
#include <stddef.h>

/* Private function declarations */
static bool ina226_write_register(INA226_t *ina, uint8_t reg, uint16_t data);
static bool ina226_read_register(INA226_t *ina, uint8_t reg, uint16_t *out);
static bool ina226_test_connection(INA226_t *ina);
static void ina226_calibrate(INA226_t *ina);

/* Initialize INA226 structure with values */
void ina226_init(INA226_t *ina, float r_shunt, float i_max, uint8_t address)
{
    ina->i2c_address = address;
    ina->r_shunt = r_shunt;
    ina->i_max = i_max;
    ina->current_lsb = 0.0f;
    ina->power_lsb = 0.0f;
    ina->cal_value = 0;

    /* Clear backend by default */
    ina->i2c.ctx = NULL;
    ina->i2c.init = NULL;
    ina->i2c.deinit = NULL;
    ina->i2c.write_reg = NULL;
    ina->i2c.read_reg = NULL;
}

void ina226_bind_i2c(INA226_t *ina, const i2c_if_t *i2c)
{
    if ((ina == NULL) || (i2c == NULL))
        return;

    ina->i2c = *i2c;
}

/* Initialize I2C communication and verify connection */
bool ina226_begin(INA226_t *ina)
{
    if ((ina == NULL) || (ina->i2c.read_reg == NULL) || (ina->i2c.write_reg == NULL))
    {
        return false; /* Check for valid INA226 structure and I2C backend */
    }
    if (ina->i2c.init && !ina->i2c.init(ina->i2c.ctx))
    {
        return false;
    }

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
    uint16_t cfg = 0;
    if (!ina226_read_register(ina, INA226_CFG_REG_ADDR, &cfg))
        return;
    cfg &= (uint16_t)~(0b111);
    /* Read config register and clear mode bits */

    (void)ina226_write_register(ina, INA226_CFG_REG_ADDR,
                                (uint16_t)(cfg | (state ? 0b000 : 0b111)));
    /* Write new config register value with selected mode */
}

/* Adjust calibration value */
void ina226_adj_calibration(INA226_t *ina, int16_t adj)
{
    int32_t cal = (int32_t)ina226_get_calibration(ina) + (int32_t)adj;

    if (cal < 0) cal = 0;
    if (cal > 65535) cal = 65535;
    /* Read and modify value */

    ina226_set_calibration(ina, (uint16_t)cal);
    ina->cal_value = (uint16_t)cal;
    /* Update internal variable */
}

/* Set calibration value */
void ina226_set_calibration(INA226_t *ina, uint16_t cal)
{
    (void)ina226_write_register(ina, INA226_CAL_REG_ADDR, cal);
    /* Write value to calibration register */

    ina->cal_value = cal; 
    /* Update internal variable */
}

/* Read calibration value */
uint16_t ina226_get_calibration(INA226_t *ina)
{
    uint16_t v = 0;
    if (ina226_read_register(ina, INA226_CAL_REG_ADDR, &v))
        ina->cal_value = v;
    /* Update internal variable */

    return ina->cal_value; /* Return value */
}

/* Set built-in sample averaging*/
void ina226_set_averaging(INA226_t *ina, uint8_t avg)
{
    uint16_t cfg_register = 0;
    if (!ina226_read_register(ina, INA226_CFG_REG_ADDR, &cfg_register))
        return;

    avg &= 0x07u; /* Ensure only 3 bits are used 0b00000111 */

    cfg_register &= (uint16_t)~((uint16_t)0x07u << 9);
    cfg_register |= (uint16_t)((uint16_t)avg << 9);
    /* Read config register, clearing AVG2-0 bits */

    (void)ina226_write_register(ina, INA226_CFG_REG_ADDR, cfg_register);
    /* Write new config register value */
}

/* Set resolution for selected channel  */
void ina226_set_sample_time(INA226_t *ina, bool ch, uint8_t mode)
{
    uint16_t cfg_register = 0;
    if (!ina226_read_register(ina, INA226_CFG_REG_ADDR, &cfg_register))
        return;

    mode &= 0x07u; /* Ensure only 3 bits are used 0b00000111 */

    cfg_register &= (uint16_t)~((uint16_t)0x07u << (ch ? 6 : 3));
    /* Read config register, clearing RES2-0 or RES5-3 bits*/

    cfg_register |= (uint16_t)((uint16_t)mode << (ch ? 6 : 3));
    /* Write required bit group, depending on channel, shunt or bus */

    (void)ina226_write_register(ina, INA226_CFG_REG_ADDR, cfg_register);
    /* Write new config register value */
}

/* Read shunt voltage */
float ina226_get_shunt_voltage(INA226_t *ina)
{
    ina226_set_calibration(ina, ina->cal_value);
    /* Force calibration update (in case of unexpected INA2xx reboot) */

    uint16_t raw = 0;
    if (!ina226_read_register(ina, INA226_SHUNT_REG_ADDR, &raw))
        return 0.0f;
    int16_t value = (int16_t)raw;
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
    uint16_t raw = 0;
    if (!ina226_read_register(ina, INA226_VBUS_REG_ADDR, &raw))
        return 0;
    return (uint16_t)(raw * 1.25f); /* still INA226 scale: 1.25mV/bit */
}

/* Read current in milliamps */
float ina226_get_mili_current(INA226_t *ina)
{
    ina226_set_calibration(ina, ina->cal_value);
    /* Force calibration update (in case of unexpected INA2xx reboot) */

    uint16_t raw = 0;
    if (!ina226_read_register(ina, INA226_CUR_REG_ADDR, &raw))
        return 0.0f;
    int16_t value = (int16_t)raw;
    return (value * ina->current_lsb * 1000.0f);
    /* LSB is calculated based on max expected current, multiply and return */
}

/* Read power */
float ina226_get_power(INA226_t *ina)
{
    ina226_set_calibration(ina, ina->cal_value);
    /* Force calibration update (in case of unexpected INA2xx reboot) */

    uint16_t raw = 0;
    if (!ina226_read_register(ina, INA226_POWER_REG_ADDR, &raw))
        return 0.0f;
    return raw * ina->power_lsb;
    /* LSB is 25 times larger than current LSB, multiply and return */
}

/* ===== Private Function Implementations ===== */

/* Write 16-bit INA226 register */
// static void ina226_write_register(INA226_t *ina, uint8_t address, uint16_t data)
// {
//     Wire.beginTransmission(ina->i2c_address); /* Start transmission */
//     Wire.write(address);                      /* Send address */
//     Wire.write((uint8_t)(data >> 8));         /* Send high byte */
//     Wire.write((uint8_t)(data & 0xFF));       /* Send low byte */
//     Wire.endTransmission();                   /* End transmission */
// }
static bool ina226_write_register(INA226_t *ina, uint8_t reg, uint16_t data)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(data >> 8);
    buf[1] = (uint8_t)(data & 0xFF);

    return ina->i2c.write_reg(ina->i2c.ctx, ina->i2c_address, reg, buf, 2);
}

/* Read 16-bit INA226 register */
// static uint16_t ina226_read_register(INA226_t *ina, uint8_t address)
// {
//     Wire.beginTransmission(ina->i2c_address);       /* Start transmission */
//     Wire.write(address);                            /* Send address */
//     Wire.endTransmission();                         /* End transmission */
//     Wire.requestFrom(ina->i2c_address, (uint8_t)2); /* Request 2 bytes */

//     uint16_t result = ((uint16_t)Wire.read() << 8) | Wire.read();
//     /* Combine high and low bytes */

//     return result; /* Return result */
// }
static bool ina226_read_register(INA226_t *ina, uint8_t reg, uint16_t *out)
{
    if (out == NULL || (out == NULL) || (ina->i2c.read_reg == NULL))
        return false;

    uint8_t buf[2] = {0};
    if (!ina->i2c.read_reg(ina->i2c.ctx, ina->i2c_address, reg, buf, 2))
        return false;

    *out = (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
    return true;
}

/* Check presence of device */
static bool ina226_test_connection(INA226_t *ina)
{
    uint16_t tmp = 0;
    return ina226_read_register(ina, INA226_CFG_REG_ADDR, &tmp);
}

/* Procedure for calculating calibration value and initialization */
static void ina226_calibrate(INA226_t *ina)
{
    (void)ina226_write_register(ina, INA226_CFG_REG_ADDR, 0x8000);
    /* Force reset */

    ina->current_lsb = ina->i_max / 32768.0f;
    /* Calculate LSB for current (see INA2xx datasheet) */

    ina->power_lsb = ina->current_lsb * 25.0f;
    /* Calculate LSB for power (see INA2xx datasheet) */

    ina->cal_value = (uint16_t)(0.00512f / (ina->current_lsb * ina->r_shunt));
    /* Calculate calibration value (see INA2xx datasheet) */

    ina226_set_calibration(ina, ina->cal_value);
    /* Write standard calibration value */
}
