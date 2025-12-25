// #ifndef _INA219_h
// #define _INA219_h
// #include <Wire.h>
// #include <Arduino.h>

// /*
// Constructor:	
//     INA219 ina219 (Shunt resistance, Maximum expected current, I2C bus address)
    
//     INA219 ina219;						// Standard values for INA219 module (0.1 Ohm, 3.2A, address 0x40) - suitable for one module
//     INA219 ina219 (0x41);				// Shunt and max current by default, address 0x41 - suitable for multiple modules
//     INA219 ina219 (0.05f);				// Shunt 0.05 Ohm, max current and address by default (3.2A, 0x40) - Modified module or bare chip
//     INA219 ina219 (0.05f, 2.0f);		// Shunt 0.05 Ohm, max expected current 2A, default address (0x40) - Modified module or bare chip
//     INA219 ina219 (0.05f, 2.0f, 0x41);	// Shunt 0.05 Ohm, max expected current 2A, address 0x41 - Modified modules or bare chips
    
// Methods:
//     bool begin();							    // Initialize module and check presence, returns false if INA226 not found
// 	  bool begin(SCL, SDA);				  // Initialize module for ESP8266/ESP32
//     void sleep(true / false);				    // Enable or disable low power mode depending on argument
//     void setResolution(channel, mode);		    // Set resolution and averaging mode for voltage and current measurement
    
//     float getShuntVoltage(); 				    // Read shunt voltage
//     float getVoltage();	 					    // Read voltage
//     float getCurrent();  					    // Read current
//     float getPower(); 						    // Read power
    
//     uint16_t getCalibration();	 			    // Read calibration value (calculated automatically after startup)
//     void setCalibration(calibration value);	    // Write calibration value (can store in EEPROM)	 		
//     void adjCalibration(calibration offset);    // Adjust calibration value by specified amount (can change on the fly)
// */

// /* Public definitions (constants) */
// #define INA219_VBUS true              // ADC channel measuring bus voltage (0-26V)
// #define INA219_VSHUNT false           // ADC channel measuring shunt voltage
// #define INA219_RES_9BIT 0b0000        // 9 Bit - 84µs
// #define INA219_RES_10BIT 0b0001       // 10 Bit - 148µs
// #define INA219_RES_11BIT 0b0010       // 11 Bit - 276µs
// #define INA219_RES_12BIT 0b0011       // 12 Bit - 532µs
// #define INA219_RES_12BIT_X2 0b1001    // 12 Bit, average of 2 - 1.06 ms
// #define INA219_RES_12BIT_X4 0b1010    // 12 Bit, average of 4 - 2.13 ms
// #define INA219_RES_12BIT_X8 0b1011    // 12 Bit, average of 8 - 4.26 ms
// #define INA219_RES_12BIT_X16 0b1100   // 12 Bit, average of 16 - 8.51 ms
// #define INA219_RES_12BIT_X32 0b1101   // 12 Bit, average of 32 - 17.02 ms
// #define INA219_RES_12BIT_X64 0b1110   // 12 Bit, average of 64 - 34.05 ms
// #define INA219_RES_12BIT_X128 0b1111  // 12 Bit, average of 128 - 68.10 ms

// /* Private definitions (addresses) */
// #define INA219_CFG_REG_ADDR 0x00
// #define INA219_SHUNT_REG_ADDR 0x01
// #define INA219_VBUS_REG_ADDR 0x02
// #define INA219_POWER_REG_ADDR 0x03
// #define INA219_CUR_REG_ADDR 0x04
// #define INA219_CAL_REG_ADDR 0x05

// #define INA219_MAX_16V false
// #define INA219_MAX_32V true

// class INA219 {
// public:
//   // constructor
//   INA219(const float r_shunt = 0.1f, const float i_max = 3.2f, const bool v_max = INA219_MAX_32V, const uint8_t address = 0x40)  // v_max = 0x01(32 V), 0x00(16 V)
//     : _r_shunt(r_shunt), _i_max(i_max), _v_max(v_max), _iic_address(address) {}

//   // constructor
//   INA219(const bool v_max, const uint8_t address)
//     : _r_shunt(0.1f), _i_max(3.2f), _v_max(v_max), _iic_address(address) {}

//   // // constructor
//   // INA219(const uint8_t address)
//   //   : _r_shunt(0.1f), _i_max(3.2f), _v_max(INA219_MAX_32V), _iic_address(address) {}

//   float _calibVal = 1.0f;


//   // Initialize and check
//   bool begin(int __attribute__((unused)) sda = 0, int __attribute__((unused)) scl = 0) {
// #if defined(ESP32) || defined(ESP8266)
//     if (sda || scl) Wire.begin(sda, scl);  // Initialize for ESP
//     else Wire.begin();
// #else
//     Wire.begin();  // Initialize I2C bus
// #endif
//     if (!testConnection()) return false;  // Check presence
//     calibrate();                          // Calculate calibration value and initialize
//     return true;                          // Return true if all OK
//   }

//   // Set / clear sleep mode
//   void sleep(bool state) {
//     uint16_t cfg_register = readRegister(INA219_CFG_REG_ADDR) & ~(0b111);        // Read config register and clear mode bits
//     writeRegister(INA219_CFG_REG_ADDR, cfg_register | (state ? 0b000 : 0b111));  // Write new config register value with selected mode
//   }

//   // Adjust calibration value
//   void adjCalibration(int16_t adj) {
//     setCalibration(getCalibration() + adj);  // Read and modify value
//     _cal_value = _cal_value + adj;           // Update internal variable
//   }

