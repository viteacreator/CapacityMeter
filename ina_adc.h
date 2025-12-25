#ifndef _INA226_h
#define _INA226_h
#include <Wire.h>
#include <Arduino.h>
#include <math.h>

/* 
 * INA226 Bi-Directional Current and Power Monitor
 * 
 * Constructor:	
 *     INA226_t ina226;  // Declare a structure
 *     ina226_init(&ina226, 0.1f, 0.8f, 0x40);  // Initialize with shunt resistance, max current, I2C address
 * 
 * Key Functions:
 *     bool ina226_begin(INA226_t *ina);                        // Initialize module and check presence
 *     void ina226_sleep(INA226_t *ina, bool state);            // Enable or disable low power mode
 *     void ina226_set_calibration(INA226_t *ina, uint16_t cal); // Set calibration value
 *     uint16_t ina226_get_calibration(INA226_t *ina);          // Read calibration value
 *     float ina226_get_shunt_voltage(INA226_t *ina);           // Read shunt voltage
 *     float ina226_get_voltage(INA226_t *ina);                 // Read bus voltage
 *     float ina226_get_mili_current(INA226_t *ina);            // Read current in mA
 *     float ina226_get_power(INA226_t *ina);                   // Read power
 *     void ina226_set_averaging(INA226_t *ina, uint8_t avg);   // Set sample averaging
 *     void ina226_set_sample_time(INA226_t *ina, bool ch, uint8_t mode); // Set sample time
 * 
 * Version 1.0 from 31.10.2021
 */

/* Public definitions (constants) */
#define INA226_VBUS true     /* ADC channel measuring bus voltage (0-36V) */
#define INA226_VSHUNT false  /* ADC channel measuring shunt voltage */

#define INA226_CONV_140US 0b000   /* Sample time (signal accumulation for digitization) */
#define INA226_CONV_204US 0b001
#define INA226_CONV_332US 0b010
#define INA226_CONV_588US 0b011
#define INA226_CONV_1100US 0b100
#define INA226_CONV_2116US 0b101
#define INA226_CONV_4156US 0b110
#define INA226_CONV_8244US 0b111

#define INA226_AVG_X1 0b000    /* Built-in averaging (proportionally increases digitization time) */
#define INA226_AVG_X4 0b001
#define INA226_AVG_X16 0b010
#define INA226_AVG_X64 0b011
#define INA226_AVG_X128 0b100
#define INA226_AVG_X256 0b101
#define INA226_AVG_X512 0b110
#define INA226_AVG_X1024 0b111

/* Private definitions (addresses) */
#define INA226_CFG_REG_ADDR 0x00
#define INA226_SHUNT_REG_ADDR 0x01
#define INA226_VBUS_REG_ADDR 0x02
#define INA226_POWER_REG_ADDR 0x03
#define INA226_CUR_REG_ADDR 0x04
#define INA226_CAL_REG_ADDR 0x05

/* INA226 Device Structure */
typedef struct {
  uint8_t iic_address;   /* I2C bus address */
  float r_shunt;         /* Shunt resistance */
  float i_max;           /* Max expected current */
  
  float current_lsb;     /* LSB for current */
  float power_lsb;       /* LSB for power */
  uint16_t cal_value;    /* Calibration value */
} INA226_t;

/* Function Declarations */

/* Initialize INA226 structure with default values */
void ina226_init(INA226_t *ina, float r_shunt, float i_max, uint8_t address);

/* Initialize I2C communication and verify connection */
bool ina226_begin(INA226_t *ina);

/* Set or clear sleep mode */
void ina226_sleep(INA226_t *ina, bool state);

/* Adjust calibration value */
void ina226_adj_calibration(INA226_t *ina, int16_t adj);

/* Set calibration value */
void ina226_set_calibration(INA226_t *ina, uint16_t cal);

/* Read calibration value */
uint16_t ina226_get_calibration(INA226_t *ina);

/* Set built-in sample averaging */
void ina226_set_averaging(INA226_t *ina, uint8_t avg);

/* Set resolution for selected channel */
void ina226_set_sample_time(INA226_t *ina, bool ch, uint8_t mode);

/* Read shunt voltage */
float ina226_get_shunt_voltage(INA226_t *ina);

/* Read bus voltage */
float ina226_get_voltage(INA226_t *ina);

/* Read bus voltage in millivolts */
uint16_t ina226_get_mili_voltage(INA226_t *ina);

/* Read current in milliamps */
float ina226_get_mili_current(INA226_t *ina);

/* Read power */
float ina226_get_power(INA226_t *ina);

#endif
