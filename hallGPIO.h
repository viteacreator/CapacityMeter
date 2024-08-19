#ifndef HALLGPIO_H
#define HALLGPIO_H

typedef struct {
    volatile uint8_t *port;   // Pointer to the port register
    uint8_t pinNumber;        // Pin number
} Pin;

// Function to initialize a pin as input with pull-up
void initPinAsInput(Pin *pin) {
    *(pin->port - 1) &= ~(1 << pin->pinNumber);  // Set DDRx register to input
    *(pin->port) |= (1 << pin->pinNumber);       // Enable pull-up resistor
}

// Function to initialize a pin as output
void initPinAsOutput(Pin *pin) {
    *(pin->port - 1) |= (1 << pin->pinNumber);   // Set DDRx register to output
}

// Function to read the state of an input pin
uint8_t readPin(Pin *pin) {
    return (*(pin->port - 2) & (1 << pin->pinNumber)) ? 0 : 1;  // Read PINx register
}

// Function to set an output pin high
void setPinHigh(Pin *pin) {
    *(pin->port) |= (1 << pin->pinNumber);       // Set PORTx register
}

// Function to set an output pin low
void setPinLow(Pin *pin) {
    *(pin->port) &= ~(1 << pin->pinNumber);      // Clear PORTx register
}

// Function to toggle an output pin
void togglePin(Pin *pin) {
    *(pin->port) ^= (1 << pin->pinNumber);       // Toggle PORTx register
}

#endif  // HALLGPIO_H