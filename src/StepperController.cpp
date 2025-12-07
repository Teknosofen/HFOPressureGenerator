#include "StepperController.hpp"

StepperController::StepperController(int stepPin, int dirPin, int enablePin,
                                   int stepsPerRev, int m0Pin, int m1Pin, int m2Pin)
    : stepPin(stepPin)
    , dirPin(dirPin)
    , enablePin(enablePin)
    , m0Pin(m0Pin)
    , m1Pin(m1Pin)
    , m2Pin(m2Pin)
    , stepsPerRev(stepsPerRev)
    , currentMode(FULL_STEP)
    , currentPosition(0)
    , targetPosition(0)
    , targetSpeed(100.0)
    , currentSpeed(0.0)
    , maxSpeed(1000.0)
    , acceleration(0.0)  // 0 = no acceleration ramping
    , stepInterval(1000)
    , lastStepTime(0)
    , directionForward(true)
    , enabled(false)
{
}

void StepperController::init() {
    // Configure step and direction pins
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    digitalWrite(stepPin, LOW);
    digitalWrite(dirPin, LOW);
    
    // Configure enable pin if provided
    if (enablePin >= 0) {
        pinMode(enablePin, OUTPUT);
        digitalWrite(enablePin, HIGH);  // Disabled by default (active LOW)
    }
    
    // Configure microstepping pins if provided
    if (m0Pin >= 0) pinMode(m0Pin, OUTPUT);
    if (m1Pin >= 0) pinMode(m1Pin, OUTPUT);
    if (m2Pin >= 0) pinMode(m2Pin, OUTPUT);
    
    // Set default microstepping mode
    updateMicrostepPins();
    
    // Compute initial step interval
    computeStepInterval();
    
    Serial.println("StepperController initialized (DRV8825)");
    Serial.printf("  Step Pin: %d, Dir Pin: %d, Enable Pin: %d\n", stepPin, dirPin, enablePin);
    Serial.printf("  Steps/Rev: %d, Microstepping: 1/%d\n", stepsPerRev, currentMode);
}

void StepperController::setSpeed(float rpm) {
    if (rpm < 0.1) rpm = 0.1;
    if (rpm > 3000.0) rpm = 3000.0;
    
    // Convert RPM to steps per second, accounting for microstepping
    int effectiveStepsPerRev = stepsPerRev * currentMode;
    targetSpeed = (rpm * effectiveStepsPerRev) / 60.0;
    
    // If no acceleration, set current speed immediately
    if (acceleration == 0.0) {
        currentSpeed = targetSpeed;
    }
    
    computeStepInterval();
}

bool StepperController::setMicrostepMode(MicrostepMode mode) {
    // Check if microstepping pins are configured
    if (m0Pin < 0 || m1Pin < 0 || m2Pin < 0) {
        Serial.println("Error: Microstepping pins not configured");
        return false;
    }
    
    currentMode = mode;
    updateMicrostepPins();
    
    // Recompute speed with new microstepping
    float currentRPM = (currentSpeed * 60.0) / (stepsPerRev * currentMode);
    setSpeed(currentRPM);
    
    Serial.printf("Microstepping mode set to 1/%d\n", currentMode);
    return true;
}

void StepperController::updateMicrostepPins() {
    if (m0Pin < 0 || m1Pin < 0 || m2Pin < 0) return;
    
    // Set M0, M1, M2 pins according to microstepping mode
    switch (currentMode) {
        case FULL_STEP:        // 1/1:  M0=L, M1=L, M2=L
            digitalWrite(m0Pin, LOW);
            digitalWrite(m1Pin, LOW);
            digitalWrite(m2Pin, LOW);
            break;
        case HALF_STEP:        // 1/2:  M0=H, M1=L, M2=L
            digitalWrite(m0Pin, HIGH);
            digitalWrite(m1Pin, LOW);
            digitalWrite(m2Pin, LOW);
            break;
        case QUARTER_STEP:     // 1/4:  M0=L, M1=H, M2=L
            digitalWrite(m0Pin, LOW);
            digitalWrite(m1Pin, HIGH);
            digitalWrite(m2Pin, LOW);
            break;
        case EIGHTH_STEP:      // 1/8:  M0=H, M1=H, M2=L
            digitalWrite(m0Pin, HIGH);
            digitalWrite(m1Pin, HIGH);
            digitalWrite(m2Pin, LOW);
            break;
        case SIXTEENTH_STEP:   // 1/16: M0=L, M1=L, M2=H
            digitalWrite(m0Pin, LOW);
            digitalWrite(m1Pin, LOW);
            digitalWrite(m2Pin, HIGH);
            break;
        case THIRTYTWO_STEP:   // 1/32: M0=H, M1=H, M2=H
            digitalWrite(m0Pin, HIGH);
            digitalWrite(m1Pin, HIGH);
            digitalWrite(m2Pin, HIGH);
            break;
    }
}

