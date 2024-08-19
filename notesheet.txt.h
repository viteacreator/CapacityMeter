#include <stdint.h>

// Enum for Capacity Tester State
typedef enum {
    CHG,    // Charging
    DCHG,   // Discharging
    REST    // Resting
} TesterState;

// Enum for Charge/Discharge Mode
typedef enum {
    CC,     // Constant Current
    CV      // Constant Voltage
} ChgDischgMode;

// Struct for Capacity Tester
typedef struct {
    TesterState state;             // Current state: chg, dchg, rest
    ChgDischgMode chgDischgMode;   // Mode: cc, cv
    int chgCurrent;                // Charging current
    int testDoneCurrent;           // Current when the test is done
    int testDonePeriodOfTDC;       // Period when the test is done
    int dchgCurrent;               // Discharging current
    int topVoltage;                // Top voltage
    int bottomVoltage;             // Bottom voltage
    int restTimeBetweenChDch;      // Rest time between charge and discharge
} CapacityTester;

// Struct for Battery
typedef struct {
    int chgCapacity;   // Charging capacity
    int dchgCapacity;  // Discharging capacity
    int actualCap;     // Actual capacity
    int internResist;  // Internal resistance
} Battery;

int mainnn() {
    // Example of how to initialize and use the structs
    CapacityTester tester = {
        .state = REST,
        .chgDischgMode = CC,
        .chgCurrent = 500,           // 500 mA
        .testDoneCurrent = 50,       // 50 mA
        .testDonePeriodOfTDC = 1000, // 1000 ms
        .dchgCurrent = 500,          // 500 mA
        .topVoltage = 4200,          // 4200 mV
        .bottomVoltage = 3000,       // 3000 mV
        .restTimeBetweenChDch = 600  // 600 seconds
    };

    Battery battery = {
        .chgCapacity = 1000,       // 1000 mAh
        .dchgCapacity = 950,       // 950 mAh
        .actualCap = 970,          // 970 mAh
        .internResist = 50         // 50 mOhm
    };

    // Example usage of the structs

    return 0;
}
