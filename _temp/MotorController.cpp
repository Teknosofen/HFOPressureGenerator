#include "MotorController.hpp"

MotorController::MotorController(int pin) 
    : escPin(pin), 
      currentMode(MODE_PWM), 
      motorForward(true),
      lastThrottleValue(1000),
      dshotMux(portMUX_INITIALIZER_UNLOCKED) {
}

void MotorController::init() {
    pinMode(escPin, OUTPUT);
    initPWM();
}

void MotorController::setMode(MotorMode mode) {
    if (currentMode == mode) return;
    
    currentMode = mode;
    
    if (mode == MODE_DSHOT) {
        ledcDetachPin(escPin);
        initDSHOT();
    } else {
        initPWM();
    }
}

void MotorController::setDirection(bool forward) {
    motorForward = forward;
}

void MotorController::setThrottleFromADC(int adcCounts) {
    if (currentMode == MODE_PWM) {
        int throttleUs = map(adcCounts, 0, 4095, 1000, 2000);
        setPWMThrottle(throttleUs);
        lastThrottleValue = throttleUs;
    } else {
        int throttlePkt = map(adcCounts, 0, 4095, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
        sendDSHOTThrottle(throttlePkt);
        lastThrottleValue = throttlePkt;
    }
}

const char* MotorController::getModeString() const {
    return (currentMode == MODE_PWM) ? "PWM" : "DSHOT";
}

const char* MotorController::getDirectionString() const {
    return motorForward ? "Forward" : "Reverse";
}

const char* MotorController::getThrottleLabel() const {
    return (currentMode == MODE_PWM) ? "Throttle (us)" : "Throttle (pkt)";
}

// ===== PWM Methods =====

void MotorController::initPWM() {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(escPin, PWM_CHANNEL);
    
    // Set idle throttle (1000 µs)
    float frameUs = 1000000.0 / PWM_FREQ;
    int dutyIdle = (int)((1000.0 / frameUs) * ((1 << PWM_RESOLUTION) - 1));
    ledcWrite(PWM_CHANNEL, dutyIdle);
}

void MotorController::setPWMThrottle(int throttleUs) {
    float frameUs = 1000000.0 / PWM_FREQ;
    int duty = (int)((throttleUs / frameUs) * ((1 << PWM_RESOLUTION) - 1));
    ledcWrite(PWM_CHANNEL, duty);
}

// ===== DSHOT Methods =====

void MotorController::initDSHOT() {
    pinMode(escPin, OUTPUT);
    digitalWrite(escPin, LOW);
}

void MotorController::sendDSHOTPacket(uint16_t payload11bits, bool telemetry) {
    uint16_t packet = (payload11bits << 1) | (telemetry ? 1 : 0);
    
    // Calculate checksum
    uint8_t csum = 0;
    uint16_t d = packet;
    for (int i = 0; i < 3; i++) {
        csum ^= (d & 0xF);
        d >>= 4;
    }
    packet = (packet << 4) | (csum & 0xF);
    
    // Send packet
    portENTER_CRITICAL(&dshotMux);
    for (int i = DSHOT_BITCOUNT - 1; i >= 0; i--) {
        bool bit = (packet >> i) & 1;
        if (bit) {
            gpioHigh(escPin);
            burnCycles(ONE_HI);
            gpioLow(escPin);
            burnCycles(ONE_LO);
        } else {
            gpioHigh(escPin);
            burnCycles(ZERO_HI);
            gpioLow(escPin);
            burnCycles(ZERO_LO);
        }
    }
    portEXIT_CRITICAL(&dshotMux);
}

void MotorController::sendDSHOTThrottle(int throttle) {
    throttle = constrain(throttle, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
    sendDSHOTPacket((uint16_t)throttle, false);
}
