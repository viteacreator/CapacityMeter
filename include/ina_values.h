#ifndef INA_VALUES_H
#define INA_VALUES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint16_t g_ina_voltage_mv;
extern volatile int16_t g_ina_current_ma;

#ifdef __cplusplus
}
#endif

#endif /* INA_VALUES_H */