//   // Set calibration value
//   void setCalibration(uint16_t cal) {
//     writeRegister(INA219_CAL_REG_ADDR, cal);  // Write value to calibration register
//     _cal_value = cal;                         // Update internal variable
//   }

//   // Read calibration value
//   uint16_t getCalibration(void) {
//     _cal_value = readRegister(INA219_CAL_REG_ADDR);  // Update internal variable
//     return _cal_value;                               // Return value
//   }

//   // Set resolution for selected channel
//   void setResolution(bool ch, uint8_t mode) {
//     uint16_t cfg_register = readRegister(INA219_CFG_REG_ADDR);  // Read config register
//     cfg_register &= ~((0b1111) << (ch ? 7 : 3));                // Clear required bit group, depending on channel
//     cfg_register |= mode << (ch ? 7 : 3);                       // Write required bit group, depending on channel
//     writeRegister(INA219_CFG_REG_ADDR, cfg_register);           // Write new config register value
//   }

//   // Read shunt voltage
//   float getShuntVoltage(void) {
//     setCalibration(_cal_value);                           // Force calibration update (in case of unexpected INA219 reboot)
//     int16_t value = readRegister(INA219_SHUNT_REG_ADDR);  // Read shunt voltage register
//     return value * 0.00001f;                              // LSB = 10uV = 0.00001V, multiply and return
//   }
  
//   uint16_t getMiliVoltage(void) {
//     uint16_t value = readRegister(INA219_VBUS_REG_ADDR);
//     return (value >> 2) * 2.0f * _calibVal;             // LSB = 4mV = 0.004V, Shift value to 12 bits and multiply
//   }

//   // Read voltage
//   float getVoltage(void) {
//     return ((float)getMiliVoltage() *0.001f);             // LSB = 4mV = 0.004V, Shift value to 12 bits and multiply
//   }

//   void setCalibVolt(float calibVal){
//     _calibVal = calibVal;
//   }

//   // Read current
//   float getMiliCurrent(void) {
//     setCalibration(_cal_value);                         // Force calibration update (in case of unexpected INA219 reboot)
//     int16_t value = readRegister(INA219_CUR_REG_ADDR);  // Read current register
//     return (value * _current_lsb * 1000);                        // LSB is calculated based on max expected current, multiply and return
//   }

//   // Read power
//   float getPower(void) {
//     setCalibration(_cal_value);                            // Force calibration update (in case of unexpected INA219 reboot)
//     uint16_t value = readRegister(INA219_POWER_REG_ADDR);  // Read power register
//     return value * _power_lsb;                             // LSB is 20 times larger than current LSB, multiply and return
//   }

// //private:
//   const uint8_t _iic_address = 0x00;  // I2C bus address
//   const float _r_shunt = 0.0;         // Shunt resistance
//   const float _i_max = 0.0;           // Max expected current
//   const bool _v_max = 0x00;           // Max expected voltage

//   float _current_lsb = 0.0;  // LSB for current
//   float _power_lsb = 0.0;    // LSB for power
//   uint16_t _cal_value = 0;   // Calibration value

//   // Write 16-bit INA219 register
//   void writeRegister(uint8_t address, uint16_t data) {
//     Wire.beginTransmission(_iic_address);  // Start transmission
//     Wire.write(address);                   // Send address
//     Wire.write(highByte(data));            // Send high byte
//     Wire.write(lowByte(data));             // Send low byte
//     Wire.endTransmission();                // End transmission
//   }

//   // Read 16-bit INA219 register
//   uint16_t readRegister(uint8_t address) {
//     Wire.beginTransmission(_iic_address);        // Start transmission
//     Wire.write(address);                         // Send address
//     Wire.endTransmission();                      // End transmission
//     Wire.requestFrom(_iic_address, (uint8_t)2);  // Request 2 bytes
//     return Wire.read() << 8 | Wire.read();       // Combine and return result
//   }

//   // Check presence
//   bool testConnection(void) {
//     Wire.beginTransmission(_iic_address);  // Start transmission
//     return (bool)!Wire.endTransmission();  // End immediately, invert result
//   }

//   // Procedure for calculating calibration value and initialization
//   void calibrate(void) {
//     writeRegister(INA219_CFG_REG_ADDR, 0x8000);  // Force reset

//     _current_lsb = _i_max / 32768.0f;                          // Calculate LSB for current (see INA219 datasheet)
//     _power_lsb = _current_lsb * 20.0f;                         // Calculate LSB for power (see INA219 datasheet)
//     _cal_value = trunc(0.04096f / (_current_lsb * _r_shunt));  // Calculate calibration value (see INA219 datasheet)

//     setCalibration(_cal_value);  // Write standard calibration value

//     uint16_t cfg_register = readRegister(INA219_CFG_REG_ADDR) & ~(0b0111 << 11);  // Read calibration register, clear shunt voltage range bits
//     uint16_t vshunt_max_mv = (_r_shunt * _i_max) * 1000;                        // Calculate max shunt voltage at max expected current

//     if (vshunt_max_mv <= 40) cfg_register |= 0b00 << 11;        // +/- 40mV - select range
//     else if (vshunt_max_mv <= 80) cfg_register |= 0b01 << 11;   // +/- 80mV
//     else if (vshunt_max_mv <= 160) cfg_register |= 0b10 << 11;  // +/- 160mV
//     else cfg_register |= 0b11 << 11;                            // +/- 320mV

//     if (!_v_max) cfg_register |= 0b0 << 13;                     // Select 16V
//     else if (_v_max) cfg_register |= 0b1 << 13;                 // 32V
//     writeRegister(INA219_CFG_REG_ADDR, cfg_register);           // Write config register
//   }
// };
// #endif