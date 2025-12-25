#ifndef _INA226_h
#define _INA226_h
#include <Wire.h>
#include <Arduino.h>

/*
Constructor:	
    INA226 ina226 (Shunt resistance, Maximum expected current, I2C bus address)
    
    INA226 ina226;						// Standard values for INA226 module (0.1 Ohm, 0.8 A, address 0x40) - suitable for one module
    INA226 ina226 (0x41);				// Shunt and max current by default, address 0x41 - suitable for multiple modules
    INA226 ina226 (0.05f);				// Shunt 0.05 Ohm, max current and address by default (0.8 A, 0x40) - Modified module or bare chip
    INA226 ina226 (0.05f, 1.5f);		// Shunt 0.05 Ohm, max expected current 1.5 A, default address (0x40) - Modified module or bare chip
    INA226 ina226 (0.05f, 1.5f, 0x41);	// Shunt 0.05 Ohm, max expected current 1.5 A, address 0x41 - Modified modules or bare chips
    
Methods:
	bool begin();							    // Initialize module and check presence, returns false if INA226 not found
	bool begin(SCL, SDA);						// Initialize module for ESP8266/ESP32
    void sleep(true / false);				    // Enable or disable low power mode depending on argument
    void setAveraging(avg);					    // Set number of sample averaging (see table below)
    void setSampleTime(ch, time);			    // Set sample time for voltage and current (INA226_VBUS / INA226_VSHUNT), default INA226_CONV_1100US
        
    float getShuntVoltage(); 				    // Read shunt voltage
    float getVoltage();	 					    // Read voltage
    float getCurrent();  					    // Read current
    float getPower(); 						    // Read power
    
    uint16_t getCalibration();	 			    // Read calibration value (calculated automatically after startup)
    void setCalibration(calibration value);	    // Write calibration value (can store in EEPROM)	 		
    void adjCalibration(calibration offset);    // Adjust calibration value by specified amount (can change on the fly)
    
    Version 1.0 from 31.10.2021
*/

/* Public definitions (constants) */
#define INA226_VBUS true     // ADC channel measuring bus voltage (0-36V)
#define INA226_VSHUNT false  // ADC channel measuring shunt voltage

#define INA226_CONV_140US 0b000  // Sample time (signal accumulation for digitization)
#define INA226_CONV_204US 0b001
#define INA226_CONV_332US 0b010
#define INA226_CONV_588US 0b011
#define INA226_CONV_1100US 0b100
#define INA226_CONV_2116US 0b101
#define INA226_CONV_4156US 0b110
#define INA226_CONV_8244US 0b111

#define INA226_AVG_X1 0b000  // Built-in averaging (proportionally increases digitization time)
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

class INA226 {
public:

  INA226(const float r_shunt = 0.1f, const float i_max = 0.8f, const uint8_t address = 0x40)
    : _r_shunt(r_shunt), _i_max(i_max), _iic_address(address) {}


  INA226(const uint8_t address)
    : _r_shunt(0.1f), _i_max(3.2f), _iic_address(address) {}

  // Initialize and check
  bool begin(int __attribute__((unused)) sda = 0, int __attribute__((unused)) scl = 0) {
#if defined(ESP32) || defined(ESP8266)
    if (sda || scl) Wire.begin(sda, scl);  // Initialize for ESP
    else Wire.begin();
#else
    Wire.begin();  // Initialize I2C bus
#endif
    if (!testConnection()) return false;  // Check presence
    calibrate();                          // Calculate calibration value and initialize
    return true;                          // Return true if all OK
  }

  // Set / clear sleep mode
  void sleep(bool state) {
    uint16_t cfg_register = readRegister(INA226_CFG_REG_ADDR) & ~(0b111);        // Read config register and clear mode bits
    writeRegister(INA226_CFG_REG_ADDR, cfg_register | (state ? 0b000 : 0b111));  // Write new config register value with selected mode
  }

  // Adjust calibration value
  void adjCalibration(int16_t adj) {
    setCalibration(getCalibration() + adj);  // Read and modify value
    _cal_value = _cal_value + adj;           // Update internal variable
  }

  // Set calibration value
  void setCalibration(uint16_t cal) {
    writeRegister(INA226_CAL_REG_ADDR, cal);  // Write value to calibration register
    _cal_value = cal;                         // Update internal variable
  }

  // Read calibration value
  uint16_t getCalibration(void) {
    _cal_value = readRegister(INA226_CAL_REG_ADDR);  // Update internal variable
    return _cal_value;                               // Return value
  }

  // Set built-in sample averaging
  void setAveraging(uint8_t avg) {
    uint16_t cfg_register = readRegister(INA226_CFG_REG_ADDR) & ~(0b111 << 9);  // Read config register, clearing AVG2-0 bits
    writeRegister(INA226_CFG_REG_ADDR, cfg_register | avg << 9);                // Write new config register value
  }

