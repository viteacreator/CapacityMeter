#include <Arduino.h>
#include <Wire.h>
#include "i2c_if.h"

static bool wire_init(void *ctx)
{
  (void)ctx;
  Wire.begin();
  return true;
}

static bool wire_write_reg(void *ctx, uint8_t addr7, uint8_t reg, const uint8_t *data, uint16_t len)
{
  (void)ctx;

  Wire.beginTransmission(addr7);
  Wire.write(reg);

  for (uint16_t i = 0; i < len; i++)
    Wire.write(data[i]);

  return (Wire.endTransmission() == 0);
}

static bool wire_read_reg(void *ctx, uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len)
{
  (void)ctx;

  Wire.beginTransmission(addr7);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) /* repeated start */
    return false;

  uint16_t got = Wire.requestFrom((int)addr7, (int)len);
  if (got != len)
    return false;

  for (uint16_t i = 0; i < len; i++)
    data[i] = (uint8_t)Wire.read();

  return true;
}

/* Export a ready-to-use interface instance */
extern "C" const i2c_if_t I2C_WIRE_ARDUINO = {
  .ctx = NULL,
  .init = wire_init,
  .deinit = NULL,
  .write_reg = wire_write_reg,
  .read_reg  = wire_read_reg
};
