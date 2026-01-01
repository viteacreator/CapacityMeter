#ifndef I2C_IF_H
#define I2C_IF_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generic I2C interface for drivers (C ABI). */
typedef struct
{
    void *ctx;

    /* Optional: init/deinit for the bus instance (may be NULL). */
    bool (*init)(void *ctx);
    void (*deinit)(void *ctx);

    /* Write: [reg] + payload */
    bool (*write_reg)(void *ctx, uint8_t addr7, uint8_t reg, const uint8_t *data, uint16_t len);

    /* Read: write reg pointer, then read payload */
    bool (*read_reg)(void *ctx, uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len);

} i2c_if_t;

#ifdef __cplusplus
}
#endif

#endif /* I2C_IF_H */
