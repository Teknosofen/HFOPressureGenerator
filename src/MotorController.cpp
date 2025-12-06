#include "MotorController.hpp"

MotorController::MotorController(int pin) 
    : escPin(pin), 
      lastThrottleValue(0),
      lastThrottleUs(1000) {
}

void MotorController::init() {
    pinMode(escPin, OUTPUT);
    
    // Setup PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(escPin, PWM_CHANNEL);
    
    // Set idle throttle (1000 µs) for ESC arming
    float frameUs = 1000000.0 / PWM_FREQ;
    int dutyIdle = (int)((1000.0 / frameUs) * ((1 << PWM_RESOLUTION) - 1));
    ledcWrite(PWM_CHANNEL, dutyIdle);
    
    lastThrottleUs = 1000;
}

void MotorController::setThrottleFromADC(int adcCounts) {
    lastThrottleValue = adcCounts;
    
    // Map ADC counts (0-4095) to throttle microseconds (1000-2000)
    int throttleUs = map(adcCounts, 0, 4095, 1000, 2000);
    setPWMThrottle(throttleUs);
    lastThrottleUs = throttleUs;
}

void MotorController::setPWMThrottle(int throttleUs) {
    float frameUs = 1000000.0 / PWM_FREQ;
    int duty = (int)((throttleUs / frameUs) * ((1 << PWM_RESOLUTION) - 1));
    ledcWrite(PWM_CHANNEL, duty);
}