  // Set resolution for selected channel
  void setSampleTime(bool ch, uint8_t mode) {
    uint16_t cfg_register = readRegister(INA226_CFG_REG_ADDR);  // Read config register
    cfg_register &= ~((0b111) << (ch ? 6 : 3));                 // Clear required bit group, depending on channel
    cfg_register |= mode << (ch ? 6 : 3);                       // Write required bit group, depending on channel
    writeRegister(INA226_CFG_REG_ADDR, cfg_register);           // Write new config register value
  }

  // Read shunt voltage
  float getShuntVoltage(void) {
    setCalibration(_cal_value);                           // Force calibration update (in case of unexpected INA219 reboot)
    int16_t value = readRegister(INA226_SHUNT_REG_ADDR);  // Read shunt voltage register
    return value * 0.0000025f;                            // LSB = 2.5uV = 0.0000025V, multiply and return
  }

  // // Read voltage
  // float getVoltage(void) {
  //   uint16_t value = readRegister(INA226_VBUS_REG_ADDR);  // Read voltage register
  //   return value * 0.00125f;                              // LSB = 1.25mV = 0.00125V, Shift value to 12 bits and multiply
  // }

  // // Read current
  // float getCurrent(void) {
  //   setCalibration(_cal_value);                         // Force calibration update (in case of unexpected INA219 reboot)
  //   int16_t value = readRegister(INA226_CUR_REG_ADDR);  // Read current register
  //   return value * _current_lsb;                        // LSB is calculated based on max expected current, multiply and return
  // }
  uint16_t getMiliVoltage(void) {
    uint16_t value = readRegister(INA226_VBUS_REG_ADDR);
    //value &= (uint16_t)~0x0001;          // clear last 2 bits (truncate)
    return value * 1.25f;             // still INA226 scale: 1.25mV/bit
    //return (value >> 2) * 5.0f * 1;  // LSB = 4mV = 0.004V, Shift value to 12 bits and multiply
  }
  // float getVoltage_rounded2LSB(void) {
  //   uint16_t raw = readRegister(INA226_VBUS_REG_ADDR);
  //   raw = (raw + 2) & (uint16_t)~0x0003;  // round to nearest multiple of 4
  //   return raw * 0.00125f;
  // }


  // Read voltage
  float getVoltage(void) {
    return ((float)getMiliVoltage() * 0.001f);  // LSB = 4mV = 0.004V, Shift value to 12 bits and multiply
  }

  // void setCalibVolt(float calibVal) {
  //   _calibVal = calibVal;
  // }

  // Read current
  float getMiliCurrent(void) {
    setCalibration(_cal_value);                         // Force calibration update (in case of unexpected INA219 reboot)
    int16_t value = readRegister(INA226_CUR_REG_ADDR);  // Read current register
    return (value * _current_lsb * 1000);               // LSB is calculated based on max expected current, multiply and return
  }

  // Read power
  float getPower(void) {
    setCalibration(_cal_value);                            // Force calibration update (in case of unexpected INA219 reboot)
    uint16_t value = readRegister(INA226_POWER_REG_ADDR);  // Read power register
    return value * _power_lsb;                             // LSB is 25 times larger than current LSB, multiply and return
  }

private:
  const uint8_t _iic_address = 0x00;  // I2C bus address
  const float _r_shunt = 0.0;         // Shunt resistance
  const float _i_max = 0.0;           // Max expected current

  float _current_lsb = 0.0;  // LSB for current
  float _power_lsb = 0.0;    // LSB for power
  uint16_t _cal_value = 0;   // Calibration value

  // Write 16-bit INA219 register
  void writeRegister(uint8_t address, uint16_t data) {
    Wire.beginTransmission(_iic_address);  // Start transmission
    Wire.write(address);                   // Send address
    Wire.write(highByte(data));            // Send high byte
    Wire.write(lowByte(data));             // Send low byte
    Wire.endTransmission();                // End transmission
  }

  // Read 16-bit INA219 register
  uint16_t readRegister(uint8_t address) {
    Wire.beginTransmission(_iic_address);        // Start transmission
    Wire.write(address);                         // Send address
    Wire.endTransmission();                      // End transmission
    Wire.requestFrom(_iic_address, (uint8_t)2);  // Request 2 bytes
    return Wire.read() << 8 | Wire.read();       // Combine and return result
  }

  // Check presence
  bool testConnection(void) {
    Wire.beginTransmission(_iic_address);  // Start transmission
    return (bool)!Wire.endTransmission();  // End immediately, invert result
  }

  // Procedure for calculating calibration value and initialization
  void calibrate(void) {
    writeRegister(INA226_CFG_REG_ADDR, 0x8000);  // Force reset

    _current_lsb = _i_max / 32768.0f;                          // Calculate LSB for current (see INA219 datasheet)
    _power_lsb = _current_lsb * 25.0f;                         // Calculate LSB for power (see INA219 datasheet)
    _cal_value = trunc(0.00512f / (_current_lsb * _r_shunt));  // Calculate calibration value (see INA219 datasheet)

    setCalibration(_cal_value);  // Write standard calibration value
  }
};
#endif