void StepperController::setAcceleration(float stepsPerSecSquared) {
    acceleration = stepsPerSecSquared;
    if (acceleration < 0) acceleration = 0;
}

void StepperController::setMaxSpeed(float stepsPerSec) {
    maxSpeed = stepsPerSec;
    if (maxSpeed < 1.0) maxSpeed = 1.0;
}

void StepperController::enable() {
    if (enablePin >= 0) {
        digitalWrite(enablePin, LOW);  // Active LOW
    }
    enabled = true;
    Serial.println("Stepper motor enabled");
}

void StepperController::disable() {
    if (enablePin >= 0) {
        digitalWrite(enablePin, HIGH);  // Active LOW
    }
    enabled = false;
    currentSpeed = 0.0;
    Serial.println("Stepper motor disabled");
}

void StepperController::moveTo(long position) {
    targetPosition = position;
}

void StepperController::move(long steps) {
    targetPosition = currentPosition + steps;
}

void StepperController::stepForward() {
    move(1);
}

void StepperController::stepBackward() {
    move(-1);
}

bool StepperController::run() {
    if (!enabled) return false;
    
    // Check if we need to move
    long distToGo = distanceToGo();
    if (distToGo == 0) {
        currentSpeed = 0.0;
        return false;
    }
    
    // Set direction
    bool needForward = (distToGo > 0);
    if (needForward != directionForward) {
        setDirection(needForward);
    }
    
    // Handle acceleration
    if (acceleration > 0.0) {
        // Calculate time since last step
        unsigned long now = micros();
        float timeSinceLastStep = (now - lastStepTime) / 1000000.0;
        
        // Compute distance to deceleration point
        long stepsToStop = (long)((currentSpeed * currentSpeed) / (2.0 * acceleration));
        
        if (abs(distToGo) <= stepsToStop) {
            // Time to decelerate
            currentSpeed -= acceleration * timeSinceLastStep;
            if (currentSpeed < 0) currentSpeed = 0;
        } else {
            // Accelerate
            currentSpeed += acceleration * timeSinceLastStep;
            if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
            if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
        }
        
        computeStepInterval();
    }
    
    // Check if it's time to step
    unsigned long now = micros();
    if (now - lastStepTime >= stepInterval) {
        // Generate step pulse
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(2);  // Minimum pulse width for DRV8825 is 1.9μs
        digitalWrite(stepPin, LOW);
        
        // Update position
        if (directionForward) {
            currentPosition++;
        } else {
            currentPosition--;
        }
        
        lastStepTime = now;
        return true;
    }
    
    return true;  // Still moving
}

void StepperController::runToPosition() {
    while (isRunning()) {
        run();
        yield();  // Allow other tasks to run
    }
}

void StepperController::runToNewPosition(long position) {
    moveTo(position);
    runToPosition();
}

bool StepperController::isRunning() const {
    return enabled && (currentPosition != targetPosition);
}

void StepperController::stop() {
    targetPosition = currentPosition;
    currentSpeed = 0.0;
}

void StepperController::setCurrentPosition(long position) {
    currentPosition = position;
    targetPosition = position;
}

long StepperController::distanceToGo() const {
    return targetPosition - currentPosition;
}

void StepperController::setDirection(bool forward) {
    directionForward = forward;
    digitalWrite(dirPin, forward ? HIGH : LOW);
    delayMicroseconds(1);  // Direction setup time for DRV8825
}

void StepperController::reverseDirection() {
    setDirection(!directionForward);
}

void StepperController::computeStepInterval() {
    if (currentSpeed == 0.0) {
        stepInterval = 1000000;  // Very long interval (stopped)
        return;
    }
    
    // Calculate interval in microseconds
    stepInterval = (unsigned long)(1000000.0 / currentSpeed);
    
    // Ensure minimum interval (maximum speed limit)
    if (stepInterval < 50) {  // ~20kHz max step rate
        stepInterval = 50;
    }
}

float StepperController::constrainSpeed(float speed) {
    if (speed > maxSpeed) return maxSpeed;
    if (speed < -maxSpeed) return -maxSpeed;
    return speed;
}
