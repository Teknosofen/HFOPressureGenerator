#ifndef MOTOR_CONTROLLER_HPP
#define MOTOR_CONTROLLER_HPP

#include <Arduino.h>

class MotorController {
public:
    MotorController(int pin);
    
    void init();
    void setThrottleFromADC(int adcCounts);
    
    int getThrottleValue() const { return lastThrottleValue; }
    int getThrottleUs() const { return lastThrottleUs; }

private:
    int escPin;
    int lastThrottleValue;  // ADC value
    int lastThrottleUs;     // Microseconds (1000-2000)
    
    // PWM settings
    static const int PWM_CHANNEL = 0;
    static const int PWM_FREQ = 50;
    static const int PWM_RESOLUTION = 14;
    
    void setPWMThrottle(int throttleUs);
};

#endif // MOTOR_CONTROLLER_HPP
