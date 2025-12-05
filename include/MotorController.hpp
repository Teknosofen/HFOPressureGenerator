#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

// DSHOT settings
#define DSHOT_THROTTLE_MIN 48
#define DSHOT_THROTTLE_MAX 2047
#define DSHOT_BITCOUNT     16

enum MotorMode {
    MODE_PWM,
    MODE_DSHOT
};

class MotorController {
public:
    MotorController(int pin);
    
    void init();
    void setMode(MotorMode mode);
    void setDirection(bool forward);
    void setThrottleFromADC(int adcCounts);
    
    MotorMode getMode() const { return currentMode; }
    bool isForward() const { return motorForward; }
    int getThrottleValue() const { return lastThrottleValue; }
    const char* getModeString() const;
    const char* getDirectionString() const;
    const char* getThrottleLabel() const;

private:
    int escPin;
    MotorMode currentMode;
    bool motorForward;
    int lastThrottleValue;
    
    // PWM settings
    static const int PWM_CHANNEL = 0;
    static const int PWM_FREQ = 50;
    static const int PWM_RESOLUTION = 14;
    
    // DSHOT bit timing (DSHOT150 at 240 MHz)
    static const uint32_t BIT_CYCLES = 1600;
    static const uint32_t ONE_HI = (BIT_CYCLES * 3) / 4;   // 1200 cycles
    static const uint32_t ONE_LO = BIT_CYCLES - ONE_HI;    // 400 cycles
    static const uint32_t ZERO_HI = (BIT_CYCLES * 3) / 8;  // 600 cycles
    static const uint32_t ZERO_LO = BIT_CYCLES - ZERO_HI;  // 1000 cycles
    
    portMUX_TYPE dshotMux;
    
    // PWM methods
    void initPWM();
    void setPWMThrottle(int throttleUs);
    
    // DSHOT methods
    void initDSHOT();
    void sendDSHOTPacket(uint16_t payload11bits, bool telemetry);
    void sendDSHOTThrottle(int throttle);
    
    // GPIO inline helpers
    static inline void gpioHigh(int pin) { GPIO.out_w1ts = (1UL << pin); }
    static inline void gpioLow(int pin) { GPIO.out_w1tc = (1UL << pin); }
    static inline void burnCycles(uint32_t cycles) {
        while (cycles--) { asm volatile("nop"); }
    }
};

#endif // MOTOR_CONTROLLER_H
