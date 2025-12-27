#include <stdint.h>
//

#define MAX_CYCLE_OF_TEST 5U
// Enum for Capacity Tester State
typedef enum
{
    IDLE,
    CHG,  // Charging
    DCHG, // Discharging
    REST  // Resting
} TesterState;

// Enum for Charge/Discharge Mode
typedef enum
{
    CC, // Constant Current
    CV  // Constant Voltage
} ChgDischgMode;

// Struct for Capacity Tester
typedef struct
{
    TesterState state;            // Current state: chg, dchg, rest
    ChgDischgMode chgDischgMode;  // Mode: cc, cv
    uint16_t chgCurrent;          // Charging current
    uint16_t testDoneCurrent;     // Current when the test is done
    uint16_t testDonePeriodOfTDC; // Period of wait time for done current ms
    int dchgCurrent;              // Discharging current
    int topVoltage;               // Top voltage
    int bottomVoltage;            // Bottom voltage
    int restTimeBetweenChDch;     // Rest time between charge and discharge
} CapacityTester;

// Struct for Battery
typedef struct
{
    uint8_t cycleIndex;                   // Current cycle index
    uint32_t chg_buf[MAX_CYCLE_OF_TEST];  // Buffer for charging data
    uint32_t dchg_buf[MAX_CYCLE_OF_TEST]; // Buffer for discharging data
    uint32_t chgCapacity;                 // Charging capacity
    uint32_t dchgCapacity;                // Discharging capacity
    uint32_t actualCap;                   // Actual capacity
    uint8_t internResist1;                // Internal resistance methode 1
    uint8_t internResist2;                // Internal resistance methode 2
} Battery;

int mainnn()
{
    // Example of how to initialize and use the structs
    CapacityTester tester = {
        .state = IDLE,
        .chgDischgMode = CC,
        .chgCurrent = 500,           // max charging current mA
        .testDoneCurrent = 50,       // threshold of final charging current mA
        .testDonePeriodOfTDC = 1000, // period of wait time for done current ms
        .dchgCurrent = 500,          // max dischg current mA
        .topVoltage = 4200,          // max voltage of btr at charge mV
        .bottomVoltage = 3000,       // min voltage of btr at dischg mV
        .restTimeBetweenChDch = 600  // 600 seconds
    };

    Battery battery = {
        .cycleIndex = 0,
        .chg_buf = {0},      // initialize all to 0
        .dchg_buf = {0},     // initialize all to 0
        .chgCapacity = 1000, // last capacity at charge mAh
        .dchgCapacity = 950, // last capacity at discharge mAh
        .actualCap = 970,    // used for current cap. measureing
        .internResist1 = 50, // determinated with methode 1, in  mOhm
        .internResist2 = 50  // determinated with methode 2, in mOhm
    };

    // Example usage of the structs

    return 0;
}
